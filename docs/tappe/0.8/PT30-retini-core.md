# [0.8-PT30] `core`: retini nel modello, con il riempimento pieno

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT30]` (#…), agente codex
- **Dipende da:** PT1 (`sinCos`, `cardinalSinCos`), PT5 (revisione delle interfacce), PT8, PT9,
  RT30
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
   nelle due versioni. I numeri qui sotto sono quelli dei file `.pat`
   (angolo, origine `x0, y0`, scostamento `dx, dy`), non ancora i campi di
   `PatternLine`:
   - `acad.pat`: `ANSI31` = una famiglia a 45°, origine 0,0, scostamento
     `0, 0.125`; `NET` = due famiglie a 0° e 90°, scostamento `0, 0.125`;
   - `acadiso.pat` (disegno metrico): gli stessi con scostamento `0, 3.175`;
   - la funzione che, dati nome, scala `s`, angolo `θ` del retino (in
     radianti, già ridotto a un giro dal chiamante con
     `reducedDegreesToRadians` di PT1: PT31, PT44) e `$MEASUREMENT`, dà le
     righe come le scrive AutoCAD nel `HATCH`. Per una
     famiglia ad angolo `a` del `.pat`, la `PatternLine` ha: angolo `a + θ`
     (gruppo 53); origine = l'origine del `.pat` ruotata di `θ` e
     moltiplicata per `s` (43/44); **scostamento = `(dx, dy)` ruotato di
     `a + θ`, l'angolo della famiglia, e moltiplicato per `s`** (45/46,
     vettore nel sistema del disegno, quello che PT42 proietta); tratti
     moltiplicati per `s` (49). Le rotazioni si fanno con `cardinalSinCos`
     di PT1 (quarti di giro esatti: [DECISIONI.md](../../DECISIONI.md),
     2026-11-12, «Direzioni cardinali esatte» e «Quarti di giro esatti»): lo
     scostamento è `s · (dx · d + dy · n)`, con `d = cardinalSinCos(a + θ)`
     come `(cosine, sine)` e `n = (−d.sine, d.cosine)`, cioè con le
     componenti che `cardinalSinCos` restituisce, comprese quelle portate a 0
     o a ±1; l'origine ruota con `cardinalSinCos(θ)`. Le trasformazioni del
     punto 4 usano `Similarity::matrix`, che con PT1 dà anch'essa i quarti di
     giro esatti. Così il passo fra due righe,
     `offset · (−sin(a + θ), cos(a + θ))`, vale `s · dy`, e lo spostamento
     lungo la riga vale `s · dx`, per ogni famiglia e ogni angolo. Un nome che
     non c'è dà nessuna riga. È ciò che AutoCAD scrive: un `ANSI31` in
     pollici, angolo 0 e scala 1, ha nel DXF 45 = `−0.0883883476483185` e
     46 = `0.0883883476483185` (`non-gestiti.dxf` di dxf-test-file).
3. **Geometria generata:** con `solid`, una `FilledRegion` con gli anelli di
   tutti i contorni (polilinee chiuse; un contorno a lati diventa una polilinea
   con i bulge degli archi) quando lo stile delle isole è *normale*; per gli
   altri stili e per i motivi, nessun elemento fino a PT42 (ADR 0048, punto 3).
4. **Trasformazioni:** contorni, punti interni e origini delle righe trasformati;
   angolo del motivo più l'angolo della similitudine (con una riflessione,
   `2φ − angolo`), scala e tratti per il fattore di scala. Lo scostamento è un
   vettore del disegno (punto 2): riceve la parte lineare della similitudine,
   cioè rotazione, riflessione e fattore di scala, non lo spostamento. Così
   dopo una rotazione il passo resta `s · dy`; dopo una riflessione cambia
   segno (la famiglia è la stessa, con `k` contato al contrario) e lo
   spostamento lungo la riga resta `s · dx`.
5. **Selezione** dalla geometria generata (PT6): per la regione piena la
   distanza è 0 nell'area riempita (pari-dispari, con la sua affinità) e
   quella dal bordo fuori, foro compreso, come in un blocco (PT4, punto P3:
   [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle
   interfacce PT4»); **snap** e **grip**: nessuno. Anche l'angolo di ogni
   famiglia di righe ruota con il retino, come lo scostamento (punto 4; PT5,
   punto B19).
6. **Ingombro:** quello dei contorni.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `hatch_test.cpp`: contorno aperto non supportato; regione piena di un
  rettangolo con un cerchio dentro (due anelli); trasformazione con
  riflessione; un clic nel pieno prende il retino, uno al centro di un foro
  il cui bordo è più lontano dell'apertura del clic no; un motivo
  con una famiglia a 30° ruotato di 90° ha la famiglia a 120°;
- le righe dei motivi predefiniti (punto 2), con una tolleranza assoluta di
  `1e-12` sulle componenti, origine sempre `(0, 0)`:
  - `ANSI31` in pollici, `θ` = 0, `s` = 1: angolo 45, scostamento
    `(−0.0883883476483184, 0.0883883476483185)`;
  - `ANSI31` metrico, `θ` = 0, `s` = 1: angolo 45, scostamento
    `(−2.245064030267288, 2.245064030267288)`;
  - `ANSI31` metrico ruotato di 30° e scalato 2: angolo 75, scostamento
    `(−6.133628996935584, 1.643500936401007)`;
  - `NET` metrico, `θ` = 0, `s` = 1: angolo 0 con scostamento `(0, 3.175)` e
    angolo 90 con scostamento `(−3.175, 0)` esatto; in pollici gli stessi con
    `0.125`;
  - per ognuna, il passo `offset · (−sin, cos)` dell'angolo della riga vale
    `s · dy` del `.pat` (3,175, 0,125 o 6,35);
  - `ANSI31` metrico ruotato di 90° con `ROTATE`: angolo 135, scostamento
    `(−2.245064030267288, −2.245064030267288)`, passo 3,175; riflesso con
    `MIRROR` sull'asse x: angolo −45 (o 315, con la normalizzazione
    che il codice usa già per gli angoli riflessi), scostamento
    `(−2.245064030267288, −2.245064030267288)`, passo −3,175, spostamento
    lungo la riga 0;
  - una riga con angolo 0, scostamento `(2, 3)` e trattini, riflessa con
    `MIRROR` sull'asse x: scostamento `(2, −3)`, passo −3, spostamento lungo
    la riga 2, trattini invariati;
  - `NET` a 90°: scostamento esattamente `(−3.175, 0)` in millimetri e
    `(−0.125, 0)` in pollici.

## Fatto quando

- I casi di RT30 passano; la CI è verde.
