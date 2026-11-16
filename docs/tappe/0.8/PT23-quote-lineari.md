# [0.8-PT23] `core`: quote lineari e allineate, valore e grafica

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT23]` (#…), agente codex, fascia forte
- **Dipende da:** PT5 (revisione delle interfacce), PT10, PT13, PT22, RT23
- **Modulo:** `core`
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md) per intero;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto 5;
  le schede [PT6](PT6-scheletro-core.md), [PT13](PT13-mtext-core.md),
  [PT22](PT22-stili-di-quota.md) e [PT24](PT24-quote-angolari-radiali.md)
  (sorella)

## File

- `core/include/tg/core/dimension.hpp`, `core/src/dimension.cpp`
- `core/src/dimension_text.cpp`, `core/src/dimension_linear.cpp`,
  `core/src/dimension_graphics.hpp` (nuovi, privati)
- `tests/core/dimension_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const Dimension&)` restituisce `true`. Ogni quota si mostra con la
   sua `graphics` (geometria generata = le entità della grafica, con le
   proprietà del layer `0`/*DaBlocco* come in un blocco, ADR 0045, punto 3).
2. **Variabili usate:** quelle dello stile della quota con le sovrascritture
   (PT22). `s` = `DIMSCALE`; le **grandezze grafiche** qui sotto (`DIMASZ`,
   `DIMEXO`, `DIMEXE`, `DIMGAP`, `DIMTXT`, `DIMCEN`) sono già moltiplicate per
   `s`; la misura no (con distanza 10 e `DIMSCALE` 2 il gruppo 42 è 10)
   (PT5, punto B11: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT5»).
3. **Testo** (ADR 0047, punto 3):

   ```cpp
   /// The text a dimension shows (docs/adr/0047, point 3), with %%c, %%d.
   /// measurement: drawing units for lengths, degrees for angular ones;
   /// DIMLFAC and DIMRND apply to lengths only (PT24, point 1).
   [[nodiscard]] std::string dimensionText(const DimVariables&, DimensionKind,
                                           double measurement,
                                           std::string_view textOverride);
   ```

   Lunghezze: `m · DIMLFAC`, arrotondata a un multiplo di `DIMRND` se > 0
   (metà in su, lontano da zero), scritta con `std::to_chars` fissa a `DIMDEC`
   cifre, `.` sostituito da `DIMDSEP`, poi senza zeri finali (e senza il
   separatore se resta in fondo) se `DIMZIN & 8`; `-0` si scrive `0`. `DIMPOST`:
   se contiene `<>` il valore va al suo posto, altrimenti si aggiunge in fondo.
   Testo sostituito come nell'ADR.
4. **Grafica lineare e allineata.** `p1 = point13`, `p2 = point14`,
   `d = linePoint`. Direzione `u`: lineare `(cos r, sin r)` con `r = rotation`;
   allineata `(p2 − p1)/|p2 − p1|`. `n` = `u` ruotato di +90°. Misura
   `|(p2 − p1)·u|`. Piedi `f1`, `f2` = proiezioni di `p1`, `p2` sulla retta per
   `d` con direzione `u`. **Misura zero** (PT5, punto B08): una lineare con
   `f1 = f2` si crea con testo `0`, senza i pezzi lunghi zero (ADR 0036, punto
   5), con `w = u` dove serve un verso; un'allineata con `p1 = p2` non è
   rigenerabile (motivo `Points coincide.`).
   - **Linee d'estensione** (se non soppresse): da `pᵢ + DIMEXO·eᵢ` a
     `fᵢ + DIMEXE·eᵢ`, con `eᵢ` = versore da `pᵢ` a `fᵢ`; se `pᵢ = fᵢ` nessuna.
   - **Frecce** piene chiuse: un `SOLID` triangolare con la punta in `fᵢ`,
     lunghezza `a = DIMASZ` lungo la linea, base larga `a/3`.
   - **Dentro o fuori** (ADR 0047, punto 4; `DIMATFIT` 3, `DIMTMOVE` 0), con
     `L = |f2 − f1|`, `t` = larghezza del testo (impaginazione di PT13) e
     `w` = versore da `f1` a `f2`, nell'ordine:
     1. testo e frecce dentro se ci stanno tutti e due;
     2. altrimenti il testo dentro e le frecce fuori, se ci sta il testo;
     3. altrimenti le frecce dentro e il testo fuori, se ci stanno le frecce;
     4. altrimenti tutto fuori.
     Le **soglie** di «ci sta» e la **posizione del testo fuori** non sono
     pubblicate da AutoCAD: si ricavano dai casi di RT23, presi dalle quote
     strette di `autocad-quote.dxf`. Ipotesi di partenza, da confermare con
     quei casi: le frecce ci stanno se `L ≥ 2a`; il testo se
     `L ≥ t + 2·DIMGAP`; tutti e due se `L ≥ t + 2·DIMGAP + 2a`; il testo fuori
     sta oltre la seconda linea d'estensione, sulla linea di quota prolungata,
     a `a + DIMGAP` da essa. Frecce fuori: puntate verso l'interno, con un
     tratto di linea di quota lungo `a` oltre ciascuna; con `DIMTOFL` di
     `ISO-25` la linea di quota resta anche fra le linee d'estensione. Se un
     caso di RT23 contraddice un'ipotesi, vale il caso e la pull request lo
     scrive (è il disegno di AutoCAD, la fonte decisa dal responsabile).
   - **Testo** (`MTEXT`, altezza `DIMTXT`, stile `DIMTXSTY`, valore di
     `dimensionText`): se `userTextPosition`, in `textPoint`; altrimenti al
     centro `c = (f1 + f2)/2`. Con `DIMTAD` = 1 e testo allineato (`DIMTIH`/
     `DIMTOH` 0): allineamento in basso al centro, in `c + DIMGAP·n'`, con `n'`
     la normale verso l'alto della lettura; rotazione `= angolo di u` portata
     nella lettura (fra −90° escluso e 90° compreso). Con `DIMTAD` = 0: in
     mezzo al centro, e la linea di quota si interrompe per la larghezza del
     testo più `DIMGAP` per lato. Con `DIMTIH`/`DIMTOH` 1: testo orizzontale,
     in mezzo, linea interrotta. Un **angolo esplicito** del testo (*Angle* di
     PT26 e PT27) vale sempre, per ogni tipo di quota e posizione, anche con
     `DIMTIH`/`DIMTOH` 1: lo stile decide solo l'angolo automatico; *Angle* 0°
     dà il testo orizzontale anche dove l'automatico sarebbe obliquo
     ([DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Secondo giro delle revisioni PT4 e PT5»). Quando il testo va fuori (casi 3 e 4), stesse
     regole nella sua posizione fuori. `textPoint` si aggiorna al centro del
     testo. **Posizione e angolo** (PT5, punto B10): `textPoint` è sempre il
     centro del testo (gruppo DXF 11); con `userTextPosition` il testo ha
     l'allineamento in mezzo al centro, in `textPoint`. L'angolo esplicito è
     assoluto ed è distinto dall'automatico: `std::optional<double>`, assente =
     automatico (nel DXF il gruppo 53, assente = automatico, con il
     significato che hanno i file di AutoCAD: RT25 lo verifica); una rotazione
     della quota lo aumenta dello stesso angolo (*Angle* 30°, poi `ROTATE`
     90°: 120°).
   - **Soppressioni** (PT5, punto B11): `DIMSD1` toglie la metà della linea di
     quota dal lato di `f1`, fino al centro o al testo interrotto, con la
     freccia in `f1` e, con le frecce fuori, il tratto esterno da quel lato;
     `DIMSD2` lo stesso dal lato di `f2`. L'adattamento si sceglie prima, con
     i quattro casi e due frecce; le soppressioni si applicano dopo, senza
     cambiarlo ([DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Secondo giro delle revisioni PT4 e PT5»).
   - Colori e spessori: `DIMCLRD`/`DIMLWD` per linea di quota e frecce,
     `DIMCLRE`/`DIMLWE` per le linee d'estensione, `DIMCLRT` per il testo.
5. **Creazione e rigenerazione:**

   ```cpp
   /// Measurement, text and graphics from definition and presentation.
   /// nullopt (with the reason) when not regenerable (docs/adr/0047, point 4).
   [[nodiscard]] std::optional<Dimension> regenerated(const Document&, Dimension,
                                                      std::string* reason = nullptr);
   ```

   I record della grafica nuova hanno handle 0 (li assegna l'editor) e nessun
   `extras`; la grafica vecchia sparisce.
6. **Trasformazioni** (`transformedOf`): traslazione → punti e grafica
   traslati, senza rigenerare; altrimenti punti trasformati (rotazione della
   lineare più l'angolo della similitudine; con una riflessione anche il testo
   resta leggibile), poi `regenerated`; se non è rigenerabile, rifiuto con
   `Dimension cannot be regenerated by Tecnigrafo.` (PT6, punto 3). Lo stesso
   messaggio per un grip di definizione che non si può applicare (PT7, punto
   2; PT5, punto B12).
7. **Grip:** `point13`, `point14`, `linePoint`, `textPoint`; il grip del testo
   sposta solo il testo (`userTextPosition`) e la grafica del testo senza
   rigenerare il resto; gli altri rigenerano. **Snap:** quelli della grafica.
8. I tipi angolare, radiale, diametrale li fa PT24; fino ad allora
   `regenerated` per loro dà «non rigenerabile», e il tipo *altro* sempre.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`; distanze con `std::sqrt`.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `dimension_test.cpp`: `dimensionText` con `DIMDEC` 2 e 3, `DIMZIN` 8 e 0,
  `DIMRND`, `DIMLFAC`, `DIMPOST` con `<>`, testo sostituito; grafica di una
  lineare orizzontale larga e di una stretta (frecce fuori); `DIMTAD` 0 e 1;
  allineata a 30°; rotazione, scala e specchiatura con rigenerazione; quota con
  `DIMTOL` 1 rifiutata; `DIMATFIT` 1 non rigenerabile; i quattro casi
  dell'adattamento; spostamento senza rigenerazione; lineare orizzontale con
  `p1=(0,0)`, `p2=(0,10)`, `d=(0,20)`: testo `0` e nessun pezzo lungo zero;
  allineata con `p1=p2` non rigenerabile; `DIMSCALE` 2 con distanza 10:
  misura 10, frecce lunghe `2·DIMASZ`; `DIMSD1` 1 con due piedi `(0,0)` e
  `(20,0)`: nessuna freccia in `(0,0)` e nessuna linea fra `(0,0)` e il
  centro; *Angle* 30° poi `ROTATE` 90°: angolo 120°; `textPoint` trascinato in
  `(15,8)` con `DIMTAD` 1: centro del testo in `(15,8)`; *Angle* 30° con
  `DIMTIH` 1: testo a 30°; *Angle* 0° su un'allineata a 45°: testo a 0°;
  `L` = 8, freccia 2, testo largo 6 margini compresi, `DIMSD1` 1: frecce fuori
  come con due frecce (caso 2), quella in `f1` soppressa, e lo stesso con
  `DIMSD2`; `dimensionText` di un angolo retto con `DIMLFAC` 2 e `DIMRND` 4:
  `90%%d`; un grip di `point13` su una quota con `DIMTOL` 1 lascia la quota
  ferma con il rifiuto.

## Fatto quando

- I casi di RT23 passano; la CI è verde.
