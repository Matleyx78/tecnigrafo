# [0.8-PT9] `script`: le entità nuove nel rapporto e nelle domande

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: è il sistema di verifica (come 0.6-P5); test dei
  suoi formati
- **Dipende da:** PT6
- **Modulo:** `script`
- **Da leggere:** [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md)
  (punti 1, 2, 6), [0045](../../adr/0045-blocchi-e-attributi.md),
  [0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  [0047](../../adr/0047-quote-e-stili-di-quota.md),
  [0048](../../adr/0048-retini.md); [verifica/rapporto.md](../../verifica/rapporto.md)
  per intero, in particolare «Come cresce il formato»;
  [verifica/domande.md](../../verifica/domande.md); la scheda
  [PT6](PT6-scheletro-core.md) (i tipi e `generate`); `script/src/report.cpp`,
  `entity_reader.cpp`, `question_table.cpp`, `questions.cpp`, `level1.cpp`

## Perché adesso

«Un tipo nuovo di entità si aggiunge a [rapporto.md] prima del pacchetto che lo
realizza» (rapporto.md, «Come cresce il formato»). Questo pacchetto fissa e
realizza il formato di tutti e sei i tipi, delle tre tabelle e della domanda
`generated`, così tutti i pacchetti di riferimenti della tappa possono partire.

## File

- `docs/verifica/rapporto.md`, `docs/verifica/domande.md`
- `script/src/report.cpp`, `script/src/entity_reader.cpp`,
  `script/include/tg/script/entity_reader.hpp`,
  `script/src/question_table.cpp`, `script/src/questions.cpp`,
  `script/src/text_format.cpp`
- `script/src/new_entities_format.cpp`, `script/src/new_entities_format.hpp`
  (nuovi: scrittura e lettura delle righe dei tipi nuovi)
- `tests/script/report_test.cpp`, `tests/script/entity_reader_test.cpp`,
  `tests/script/questions_test.cpp`, `tests/CMakeLists.txt` (file comune)

## Cosa fare

### 1. Le righe dei tipi nuovi

Si aggiungono a rapporto.md in una sezione «Tappa 0.8», con le regole di
sempre: proprietà nell'ordine scritto qui; quelle con un valore predefinito
indicato si scrivono solo quando sono diverse; prima delle derivate le
proprietà della 0.5 e della 0.6 (quote, spessore, estrusione, layer, colore…)
come per le altre entità.

```
SOLID vertex=0,0 vertex=10,0 vertex=0,5 vertex=10,5
```

I quattro vertici nell'ordine del DXF (10, 11, 12, 13), sempre quattro.

```
MTEXT insertion=0,0 height=2.5 width=60 attachment=top-left rotation=0 style="Standard" value="Prima\\Pseconda" ~lines=2
```

| Proprietà | DXF | Valore |
|---|---|---|
| `insertion` | 10/20 | punto d'inserimento |
| `height` | 40 | altezza |
| `width` | 41 | larghezza di riferimento, 0 = senza a capo |
| `attachment` | 71 | `top-left`, `top-center`, `top-right`, `middle-left`, `middle-center`, `middle-right`, `bottom-left`, `bottom-center`, `bottom-right` |
| `rotation` | 50 | gradi |
| `style` | 7 | nome dello stile |
| `line-spacing` | 44 | solo se diverso da 1 |
| `line-spacing-style` | 73 | `exactly`, solo se diverso da *at least* |
| `direction` | 72 | `by-style`, solo se diverso da sinistra a destra |
| `value` | 3 e 1 | il valore intero, codici compresi |
| `~lines` | — | derivata: il numero di righe della geometria generata |

```
ATTDEF position=0,0 alignment-point=0,0 height=1.8 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="N" tag="TG_PIN1" prompt="" invisible=yes constant=yes ~box=…
ATTRIB owner=300 position=… value="-K1" tag="TG_REF" invisible=no ~box=…
INSERT block="TG_EL_CONTACT_NO" insertion=10,20 scale=1,1 rotation=0
```

- `ATTDEF`: le proprietà di `TEXT` (con `value` = valore predefinito), poi
  `tag`, `prompt`, e `invisible`, `constant`, `verify`, `preset`, ognuna solo se
  `yes`.
- `ATTRIB`: `owner` (l'identificativo del suo `INSERT`), le proprietà di
  `TEXT`, poi `tag` e `invisible` solo se `yes`. Gli `ATTRIB` stanno nell'elenco
  `entities:` con il loro identificativo, come entità del DXF, e contano in
  `statistics:`.
- `INSERT`: `block` (nome tra virgolette), `insertion`, `scale` (X,Y),
  `rotation` (gradi).

```
DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=50,0 line-point=50,10 rotation=0 text-point=25,10.625 measurement=50 ~text="50"
```

| Proprietà | Quando | Valore |
|---|---|---|
| `kind` | sempre | `linear`, `aligned`, `angular`, `radius`, `diameter`, `other` |
| `style` | sempre | nome dello stile |
| `origin1`, `origin2` | lineare e allineata | 13/23 e 14/24 |
| `line-point` | lineare, allineata, angolare | 10/20 |
| `rotation` | lineare | 50, gradi |
| `first`, `second` | angolare | le due linee: `x1,y1;x2,y2` (13–14 e 10–15) |
| `arc-point` | angolare | 16/26 |
| `center`, `point` | radiale | 10/20 e 15/25 |
| `point1`, `point2` | diametrale | 15/25 e 10/20 |
| `type` | altro | il tipo DXF (70 & 7) |
| `text-point` | sempre | 11/21 |
| `measurement` | sempre | 42 |
| `text` | se non vuoto | il testo sostituito (1) |
| `text-rotation` | se diverso da 0 | 53, gradi |
| `user-text-position` | se `yes` | 70 bit 128 |
| `overrides` | se ce ne sono | le variabili essenziali sovrascritte, `NOME:valore` separate da virgole, in ordine alfabetico |
| `~text` | sempre | derivata: il testo che la grafica mostra, senza codici |

```
HATCH pattern="ANSI31" solid=no angle=0 scale=1 islands=normal loop=external+polyline vertex=0,0 vertex=10,0 vertex=10,10 vertex=0,10 loop=edges circle=5,5;2 seed=1,1
```

| Proprietà | DXF | Valore |
|---|---|---|
| `pattern` | 2 | nome |
| `solid` | 70 | `yes` o `no` |
| `angle`, `scale` | 52, 41 | gradi; scala o passo |
| `double` | 77 | solo se `yes` |
| `islands` | 75 | `normal`, `outer`, `ignore` |
| `loop` | 92 | un contorno; i segnali `external`, `outermost`, `derived`, `polyline`, `edges` uniti da `+`; poi i suoi elementi |
| `vertex`, `bulge` | 10/20, 42 | in un contorno a polilinea, come `LWPOLYLINE` |
| `line`, `arc`, `circle` | 72 | in un contorno a lati: `line=x1,y1;x2,y2`, `arc=cx,cy;r;a1;a2` (gradi, sempre antiorario come `ARC`), `circle=cx,cy;r` (un arco intero) |
| `seed` | 10/20 dopo 98 | un punto interno; si ripete |

### 2. Le tabelle e lo stato

Campi nuovi, scritti solo quando sono diversi da una sessione nuova:

| Riga | Contenuto |
|---|---|
| `text styles:` | gli stili diversi da `Standard` di una sessione nuova: `"NOME" font="…" height=… width-factor=… oblique=… backwards=no upside-down=no` |
| `text style:` | lo stile corrente, se diverso da `Standard` |
| `dimension styles:` | gli stili, se la tabella è diversa da quella di una sessione nuova: `"NOME"` e le variabili essenziali diverse da `ISO-25`, `NOME:valore` |
| `dimension style:` | lo stile corrente, se diverso da `ISO-25` |
| `dimension overrides:` | le variabili correnti diverse dallo stile corrente |
| `blocks:` | le definizioni, nell'ordine della tabella: per ognuna una riga `"NOME" base=x,y` (con `explodable=no`, `uniform=yes`, `units=N`, `description="…"` se diversi dal predefinito), poi una riga per ogni sua entità, `"NOME" <identificativo> <entità>` |

### 3. La domanda `generated`

In domande.md:

| Domanda | Risposta | Note |
|---|---|---|
| `generated <E>` | elenco di forme separate da `;`, o `none` | la geometria generata dell'entità (ADR 0044, punto 2), in ordine: ogni forma nel formato delle entità (`LINE`, `ARC`, `CIRCLE`, `LWPOLYLINE`, `TEXT`, `SOLID`, e `REGION` con i suoi `loop=` come `HATCH`), seguita dalle proprietà effettive diverse dal predefinito e da `~affine=a,b,c,d,e,f` se la trasformazione non è l'identità |

### 4. Definizioni di blocchi nelle domande

Nei file `.tgq`, `!block "NOME" base=x,y` apre una definizione di blocco nel
documento delle definizioni e `!endblock` la chiude: le definizioni di entità
fra le due righe entrano nel blocco. Così si definiscono `INSERT` da
interrogare prima che esistano i comandi.

### 5. Lettura

`entity_reader` legge anche le righe nuove (definizioni delle domande, file
`.tgq`), con gli stessi controlli delle altre entità. Una riga di un tipo il cui
pacchetto non è ancora unito (`core::isSupported` falso, PT6) si rifiuta con
l'errore `<TIPO> is not supported yet.`: i casi di quei tipi restano in attesa
(`tests/verifica/in-attesa/`) finché il pacchetto non è unito.

## Regole della tappa che valgono qui

- `rapporto.md` e `domande.md` li toccano solo PT9 e PT34 (righe del
  tracciamento): chi arriva dopo si aggiorna.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `report_test.cpp`: una riga per ogni tipo nuovo, con e senza le proprietà
  facoltative; i campi delle tabelle presenti solo quando diversi da una
  sessione nuova; un rapporto di una sessione nuova invariato.
- `entity_reader_test.cpp`: ogni riga scritta si rilegge uguale; righe non
  valide (valore mancante, `kind` sconosciuto) danno l'errore.
- `questions_test.cpp`: `generated` di una `LINE` è la linea stessa; `!block` e
  `!endblock` creano una definizione con le entità fra le due righe.

## Fatto quando

- rapporto.md e domande.md hanno i formati qui sopra, e `script` li scrive e li
  legge.
- I casi di verifica esistenti sono invariati; la CI è verde.
