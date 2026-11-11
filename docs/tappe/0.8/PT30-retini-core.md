# [0.8-PT30] `core`: retini nel modello, con il riempimento pieno

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT30]` (#…), agente codex
- **Dipende da:** PT5 (revisione delle interfacce), PT8, PT9, RT30
- **Modulo:** `core`
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punti 1–4;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto 2;
  la scheda [PT6](PT6-scheletro-core.md), punti 1 e 3

## File

- `core/include/tg/core/hatch.hpp`, `core/src/hatch.cpp`
- `core/src/hatch_patterns.cpp`, `core/include/tg/core/hatch_patterns.hpp` (nuovi)
- `tests/core/hatch_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const Hatch&)` restituisce `true` se ogni contorno è chiuso
   (polilinea chiusa, o lati che si seguono: la fine di un lato è l'inizio del
   successivo entro la regola delle tolleranze di solid-space, e l'ultimo torna
   al primo) e ha almeno un lato.
2. **Motivi predefiniti** (`hatch_patterns.hpp`): `ANSI31`, `NET` e `SOLID`
   nelle due versioni, con le righe del motivo come `PatternLine`:
   - `acad.pat`: `ANSI31` = una famiglia a 45°, origine 0,0, scostamento
     `0, 0.125`; `NET` = due famiglie a 0° e 90°, scostamento `0, 0.125`;
   - `acadiso.pat` (disegno metrico): gli stessi con scostamento `0, 3.175`;
   - la funzione che, dati nome, scala, angolo e `$MEASUREMENT`, dà le righe
     come le scrive AutoCAD nel `HATCH` (angolo sommato, origine e scostamento
     ruotati e scalati). Un nome che non c'è dà nessuna riga.
3. **Geometria generata:** con `solid`, una `FilledRegion` con gli anelli di
   tutti i contorni (polilinee chiuse; un contorno a lati diventa una polilinea
   con i bulge degli archi) quando lo stile delle isole è *normale*; per gli
   altri stili e per i motivi, nessun elemento fino a PT42 (ADR 0048, punto 3).
4. **Trasformazioni:** contorni, punti interni e origini delle righe trasformati;
   angolo del motivo più l'angolo della similitudine (con una riflessione,
   `2φ − angolo`), scala e scostamenti per il fattore di scala.
5. **Selezione** dalla geometria generata (PT6); **snap** e **grip**: nessuno.
6. **Ingombro:** quello dei contorni.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `hatch_test.cpp`: contorno aperto non supportato; `ANSI31` metrico ruotato di
  30° e scalato 2 (righe come AutoCAD); regione piena di un rettangolo con un
  cerchio dentro (due anelli); trasformazione con riflessione.

## Fatto quando

- I casi di RT30 passano; la CI è verde.
