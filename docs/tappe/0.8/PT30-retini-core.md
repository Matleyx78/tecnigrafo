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
  la scheda [PT6](PT6-scheletro-core.md), punti 1 e 3;
  `core/include/tg/core/tolerance.hpp` (`ToleranceRule`),
  `core/include/tg/core/polyline.hpp` (`toPolyline(const Arc&)`),
  `core/include/tg/core/generated.hpp` (`extent`, `appendSnapPoints` del
  documento), `script/src/question_table.cpp` (`askBoundingBox`, `askSnaps`)

## File

- `core/include/tg/core/hatch.hpp`, `core/src/hatch.cpp`
- `core/src/hatch_patterns.cpp`, `core/include/tg/core/hatch_patterns.hpp` (nuovi)
- `script/src/question_table.cpp` (file comune: solo `askBoundingBox` e
  `askSnaps`, punto 7)
- `tests/core/hatch_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const Hatch&)` restituisce `true` se ogni contorno è una
   polilinea chiusa (`isClosed()`) con almeno un vertice, o un contorno a lati
   con almeno un lato. **Non controlla che i lati si seguano**: la chiusura di
   un contorno a lati dipende dall'unità del disegno, che `supported` non
   conosce, e la controlla chi crea il retino, con la regola del documento: la
   lettura DXF ([PT31](PT31-retini-dxf.md), punto 2) e `-HATCH`
   ([PT44](PT44-comando-hatch.md), punto 2) ([DECISIONI.md](../../DECISIONI.md),
   2026-11-16, «Retini: chiusura dei contorni, domande dal documento, cerchio
   dei contorni a lati»). Così `supported` non cambia con le unità del disegno
   né con una trasformazione (punto 4): un `SCALE` che porta uno scarto oltre
   la tolleranza non rende il retino non gestito, e `transformedOf` non
   rifiuta nulla per questo. Per chi crea il retino, in `hatch.hpp`:

   ```cpp
   /// Whether `loop` is closed under `rule` (docs/tappe/0.8/PT30-retini-core.md,
   /// point 1): a polyline loop when its polyline is closed; an edge loop when
   /// it has an edge and the end of every edge, run as point 3 says, coincides
   /// under `rule` with the start of the next one, the last with the first (one
   /// edge: its own start). supported() does not check it: whoever makes a
   /// hatch does, with the rule of its document (the DXF reader, -HATCH).
   /// An edge beyond solid-space's domain (docs/adr/0035, point 23) leaves the
   /// loop open.
   [[nodiscard]] bool isClosed(const HatchLoop& loop, const ToleranceRule& rule);
   ```

   Gli estremi di un lato sono quelli del punto 3 (per un arco, quelli di
   `toPolyline`); il confronto è `ToleranceRule::coincident`.
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
   con i bulge degli archi, qui sotto) quando lo stile delle isole è *normale*;
   per gli altri stili e per i motivi, nessun elemento fino a PT42 (ADR 0048,
   punto 3).

   **La polilinea di un contorno a lati** prende i punti dei lati così come
   sono, senza controllare che si seguano (punto 1), con gli stessi `double`:
   - **Il verso dei lati.** Un arco del modello è sempre antiorario (PT6), anche
     quando il contorno lo percorre in senso orario, per esempio dopo un
     `MIRROR` (punto 4): il verso di ogni lato si ricava dai suoi estremi. Gli
     estremi di una linea sono i suoi due punti; quelli di un arco sono i due
     vertici di `toPolyline` (gli estremi esatti sugli assi). Il primo lato
     finisce nel suo estremo più vicino a uno dei due estremi del secondo; ogni
     lato dopo il primo parte dal suo estremo più vicino alla fine del lato
     prima. A parità di distanza vale il verso in cui il lato è scritto: una
     linea dal primo punto al secondo, un arco dall'angolo iniziale a quello
     finale.
   - **I vertici.** Uno per lato, nell'ordine dei lati: il punto da cui il lato
     parte, con bulge 0 per una linea e, per un arco, il bulge di `toPolyline`
     (`tan(sweep / 4)`, positivo) se il lato va in senso antiorario, lo stesso
     cambiato di segno se va in senso orario. La fine di un lato non è un
     vertice: il segmento arriva all'inizio del lato dopo (del primo, per
     l'ultimo), anche se i due punti non coincidono.
   - **Un lato solo.** Un arco di giro intero (`geometry().sweep` uguale a
     `kTwoPi`: è il `circle=` del rapporto) diventa la polilinea chiusa di
     **due mezzi giri** dal punto dell'angolo iniziale: quel punto (il primo
     vertice di `toPolyline`) con bulge 1, poi il punto opposto,
     `2 · centro − primo`, con bulge 1 ([DECISIONI.md](../../DECISIONI.md),
     2026-11-16). Un altro lato solo diventa la polilinea chiusa dei suoi due
     estremi: quello da cui parte, con il bulge del lato, e l'altro con bulge 0.
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
7. **Le domande `bbox` e `snaps`** (`script/src/question_table.cpp`, file
   comune; [DECISIONI.md](../../DECISIONI.md), 2026-11-16): oggi passano per le
   funzioni dell'entità senza documento, che per un retino non rispondono
   (`core::boundingBox(const Entity&)` è vuoto e
   `core::appendSnapPoints(const Entity&, …)` lo esclude con un'asserzione,
   PT6). Ogni entità con un nome è nel documento (`entityArgument`): `bbox`
   risponde `core::extent(document, record)` e `snaps` usa
   `core::appendSnapPoints(document, record, modes, points, std::nullopt)`,
   con il record dell'entità. Per i cinque tipi di prima della 0.8 le
   risposte non cambiano, perché le due funzioni del documento rimandano a
   quelle dell'entità (`generated.hpp`); per i tipi nuovi `bbox` diventa
   l'ingombro della loro geometria, come già si aspettano i casi di `MTEXT`,
   `INSERT` e `DIMENSION`. La forma delle risposte e il catalogo
   ([verifica/domande.md](../../verifica/domande.md)) non cambiano.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- File comuni: `script/src/question_table.cpp` (solo le due domande del
  punto 7), `tests/CMakeLists.txt`.

