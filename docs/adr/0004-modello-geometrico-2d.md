# ADR 0004: Modello geometrico 2D

- **Stato:** Accettato
- **Data:** 2026-10-15

## Contesto

Il CAD 2D deve gestire centinaia di migliaia di entità con precisione e leggere e
scrivere il formato DXF senza perdite.

## Decisione

**Coordinate.** `double` in unità di disegno. X verso destra, Y verso l'alto,
angoli in radianti in senso antiorario a partire da +X, come in AutoCAD e nel DXF.

**Polilinee.** Stesso modello dell'entità DXF `LWPOLYLINE`: una lista di vertici,
ciascuno con un *bulge* che descrive il segmento che parte da quel vertice:

```
bulge = tan(θ / 4)
```

θ è l'angolo al centro dell'arco: 0 indica un segmento retto, un valore positivo un
arco in senso antiorario, un valore negativo un arco in senso orario; 1 è una
semicirconferenza. Dati gli estremi `P0`, `P1`, la corda `c = |P1 − P0|` e la normale
sinistra unitaria `n`:

```
sagitta (con segno) s = bulge · c / 2
raggio  (con segno) r = c · (bulge² + 1) / (4 · bulge)
centro              C = (P0 + P1) / 2 + n · (r − s)
```

**Entità.** Le entità sono tipi valore raccolti in `std::variant` (`tg::core::Entity`).
Aggiungere un tipo al variant obbliga, in fase di compilazione, ogni `std::visit` a
gestirlo: nessuna entità può essere dimenticata da un algoritmo.

**Identità.** Ogni entità ha un `EntityId` a 64 bit, unico nel documento e mai
riutilizzato. È la base per selezione, annullamento e riferimenti futuri.

**Visualizzazione.** Gli archi vengono approssimati da segmenti solo al momento
del disegno, con una tolleranza in pixel: la geometria memorizzata resta esatta.

## Alternative considerate

- **`float`:** dimezza la memoria ma perde precisione su coordinate grandi
  (planimetrie in millimetri, coordinate geografiche).
- **Gerarchia di classi con metodi virtuali:** più familiare, ma un nuovo tipo di
  entità può essere dimenticato silenziosamente, e ogni entità richiede
  un'allocazione separata.
- **Archi come entità separate dentro le polilinee:** complica la conversione DXF.

## Conseguenze

- Import ed export di `LWPOLYLINE` saranno diretti.
- Quando le entità saranno molte, il `std::variant` andrà affiancato da indici
  spaziali; la decisione andrà presa con un nuovo ADR.
