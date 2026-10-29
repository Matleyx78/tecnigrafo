# ADR 0033: Nuovi snap e tracciamento polare

- **Stato:** Accettato
- **Data:** 2026-10-29
- **Estende:** ADR 0008 (selezione e snap), ADR 0014 (distanza scritta da sola)

## Contesto

La tappa 0.3 ha portato gli snap a estremo, medio e centro, e la 0.4 quello a
inserimento. Le schede delle tappe 0.3 e 0.4 rimandano alla 0.7 gli snap a
intersezione, perpendicolare, tangente e quadrante, e il tracciamento polare.
L'ADR 0012 (conseguenze) chiede che quadrante, tangente e perpendicolare stiano
in un punto solo del codice.

Due di questi snap prendono decisioni geometriche vicine allo zero: se due curve
si intersecano e se un punto sta fuori da un cerchio. Secondo l'ADR 0030 queste
decisioni spettano a solid-space, che le ha accettate con le richieste
[Matleyx78/solid-space#20](https://github.com/Matleyx78/solid-space/issues/20)
(intersezioni, [ADR 0010 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0010-posizione-e-intersezioni-nel-piano.md))
e [Matleyx78/solid-space#21](https://github.com/Matleyx78/solid-space/issues/21)
(tangenti da un punto, [ADR 0011 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0011-raccordi-e-tangenze-nel-piano.md)).
La proiezione di un punto su una curva, che serve alla perpendicolare, resta a
Tecnigrafo (ADR 0010 di solid-space, punto 9).

## Decisione

### Snap

1. **Quattro tipi nuovi:** `Intersection`, `Perpendicular`, `Tangent`,
   `Quadrant`, con i marcatori di AutoCAD:
   - intersezione: una croce a X;
   - perpendicolare: il simbolo ⊥;
   - tangente: un cerchio con una linea tangente in alto;
   - quadrante: un rombo.

   Si attivano uno per uno come quelli di oggi (`SnapModes`).
2. **Quadrante.** I punti a 0°, 90°, 180° e 270°, presi sugli assi del disegno,
   di cerchi, archi e archi delle polilinee. Per un arco valgono solo i punti che
   cadono dentro l'arco (`containsAngle`).
3. **Intersezione.** I punti in cui si incontrano due curve, anche due segmenti
   della stessa polilinea, purché entrambe passino entro l'apertura del
   puntatore. Restano fuori le intersezioni apparenti e quelle dei prolungamenti
   (*Extended Intersection*). Le calcola la facciata delle intersezioni di `core`
   sopra `solid_space::geom2d` (`intersect`, `selfIntersections`), e un punto in
   cui si incontrano più curve è uno solo.
4. **Perpendicolare e tangente** partono dal punto base del comando
   (`Command::basePoint()`):
   - **perpendicolare:** il piede della perpendicolare dal punto base alla curva,
     solo se cade sulla curva e non sul suo prolungamento;
   - **tangente:** i punti di tangenza dal punto base a cerchi, archi e archi
     delle polilinee, solo se cadono sulla curva; li dà `tangentsFrom` di
     solid-space, e un punto base esattamente sul cerchio dà il punto stesso.

   Senza punto base non danno nulla. Gli snap "rimandati" di AutoCAD, che si
   risolvono dopo il secondo punto, restano fuori.
5. **Priorità** (estende l'ADR 0008, punto 7):
   - tutti i punti candidati entro l'apertura (estremo, medio, intersezione,
     quadrante, perpendicolare, tangente) competono, e vince il più vicino al
     puntatore;
   - a pari distanza vale l'ordine estremo, intersezione, medio, quadrante,
     perpendicolare, tangente;
   - se nessuno di loro è entro l'apertura, valgono centro e inserimento
     dell'entità su cui passa il puntatore, come oggi.
6. **Modi accesi per difetto:** quelli di oggi più l'intersezione, come in
   AutoCAD; perpendicolare, tangente e quadrante restano spenti.
   - Le domande `snaps` e `snap` senza `modes=` restano con i quattro modi di
     oggi, così i casi esistenti non cambiano.
   - Lo snap a intersezione acceso per difetto può cambiare gli script esistenti
     che fanno clic vicino a un incrocio. Il pacchetto di riferimenti che lo
     verifica li rivede e, dove serve, aggiunge `!osnap` con i modi di prima.
7. **Script:** `!osnap <modi>` imposta i modi, con le abbreviazioni di AutoCAD
   (`end`, `mid`, `cen`, `ins`, `int`, `per`, `tan`, `qua`) separate da virgole,
   oppure `none`. Come `!snap`, non scrive nulla nella cronologia.

### Tracciamento polare

8. **Angoli.** Con un punto base, il puntatore si aggancia alle direzioni
   multiple di `POLARANG`, misurate dall'asse X del disegno. `POLARANG` vale 90°
   per difetto. Nel programma si sceglie tra 90, 45, 30, 22.5, 18, 15, 10 e 5,
   come in AutoCAD; il comando `POLARANG` accetta qualunque valore maggiore di 0
   e al più di 180.
9. **Aggancio.** Se il puntatore dista non più di 10 pixel (l'apertura dello
   snap) da una delle semirette polari uscenti dal punto base, il punto si
   proietta su quella semiretta; tra due semirette vince la più vicina.
   - Lo snap a oggetto vince sul polare, come vince sull'orto.
   - Orto e polare si escludono: accendere l'uno spegne l'altro, come in AutoCAD
     (`F8`, `F10`), con i messaggi `<Ortho on>`, `<Polar on>` e `<Polar off>`.
   - Una distanza scritta da sola (ADR 0014) segue la semiretta polare su cui sta
     il puntatore.
10. **Fuori:** angoli aggiuntivi, angoli relativi all'ultimo segmento e
    tracciamento degli snap (*Object Snap Tracking*). Si valutano dopo la 0.7.
11. **Script:** `!polar on` e `!polar off`, che non scrivono nella cronologia;
    al livello 2 anche `F10` con `!key`.

### Interfaccia

12. Il programma mostra:
    - i quattro modi nuovi tra le azioni degli snap;
    - il polare sulla barra di stato e con `F10`;
    - mentre il polare aggancia, la semiretta tratteggiata e il suggerimento
      `Polar: <distanza> < <angolo>°`.

    Sono parti dell'interfaccia, provate con i test Qt e la prova manuale.

## Alternative considerate

- **Perpendicolare e tangente come snap "di entità", insieme al centro:** in
  AutoCAD il marcatore compare passando sopra la curva anche lontano dal piede.
  È meno prevedibile e più difficile da provare: si sceglie la regola dei punti
  entro l'apertura, uguale per tutti.
- **Intersezione spenta per difetto:** i casi esistenti non cambierebbero, ma
  il programma si comporterebbe in modo diverso da AutoCAD proprio nello snap
  più usato.
- **Polare in pixel di angolo invece che di distanza:** vicino al punto base
  aggancerebbe troppo, lontano troppo poco.

## Conseguenze

- `SnapKind` riceve quattro valori: ogni `switch` del progetto (marcatori,
  rapporto, domande) li deve trattare.
- `findSnap` riceve il punto base, facoltativo.
- `DrawingAids` riceve il polare e `POLARANG`.
- Lo snap a intersezione e quello a tangente arrivano nella seconda metà della
  tappa, dopo i pacchetti di solid-space che realizzano intersezioni e tangenti;
  quadrante, perpendicolare e polare no.
