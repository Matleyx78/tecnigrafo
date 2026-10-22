# [0.5-P11] `io`: linee, cerchi, archi ed estrusione

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.5-R11]` (#…), agente codex (calcoli delicati)
- **Dipende da:** P10, P12, R11
- **Modulo:** `io` (più le righe di quota, spessore ed estrusione in `script`)
- **Da leggere:** [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md)
  punti 3 e 5, [ADR 0012](../../adr/0012-linea-cerchio-arco.md) punti 1–2,
  [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punto 4;
  `io/src/dxf_entity_common.hpp` (P12), `dxf_lwpolyline.cpp`, `dxf_text.cpp`,
  `dxf_read.cpp`; `docs/verifica/rapporto.md` (proprietà nuove, P8); la *DXF
  Reference*, "LINE", "CIRCLE", "ARC" e "Arbitrary Axis Algorithm"

## File

- `io/src/dxf_line.cpp` (nuovo)
- `io/src/dxf_circle_arc.cpp` (nuovo)
- `io/src/dxf_extrusion.hpp` (nuovo, interno)
- `io/src/dxf_extrusion.cpp` (nuovo)
- `io/src/dxf_read.cpp` (lascia passare l'estrusione `(0,0,-1)`)
- `io/src/dxf_lwpolyline.cpp` e `io/src/dxf_text.cpp` (applicano la
  conversione)
- `io/src/dxf_entities.cpp` (tre righe nella tabella)
- `io/CMakeLists.txt`
- `script/src/entity_reader.cpp` e `script/src/report.cpp` (proprietà `z`,
  `start-z`, `end-z`, `alignment-z`, `elevation`, `thickness`, `extrusion`)
- `tests/io/dxf_line_circle_arc_test.cpp` (nuovo)
- `tests/io/dxf_extrusion_test.cpp` (nuovo)
- `tests/script/entity_reader_test.cpp`
- `tests/verifica/in-attesa.txt` (togliere le righe di R11)

## Cosa fare

### `LINE`, `CIRCLE`, `ARC`

Con la parte comune di P12:

- `LINE`: 10/20, 11/21. Una linea di lunghezza zero resta gestita (il DXF la
  ammette e i comandi della 0.4 no, ma non è degenere per l'ADR 0018).
- `CIRCLE`: 10/20, 40. `ARC`: 10/20, 40, poi `100 AcDbArc`, 50, 51 in
  gradi; nel modello radianti in `[0, 2π)` (ADR 0012).
- Restano **non gestiti**: raggio ≤ 0; arco con angolo iniziale uguale al
  finale dopo la normalizzazione.

### Estrusione `(0,0,-1)` per tutte le entità (`dxf_extrusion`)

Un'estrusione è `(0,0,-1)` quando le prime due componenti sono entro 1e-12
da zero e la terza è negativa; `(0,0,1)` allo stesso modo con la terza
positiva. In `dxf_read.cpp` la regola di P10 diventa: entità con
estrusione `(0,0,1)` o `(0,0,-1)` vanno al lettore del tipo; le altre restano
non gestite.

Con `(0,0,-1)` il lettore converte nel sistema globale (ADR 0018, punto 5) e
mette `flippedExtrusion = true`; lo scrittore fa la conversione inversa:

| Entità | Lettura (file → modello) | Scrittura (modello → file) |
|---|---|---|
| `LINE` | nessuna: i punti sono già globali | nessuna |
| `CIRCLE` | centro `x → −x` | centro `x → −x` |
| `ARC` | centro `x → −x`; inizio `180° − fine`, fine `180° − inizio` | l'inverso, la stessa formula |
| `LWPOLYLINE` | vertici `x → −x`; bulge `→ −bulge` | lo stesso |
| `TEXT` | 10 e 11 `x → −x`; rotazione `→ −r`; bit 2 di 71 invertito | lo stesso |

Gli angoli risultanti si normalizzano in `[0°, 360°)`. Le formule si
ricavano con l'algoritmo dell'asse arbitrario: per `(0,0,-1)` l'asse X del
sistema dell'oggetto è `(−1, 0, 0)` e l'asse Y è `(0, 1, 0)`. La pull request
riporta la derivazione in due righe.

### Modalità di controllo

`entity_reader.cpp` e `report.cpp` leggono e scrivono le proprietà di
`docs/verifica/rapporto.md` (P8) che vengono da `placement`: `start-z`,
`end-z` (`LINE`), `z` (`CIRCLE`, `ARC`, `TEXT`), `alignment-z` (`TEXT`),
`elevation` (`LWPOLYLINE`), `thickness`, `extrusion`; solo quando diverse dal
valore predefinito. Per un'entità convertita la geometria è quella globale e
la riga finisce con `extrusion=0,0,-1`.

## Regole della tappa che valgono qui

- Quota, spessore ed estrusione si conservano come letti e non si disegnano.
- `io/src/dxf_entities.cpp`, `io/CMakeLists.txt`, `tests/CMakeLists.txt` e i
  due file di `script` sono comuni con P12: solo le tue righe.

## Test richiesti

- Andata e ritorno di linee, cerchi e archi casuali (seme fisso): modello
  entro 1e-9, gruppi in più uguali, angoli entro la tolleranza.
- Per ogni entità con `(0,0,-1)`: valori nel modello calcolati a mano, e
  riscrittura con gli stessi valori del file entro 1e-9.
- Archi specchiati che attraversano 0°; testo all'indietro specchiato (torna
  normale); polilinea con bulge.
- Estrusione `(0, 0.5, 0.866)`: non gestita, riscritta identica.
- Raggio 0 e −1, arco con angoli uguali: non gestiti.
- Quota e spessore diversi da 0: conservati e scritti nel rapporto.

## Fatto quando

- I casi di `[0.5-R11]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- I casi di R10 e R12 passano ancora.
- I DXF pubblici con linee, cerchi e archi si leggono con i valori di ezdxf
  e si riscrivono identici.
- La CI è verde, compreso il fuzzing.
