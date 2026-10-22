# [0.5-P2] `core`: allineamenti, obliquo e specchiatura del testo

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #155
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.5-R2]` (#154), agente claude (calcoli delicati)
- **Dipende da:** R2
- **Modulo:** `core` (più due pagine di `docs/verifica/`)
- **Da leggere:** [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md)
  (tutto: è la specifica di questo pacchetto),
  [ADR 0013](../../adr/0013-testo-e-font.md) punti 1 e 3;
  `core/include/tg/core/text.hpp`, `core/src/text.cpp`,
  `core/src/proximity.cpp` (la parte sul testo), `core/src/snap_points.cpp`

## File

- `core/include/tg/core/text.hpp`
- `core/src/text.cpp`
- `core/src/proximity.cpp`
- `core/src/snap_points.cpp`
- `tests/core/text_test.cpp`
- `tests/core/proximity_test.cpp`
- `tests/core/snap_points_test.cpp`
- `docs/verifica/domande.md` (sezione "Testo" e righe `TEXT` delle tabelle)
- `docs/verifica/rapporto.md` (definizione di `~box`)

## Cosa fare

Oggi ingombro, selezione e snap del testo usano solo posizione, altezza,
rotazione e fattore di larghezza. Con questo pacchetto usano tutte le
proprietà, secondo l'ADR 0022.

### Il sistema del testo

```cpp
/// Where the text system goes in the drawing (docs/adr/0022, points 1-4):
/// the point (x, y) of the text system, in drawing units from the start of
/// the baseline, is origin + x * xAxis + y * yAxis.
struct TextFrame {
    Vec2 origin;
    Vec2 xAxis;         ///< R(rotation) * M * (1, 0)
    Vec2 yAxis;         ///< R(rotation) * M * (tan(oblique), 1)
    double height{0.0};       ///< used height (Aligned: derived)
    double widthFactor{0.0};  ///< used width factor (Fit: derived)
    double rotation{0.0};     ///< used rotation, radians (Aligned, Fit: P1 -> P2)
    Vec2 insertion;           ///< the insertion snap point (point 5)
};

[[nodiscard]] TextFrame textFrame(const Text& text);
```

- `origin = A − R·M·S·(ox, oy)`, con `A`, `(ox, oy)`, `S`, `M`, `R` come
  nell'ADR 0022.
- `Aligned` e `Fit` con `P1 = P2` o `L(v) = 0`: si comportano come `Left`
  sulla linea di base in `P1` (punto 3 dell'ADR).
- Con 72 uguale a 3, 4 o 5, il gruppo 73 si ignora.

### Ingombro

- `textBox()` restituisce le immagini degli angoli `(0, d·h)`, `(w, d·h)`,
  `(w, a·h)`, `(0, a·h)`, **in quest'ordine**, con `h` e `f` quelli usati.
  Il commento della funzione dice che senza specchiatura girano in senso
  antiorario e con una sola specchiatura in senso orario.
- `Text::boundingBox()` è il riquadro dei quattro angoli.
- Per un testo `Left` sulla linea di base, senza obliquo né specchiatura, il
  risultato è identico a oggi: i test e i casi della tappa 0.4 non cambiano.

### Selezione e snap

- `proximity.cpp`: distanza, punto più vicino, `inside`, `crosses` del testo
  usano il **parallelogramma** di `textBox()` (regione: distanza 0 dentro).
  Il `TextFrame` locale di `proximity.cpp` oggi presuppone un rettangolo:
  va generalizzato.
- `snap_points.cpp`: lo snap d'inserimento è `textFrame(text).insertion`.

### Documentazione della verifica

In `docs/verifica/domande.md`, sezione "Testo", e in `docs/verifica/rapporto.md`
per `~box`: la definizione del riquadro e dell'ordine dei quattro punti cambia
come sopra, con il rimando all'ADR 0022. Nessuna domanda nuova: `box`,
`bbox`, `distance`, `closest`, `inside`, `crosses`, `snaps`, `pick` restano
con la stessa forma.

## Regole della tappa che valgono qui

- `docs/verifica/domande.md` lo tocca anche P8: aggiungi solo le righe sul
  testo; chi arriva dopo si aggiorna.
- Nessun file di `render` o `app`: il disegno cambia con P6.

## Test richiesti

- Per ognuno dei 15 allineamenti (72 da 0 a 2 con 73 da 0 a 3, più
  `Aligned`, `Middle`, `Fit`), un testo con valore noto: `origin` e i quattro
  angoli, calcolati a mano dalle formule dell'ADR con le misure del font.
- Obliquo di 15° e di −15°; fattore di larghezza 0,8; rotazione 30°.
- Testo all'indietro, capovolto, entrambi: ordine e verso degli angoli.
- `Aligned` e `Fit` con `P1 = P2`, e con valore vuoto.
- Distanza e punto più vicino fuori e dentro un parallelogramma inclinato.
- Snap d'inserimento per `Left`, `MC`, `Aligned`, `Fit`.

## Fatto quando

- I casi di `[0.5-R2]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- I casi della tappa 0.4 sul testo passano ancora.
- Per un testo `Left` sulla linea di base, senza obliquo né specchiatura,
  riquadro, selezione e snap sono identici a prima.
