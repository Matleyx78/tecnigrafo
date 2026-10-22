# ADR 0022: Allineamenti, obliquo e specchiatura del testo

- **Stato:** Proposto
- **Data:** 2026-10-22
- **Sostituisce in parte:** ADR 0013, punto 3 (ingombro) e punto 1 (le
  proprietà "conservate" ora si usano)

## Contesto

L'ADR 0013 ha dato a `Text` tutte le proprietà del `TEXT` DXF, ma nella tappa
0.4 disegno e ingombro usano solo posizione, altezza, rotazione e fattore di
larghezza: allineamenti diversi da sinistra sulla linea di base, angolo
obliquo e specchiatura si conservano senza disegnarli. La tappa 0.5 apre DXF
scritti da altri, dove questi testi sono comuni: la scheda della tappa chiede
di disegnarli tutti.

Serve una definizione precisa, perché da essa dipendono disegno, ingombro,
selezione, snap e i riferimenti della modalità di controllo, che si scrivono
prima del codice e a mano.

## Decisione

### 1. Grandezze

Per un testo con altezza `h`, fattore di larghezza `f` e valore `v`:

- `L(v)` è la larghezza della riga nel font incluso, in unità dell'altezza
  delle maiuscole (ADR 0013, punto 3);
- `w = L(v) · h · f` è la larghezza del testo;
- `a` e `d` sono ascendente e discendente del font (`d < 0`).

Il **rettangolo del testo**, nel sistema del testo con l'origine all'inizio
della linea di base, va da `x = 0` a `x = w` e da `y = d·h` a `y = a·h`.

### 2. Punto d'ancoraggio

Il gruppo 72 (orizzontale) e il gruppo 73 (verticale) scelgono un punto del
rettangolo, `(ox, oy)`, che finisce sul punto d'ancoraggio `A`:

| 72 | `ox` | | 73 | `oy` |
|---|---|---|---|---|
| 0 `Left` | 0 | | 0 `Baseline` | 0 |
| 1 `Center` | `w/2` | | 1 `Bottom` | `d·h` |
| 2 `Right` | `w` | | 2 `Middle` | `h/2` |
| | | | 3 `Top` | `h` |

- `Top` e `Middle` si riferiscono all'altezza delle maiuscole, come in AutoCAD.
- **72 = 4, `Middle`**: `ox = w/2`, `oy = h·(1 + d)/2`, a metà tra la linea
  dei discendenti e la cima delle maiuscole. AutoCAD usa l'ingombro reale dei
  caratteri della riga; questa è la sua approssimazione con le misure del font,
  la stessa per ogni testo.
- **72 = 3, `Aligned`, e 72 = 5, `Fit`**: vedi il punto 3.
- Con 72 uguale a 3, 4 o 5 il gruppo 73 non conta (il DXF li ammette solo con
  73 = 0); si conserva com'è.
- **Punto d'ancoraggio:** `A = position` (10/20) quando 72 e 73 sono entrambi
  0; altrimenti `A = alignmentPoint` (11/21). Come in AutoCAD, l'altro punto
  si conserva com'è, senza ricalcolarlo.

### 3. `Aligned` e `Fit`

Il testo sta tra `P1 = position` e `P2 = alignmentPoint`:

- la rotazione è la direzione da `P1` a `P2` (il gruppo 50 si conserva ma non
  si usa);
- `Aligned`: l'altezza diventa `|P2 − P1| / (L(v) · f)`, il fattore di
  larghezza resta;
- `Fit`: il fattore di larghezza diventa `|P2 − P1| / (L(v) · h)`, l'altezza
  resta;
- `A = P1`, `ox = oy = 0`.

Il modello conserva altezza e fattore di larghezza scritti nel file; quelli
ricavati servono solo a disegno, ingombro, selezione e snap. Se `P1` e `P2`
coincidono, o `L(v)` è 0, il testo si tratta come `Left` sulla linea di base
in `P1`, con altezza e fattore di larghezza del file.

### 4. Obliquo, specchiatura e posizione nel disegno

Un punto `(x, y)` del sistema del testo va nel disegno così:

```
q = S · ((x, y) − (ox, oy))      S = [[1, tan(obliquo)], [0, 1]]
p = A + R(rotazione) · M · q     M = diag(mx, my)
```

- **Obliquo** (51): inclinazione a destra per angoli positivi, misurata dalla
  verticale, come in AutoCAD.
- **Specchiatura**: `mx = −1` se il testo è all'indietro (71, bit 2), `my = −1`
  se è capovolto (71, bit 4), altrimenti 1. Si specchia intorno al punto
  d'ancoraggio: un testo `Left` all'indietro si estende a sinistra di `A`.
- I caratteri si disegnano con la stessa trasformazione.
- Per un testo scritto dal comando `TEXT` (sinistra, linea di base, senza
  obliquo né specchiatura) il risultato è quello dell'ADR 0013: nulla cambia
  per i disegni della tappa 0.4.

### 5. Ingombro, selezione e snap

- Il **riquadro del testo** è l'immagine dei quattro angoli del rettangolo,
  in quest'ordine: `(0, d·h)`, `(w, d·h)`, `(w, a·h)`, `(0, a·h)`. È un
  parallelogramma; senza specchiatura gira in senso antiorario, con una sola
  specchiatura in senso orario. Sostituisce l'ingombro dell'ADR 0013,
  punto 3, e la proprietà derivata `~box` del rapporto.
- Il **riquadro allineato agli assi** (`bbox`) è il più piccolo che contiene i
  quattro angoli.
- **Selezione:** il testo è una regione, come nella tappa 0.4: un clic dentro
  il parallelogramma è a distanza 0; fuori, la distanza è quella dal suo bordo.
- **Snap d'inserimento:** in `A` per tutti gli allineamenti, tranne `Aligned`
  e `Fit`, dove è in `P1`.

### 6. Domande al modello

Le domande `box`, `bbox`, `distance`, `closest`, `inside`, `crosses`, `snaps`
e `pick` seguono questa definizione; il catalogo
(`docs/verifica/domande.md`) lo dice con il pacchetto che la realizza. I
riferimenti si calcolano a mano dalle formule qui sopra e dalle misure del font
(domanda `text-width`).

## Alternative considerate

- **Ricalcolare `position` dal punto d'allineamento, come fa AutoCAD quando
  modifica un testo:** cambierebbe un dato del file senza che l'utente abbia
  modificato niente; lo farà il comando che modifica il testo, quando
  arriverà.
- **`Middle` uguale a `Middle Center`:** più semplice, ma darebbe lo stesso
  punto a due allineamenti che in AutoCAD sono diversi.
- **`Middle` dall'ingombro reale dei caratteri:** più vicino ad AutoCAD, ma
  richiede le misure verticali di ogni carattere, che oggi `fontmetrics` non
  produce; si potrà fare con un nuovo ADR se le differenze si notano.
- **Specchiatura intorno al centro del testo:** non è ciò che fa AutoCAD.

## Conseguenze

- Disegno, ingombro, selezione e snap del testo cambiano solo per i testi con
  allineamenti, obliquo o specchiatura, che fino alla 0.4 nessun comando
  creava.
- I casi di verifica della 0.4 sui testi restano validi.
- Il comando `TEXT` potrà offrire la scelta dell'allineamento (opzione
  `Justify` di AutoCAD) senza decisioni nuove.
