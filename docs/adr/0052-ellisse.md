# ADR 0052: Ellisse

- **Stato:** Proposto
- **Data:** 2026-11-11

## Contesto

Un blocco inserito con scala diversa in X e in Y porta cerchi e archi in
ellissi. Il piano della 0.8 proponeva di disegnarle come spezzate e di rifiutare
`EXPLODE`; il responsabile ha deciso il 2026-11-11 di portare l'ellisse nella
0.8, «anche se è la strada più in salita» ([DECISIONI.md](../DECISIONI.md),
punto 7 del piano). Oggi anche le `ELLIPSE` dei file di AutoCAD restano dati non
gestiti, che si conservano ma non si vedono.

## Decisione

1. **L'entità.** `core::Ellipse`, l'`ELLIPSE` del DXF: centro (10), estremo
   dell'asse maggiore relativo al centro (11), rapporto fra asse minore e
   maggiore (40, in (0, 1]), parametri iniziale e finale (41, 42, in radianti;
   0 e 2π per l'ellisse intera). Il punto di parametro `t` è
   `centro + cos t · a + sin t · b`, con `a` l'asse maggiore e `b` l'asse minore
   (`a` ruotato di +90° e moltiplicato per il rapporto), come in AutoCAD.
   Estrusione `(0,0,-1)` come le altre entità (ADR 0018, punto 5); altre
   estrusioni restano non gestite. Si aggiunge al variant con lo scheletro della
   tappa ([ADR 0044](0044-entita-composte-e-geometria-generata.md), punto 1).
2. **Trasformazioni esatte.** Un'affinità porta un'ellisse in un'ellisse: la
   nuova si calcola dagli assi trasformati (scomposizione ai valori singolari
   della parte lineare per gli assi nuovi, e lo spostamento dei parametri
   iniziale e finale) senza approssimazioni. Così:
   - nella **geometria generata** (ADR 0044, punto 2), sotto un inserimento con
     scala non uniforme, cerchi, archi e archi di polilinea diventano ellissi o
     archi d'ellisse, e tutte le forme arrivano già nel sistema del disegno;
     resta un'affinità solo per le regioni piene dei retini dentro un blocco;
   - **`EXPLODE`** di un inserimento con scala non uniforme mette nel disegno
     ellissi, come AutoCAD ([ADR 0045](0045-blocchi-e-attributi.md), punto 4).
3. **Comportamento.** Si disegna come spezzata calcolata sullo schermo (come gli
   archi); snap *Centro*, *Quadrante* (gli estremi degli assi che stanno
   sull'arco) ed *Estremo* (per un arco d'ellisse); grip al centro e sugli
   estremi degli assi. `MOVE`, `COPY`, `ROTATE`, `SCALE`, `MIRROR` la
   trasformano; `ERASE` e le proprietà come le altre entità.
4. **Che cosa chiede a solid-space.** La distanza dal puntatore (scelta con un
   clic), il punto più vicino (snap *Vicino*), le intersezioni con segmenti,
   archi, cerchi ed ellissi (snap *Intersezione* e attraversamento della
   finestra) sono decisioni geometriche: una richiesta nuova, «l'ellisse in
   `geom2d`», aperta con il piano. Fino al suo contratto un'ellisse si sceglie
   solo con la finestra che la contiene tutta (per il suo ingombro) e non dà
   snap *Vicino* né *Intersezione*.
5. **Comando `ELLIPSE`** nella forma di AutoCAD: `Specify axis endpoint of
   ellipse or [Arc/Center]:`, `Specify other endpoint of axis:`, `Specify
   distance to other axis or [Rotation]:`; *Center* parte dal centro; *Arc*
   chiede poi l'angolo iniziale e finale (l'opzione *Parameter* resta fuori).
   Con `PELLIPSE` = 0 soltanto: le ellissi come polilinee (`PELLIPSE` = 1) restano
   fuori. Messaggi e richieste dalla cronologia di AutoCAD LT del responsabile.
6. **Fuori dalla 0.8:** `TRIM`, `EXTEND`, `OFFSET`, `FILLET`, `CHAMFER`,
   `BREAK` e `LENGTHEN` sulle ellissi (idea
   [#663](https://github.com/Matleyx78/tecnigrafo/issues/663)); i retini con
   lati d'ellisse (ADR 0048, punto 1); le quote su un'ellisse (AutoCAD non ne
   ha).

## Alternative considerate

- **Spezzate sullo schermo e `EXPLODE` rifiutato** (la proposta del piano): più
  semplice, ma le ellissi dei file non si vedrebbero e i blocchi stirati non si
  esploderebbero.
- **Esplodere in polilinee approssimate:** cambierebbe la geometria senza dirlo.
- **Spline** al posto dell'ellisse: Tecnigrafo non ha le spline; l'ellisse è
  esatta e più semplice.

## Conseguenze

- I tipi nuovi della 0.8 sono sette; `GeneratedItem` non ha più una
  trasformazione propria.
- Una quarta richiesta a solid-space e un pacchetto con la scheda dopo il
  contratto.
