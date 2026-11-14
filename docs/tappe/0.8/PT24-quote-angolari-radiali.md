# [0.8-PT24] `core`: quote angolari, radiali e diametrali

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT24]` (#…), agente codex, fascia forte
- **Dipende da:** PT5 (revisione delle interfacce), PT23, RT24
- **Modulo:** `core`
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md);
  [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md) e
  `core/include/tg/core/intersections.hpp` (l'intersezione di due rette da
  solid-space); la scheda [PT23](PT23-quote-lineari.md) (sorella: testo,
  frecce, rigenerazione)

## File

- `core/src/dimension_angular.cpp`, `core/src/dimension_radial.cpp` (nuovi)
- `core/src/dimension.cpp` (la scelta del tipo in `regenerated`)
- `tests/core/dimension_angular_test.cpp` (nuovo), `tests/CMakeLists.txt`
  (file comune)

## Cosa fare

Le misure sono già per `DIMSCALE`; frecce, colori, spessori, testo,
adattamento quando testo e frecce non ci stanno (i quattro casi di PT23,
misurati lungo l'arco per l'angolare) e rigenerazione come PT23. Per le radiali
e diametrali in un cerchio piccolo valgono i casi di RT24, presi da
`autocad-quote.dxf`, con la stessa regola: dove contraddicono la scheda vale il
caso.

1. **Angolare fra due linee.** Prima linea `point13 → point14`, seconda
   `linePoint → point15`, punto dell'arco `point16`. Vertice `v` = intersezione
   delle due rette (da solid-space, `intersections.hpp`); rette parallele: non
   rigenerabile. Le quattro semirette da `v` lungo le due rette dividono il
   piano in quattro angoli; la quota misura quello che contiene `point16`
   (sulla bisettrice esatta vale il primo in senso antiorario dalla prima
   semiretta). Un `point16` su una delle quattro semirette appartiene
   all'angolo che viene dopo di essa in senso antiorario; `point16 = v` non è
   rigenerabile; rette coincidenti come le parallele, `Lines are parallel.`
   (PT5, punto B13: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT5»). Raggio dell'arco `R = |point16 − v|`; arco di quota da una
   semiretta all'altra. Linee d'estensione: solo se l'arco sta oltre la linea,
   dall'estremo della linea più vicino all'arco (più `DIMEXO`) fino all'arco più
   `DIMEXE`, lungo la semiretta. Frecce tangenti all'arco nei suoi estremi
   secondo i quattro casi dell'adattamento di PT23, con le misure (`L`, il
   testo, le frecce) prese lungo l'arco (PT5, punto B15). Testo al punto
   medio dell'arco, sopra di `DIMGAP` se `DIMTAD` = 1, ruotato come la tangente
   portata nella lettura; valore in gradi con `DIMADEC` (−1 = `DIMDEC`),
   `DIMAZIN`, e `%%d` in fondo; `DIMLFAC` e `DIMRND` non si applicano agli
   angoli, come AutoCAD (`dimensionText` lo dice nel suo commento: ingresso in
   gradi per l'angolare, in unità del disegno per le lunghezze).
2. **Radiale.** Centro `c = linePoint`, punto sulla curva `q = point15`,
   `R = |q − c|`; posizione del testo `t = textPoint`. Se `t` è fuori dal
   cerchio: linea da `q` a `t`, freccia in `q` puntata verso `c` dall'esterno;
   se è dentro: linea da `c` a `q`, freccia in `q` puntata verso l'esterno.
   Testo in `t`, prefisso `R`, allineato alla linea (`DIMTOH`/`DIMTIH` 0) o
   orizzontale. Un `t` esattamente sulla curva conta come dentro; un `t` nel
   centro non è rigenerabile. Segno del centro: se `DIMCEN > 0`, due segmenti
   lunghi `2·DIMCEN` a croce sul centro; se `< 0`, la croce lunga `2·|DIMCEN|`
   più le linee d'asse, staccate dalla croce di `|DIMCEN|` e che finiscono
   `|DIMCEN|` oltre il cerchio, come AutoCAD; se 0, niente (PT5, punto B16).
3. **Diametrale.** Punti opposti `point15` e `linePoint`, centro nel mezzo.
   Testo dentro: linea da un punto all'altro con due frecce verso l'esterno;
   testo fuori: come la radiale dal punto più vicino al testo; a pari
   distanza `point15`. Prefisso `%%c`.
   Segno del centro come la radiale.
4. **Trasformazioni e grip** come PT23: i punti si trasformano e la quota si
   rigenera; grip su tutti i punti di definizione e sul testo.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`; angoli con `core::atan2`.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `dimension_angular_test.cpp`: angolare a 90° e a 30° con `point16` in ognuno
  dei quattro angoli; linee che non arrivano all'arco (linee d'estensione);
  linee parallele (non rigenerabile); radiale con il testo fuori e dentro;
  diametrale; `DIMCEN` positivo, negativo e 0; assi X e Y con `point16=(5,0)`
  (l'angolo che viene dopo in senso antiorario, cioè il primo quadrante) e
  con `point16=(0,0)` (non rigenerabile); rette coincidenti (non
  rigenerabile); arco lungo 8 con frecce da 2 e testo largo 7 margini compresi
  (testo dentro e frecce fuori, il caso 2 di PT23); angolo retto con `DIMLFAC` 2 e `DIMRND` 4: `90%%d`;
  radiale di raggio 5 con il testo in `(5,0)` (dentro) e nel centro (non
  rigenerabile); diametrale `(±5,0)` con il testo in `(0,6)` (dal lato di
  `point15`); `DIMCEN` −2 su raggio 5: croce da −2 a 2, assi da 4 a 7 per
  lato.

## Fatto quando

- I casi di RT24 passano; la CI è verde.
