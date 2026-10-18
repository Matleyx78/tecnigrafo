# ADR 0012: Linea, cerchio e arco

- **Stato:** Proposto
- **Data:** 2026-10-18

## Contesto

La tappa 0.4 aggiunge al modello le prime entità oltre alla polilinea. L'ADR 0004
le vuole tipi valore nel `std::variant` `Entity`; l'ADR 0006 vuole che ognuna
corrisponda a un'entità DXF con le stesse proprietà; l'ADR 0008 ha fissato le
regole di selezione e snap per le polilinee, che vanno estese.

Restano da decidere: cosa fare delle proprietà DXF che un CAD 2D non usa (quota,
spessore, direzione d'estrusione); come rappresentare archi e cerchi senza
perdere precisione; quando tre punti sono "allineati" per i comandi che
costruiscono un cerchio o un arco per tre punti; come si comportano gli archi
tangenti di `PLINE`.

## Decisione

### 1. Entità e corrispondenza DXF

| Tecnigrafo | DXF | Proprietà |
|---|---|---|
| `Line` | `LINE` | `start` 10/20, `end` 11/21 |
| `Circle` | `CIRCLE` | `center` 10/20, `radius` 40 |
| `Arc` | `ARC` | `center` 10/20, `radius` 40, `startAngle` 50, `endAngle` 51 |

- `Entity` diventa `std::variant<Polyline, Line, Circle, Arc>` (il testo arriva
  con l'ADR 0013).
- **Arco**: sempre in senso antiorario dall'angolo iniziale a quello finale, come
  nel DXF. Nel modello gli angoli sono in radianti, normalizzati in `[0, 2π)` e
  mai uguali; nel DXF e nel rapporto della modalità di controllo sono in gradi.
  Un arco dato in senso orario (per esempio da tre punti) si scrive scambiando
  gli estremi.
- **Cerchio e arco** sono entità distinte, come nel DXF: un arco di 360° non
  esiste.
- Raggio sempre maggiore di zero. I comandi non creano entità degeneri; come
  trattarle quando arrivano da un file lo decide la tappa 0.5.

### 2. Proprietà DXF non rappresentate

Le entità di Tecnigrafo sono piane, nel piano XY del sistema di coordinate
globale: quota Z zero (gruppi 30, 31), spessore zero (39), direzione
d'estrusione `(0, 0, 1)` (210/220/230). Sono i valori predefiniti del DXF, che
un file può omettere. La scheda della tappa 0.5 decide come conservare un file
che ne usa altri, nel rispetto dell'ADR 0006, punto 2. Le proprietà comuni
(handle, layer, colore, tipo di linea) arrivano con le tappe 0.5 e 0.6.

### 3. Geometria comune degli archi

`ArcGeometry` (centro, raggio, angolo iniziale, ampiezza con segno), che oggi
descrive i segmenti ad arco delle polilinee, diventa la base comune: un `Arc` e un
`Circle` (ampiezza 2π) si trattano con le stesse funzioni, pubbliche in
`core/arc_geometry.hpp`: punto a un angolo, riquadro esatto, tassellazione,
punto più vicino, intersezione con un rettangolo. Un arco non si converte mai in
un bulge: vicino ai 360° il bulge cresce senza limite e perde precisione.

### 4. Tre punti allineati

Tre punti `a`, `b`, `c` sono **allineati** se

```
|(b − a) × (c − a)| ≤ 1e-9 · L²
```

dove `L` è il lato più lungo del triangolo. Il criterio dipende solo dalla forma,
non dalle unità né dalla distanza dall'origine; vale anche quando due punti
coincidono. Il raggio più grande accettato è dell'ordine di 5·10⁸ volte il lato
più lungo. `circleThroughPoints` e `arcThroughPoints` restituiscono
`std::nullopt` per punti allineati, e i comandi lo dicono all'utente.

### 5. Selezione e snap

| Entità | Snap | Selezione |
|---|---|---|
| `Line` | estremi, punto medio | curva |
| `Arc` | estremi, punto medio sull'arco, centro | curva |
| `Circle` | centro | curva |

- Le tre entità sono **curve**, non regioni: si sceglie un'entità cliccando
  vicino alla curva, e un rettangolo di intersezione la prende solo se tocca la
  curva. Un clic al centro di un cerchio non lo sceglie.
- La finestra prende un'entità se il suo riquadro esatto è tutto dentro.
- Il centro di un cerchio si aggancia passando sul cerchio, come quello di un
  arco (ADR 0008, punto 4); per il resto valgono le regole di scelta dell'ADR
  0008.

### 6. Archi tangenti in `PLINE`

- L'arco parte tangente alla direzione finale del segmento precedente, retto o
  ad arco. Se la polilinea non ha ancora segmenti, è tangente all'asse X.
- Detto α l'angolo con segno dalla tangente alla corda verso il punto finale, il
  segmento ha `bulge = tan(α / 2)` (ADR 0004).
- Con il punto finale dritto davanti alla tangente il segmento è retto
  (bulge 0). Con il punto dritto dietro (`π − |α| ≤ 1e-9`) l'arco non esiste e il
  comando lo dice.
- *CLose* nel modo arco chiude con un arco tangente fino al primo vertice.
- Parole chiave come in AutoCAD, perché gli script `.scr` restino validi:
  `Arc` (`A`), `Line` (`L`), `CLose` (`CL`), `Undo` (`U`).

### 7. Verifica

Le tre entità rispondono alle domande al modello generali dell'ADR 0011 (punto 3:
`bbox`, `length`, `distance`, `closest`, `inside`, `crosses`, `snaps`, e sul
documento `pick`, `window`, `crossing`, `snap`). Il cerchio e l'arco per tre
punti si verificano con le costruzioni `circle-through` e `arc-through`.

## Alternative considerate

- **Linea come polilinea di due vertici:** un solo tipo in meno, ma il DXF le
  distingue e l'andata e ritorno non sarebbe più senza perdite.
- **Arco come segmento con bulge:** riusa tutto il codice delle polilinee, ma
  perde precisione negli archi quasi completi.
- **Cerchio come arco di 360°:** il DXF ha un'entità propria, e un arco con
  angoli uguali è ambiguo.
- **Punti 3D fin da ora:** il modellatore 3D (fase 2) avrà un proprio modello
  geometrico; nel 2D la terza coordinata servirebbe solo a conservare file
  insoliti, e se ne occupa la tappa 0.5.
- **Tolleranza assoluta per i punti allineati:** dipenderebbe dalle unità del
  disegno (millimetri o metri) e dalla scala.

## Conseguenze

- Ogni `std::visit` del progetto gestisce le tre entità; la tassellazione di
  archi e cerchi riusa quella delle polilinee.
- La scheda della 0.5 deve dire come si conservano quota, spessore ed
  estrusione diversi dai valori predefiniti.
- I comandi di modifica della 0.7 lavorano su tre tipi in più; per esempio la
  specchiatura di un arco ne scambia gli angoli.
- Gli snap a quadrante, tangente e perpendicolare (0.7) si aggiungono in
  `arc_geometry`, in un solo punto.