## Test richiesti

- **RT30**, usando soltanto il formato e le domande già pubblicati: contorni;
  regione piena di un rettangolo con un cerchio dentro (due anelli);
  selezione (un clic nel pieno prende il retino, uno al centro di un foro il
  cui bordo è più lontano dell'apertura del clic no); nessuno snap e nessun
  grip; ingombro dei contorni; trasformazione con riflessione dei campi
  esposti dal formato attuale. RT30 non aggiunge formati, domande o
  dipendenze e non verifica le famiglie `PatternLine` né il generatore dei
  motivi predefiniti, che il formato attuale non espone;
- RT30, in particolare, con le decisioni del 2026-11-16
  ([DECISIONI.md](../../DECISIONI.md)): `? generated H` di un rettangolo con
  un cerchio (`loop=edges circle=5,4;2`) risponde
  `REGION loop=polyline vertex=0,0 vertex=10,0 vertex=10,8 vertex=0,8 loop=polyline vertex=7,4 bulge=1 vertex=3,4 bulge=1`
  (punto 3); `? bbox H` e `? bbox P` rispondono `0,0 10,8`, e le due
  `? snaps H` rispondono `none` senza arrestare la verifica in Debug
  (punto 7);
- **test unitari obbligatori in `hatch_test.cpp`**: tutti i comportamenti
  elencati per RT30; un motivo con una famiglia a 30° ruotato di 90° ha la
  famiglia a 120°;
- **test unitari obbligatori in `hatch_test.cpp`** per `supported` e
  `isClosed` (punto 1):
  - `supported` falso per un contorno a polilinea non chiusa, per una
    polilinea chiusa senza vertici e per un contorno a lati senza lati;
  - il contorno a lati `line=0,0;10,0 line=10,0;10,8 line=10,8;0,8
    line=0,8;0.0000005,0` (scarto 5e-7): `supported` vero;
    `isClosed` vero con `ToleranceRule{4}` (millimetri, epsilon 1e-6) e
    falso con `ToleranceRule{6}` (metri, epsilon 1e-9); lo stesso contorno
    scalato di 100 con `transformedOf`: il retino
    (scarto 5e-5) resta `supported`, e `isClosed` con `ToleranceRule{4}` è
    falso;
  - `line=0,0;10,0 line=10,0;10,8` (aperto di molto): `supported` vero,
    `isClosed` falso; la sua polilinea (punto 3) ha i vertici `0,0` e `10,0`,
    con bulge 0;
  - un cerchio `circle=5,4;2` da solo: `isClosed` vero con `ToleranceRule{4}`
    e `ToleranceRule{6}`;
- **test unitari obbligatori in `hatch_test.cpp`** per la polilinea di un
  contorno a lati (punto 3):
  - il cerchio `circle=5,4;2`: `vertex=7,4 bulge=1 vertex=3,4 bulge=1`;
  - lo scarto di 5e-7 qui sopra: i vertici `0,0`, `10,0`, `10,8`, `0,8`,
    tutti con bulge 0 (la fine `0.0000005,0` dell'ultimo lato non entra);
  - `line=0,0;10,0 arc=10,4;4;270;90 line=10,8;0,8 line=0,8;0,0`:
    `0,0`, `10,0` con bulge 1, `10,8`, `0,8`; riflesso con `MIRROR` sull'asse
    x (l'arco diventa `arc=10,-4;4;270;90`, che il contorno percorre in senso
    orario): `0,0`, `10,0` con bulge −1, `10,-8`, `0,-8`;
  - due mezzi cerchi `arc=0,0;1;0;180 arc=0,0;1;180;0` (estremi coincidenti
    a due a due: vale il verso scritto): `1,0` con bulge 1, `-1,0` con
    bulge 1;
  - un arco solo `arc=0,0;1;0;90`: `1,0` con bulge `tan(π/8)`, `0,1` con
    bulge 0;
- **test unitari obbligatori in `hatch_test.cpp`** per tutte le righe dei
  motivi predefiniti (punto 2), con una tolleranza assoluta di
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

- I casi di RT30 sui comportamenti osservabili dal formato attuale passano;
  tutti i test unitari obbligatori delle famiglie `PatternLine` e dei motivi
  predefiniti passano; la CI è verde.
