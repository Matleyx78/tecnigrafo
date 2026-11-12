# [0.8-PT40] Revisione delle interfacce: schede dopo il contratto

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è una revisione di documenti, non cambia il
  comportamento del programma
- **Dipende da:** —
- **Modulo:** nessuno: il lavoro sta nella issue, non in una pull request
- **Da leggere:** la lista di controllo di
  [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  punti 1–11; [ADR 0048](../../adr/0048-retini.md) e
  [ADR 0052](../../adr/0052-ellisse.md) per intero;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punti 2 e
  8; le schede qui sotto, con le schede [PT30](PT30-retini-core.md) e
  [PT48](PT48-ellisse.md) su cui crescono; il codice che toccano, con le
  intestazioni di solid-space che citano, al commit del contratto (sotto)

## Perché

Le schede qui sotto sono scritte sul contratto di solid-space rivisto in
[`[0.3-CS]` #222](https://github.com/Matleyx78/solid-space/issues/222) e
introducono funzioni nuove in `core`; `-HATCH` (PT44) le mette in fila. Si
controllano prima che i loro pacchetti partano
([tappe/0000-modello.md](../0000-modello.md), sezioni 5 e 6). I loro pacchetti
sono in fascia media: le decisioni devono essere complete prima.

## Il contratto

Le intestazioni accettate e riviste, al commit
[`fd69e80`](https://github.com/Matleyx78/solid-space/commit/fd69e807d4ebafe12f73f46ba26072a46ff72809)
di `main` di solid-space (2026-11-12, #231): `robust/elementary.hpp`,
`geom2d/region.hpp`, `geom2d/boundary.hpp`, `geom2d/clipping.hpp`,
`geom2d/ellipse.hpp`. Il contratto non si rivede qui: è già passato da
`[0.3-CS]`. Se una scheda chiede al contratto una risposta che non dà, è un
buco nel contratto: lo scrivi come dice
[contratto.md](../../agenti/contratto.md), «Un buco nel contratto», senza
sceglierlo.

## Che cosa si rivede

In quest'ordine, che è quello in cui le interfacce si usano l'una con l'altra:

| Scheda | Interfacce nuove | Come usa le altre |
|---|---|---|
| [PT41](PT41-trigonometria-2.md) | `tan`, `acos`, `hypot` di `tg/core/math.hpp`; `Vec2::length` | `elementary.hpp`; la riduzione degli angoli di PT1 |
| [PT42](PT42-righe-dei-motivi.md) | `closedPolylineOf` (ponte), `regionLoops`, `PatternLines`, `patternLines`, `kMaxHatchLines`; la geometria generata dei retini | `clipping.hpp`, `region.hpp`; PT30 (il modello, i motivi, la polilinea di un contorno a lati) |
| [PT43](PT43-contorni-dei-retini.md) | `closedPolylineOf` (ponte), `detectHatchBoundary`, `DetectedBoundary`, `chosenHatchContours`, `ChosenContours`; i segnali dei contorni; le domande `hatch-boundary` e `hatch-contours` | `boundary.hpp`, `region.hpp`; `ToleranceRule`; i segnali che legge PT42 |
| [PT44](PT44-comando-hatch.md) | `-HATCH`, `HPISLANDDETECTION`, i valori della sessione | PT42, PT43, i motivi di PT30 |
| [PT50](PT50-ellisse-selezione-snap.md) | `ellipseOf` (ponte), `distance`, `closestPoint`, `intersects` dell'ellisse; l'ellisse in `intersect` | `ellipse.hpp`; PT48; `findSnap`, `pickEntity`, `selectInRectangle` |

## Come

Per ogni tipo e ogni funzione delle schede, e per ogni passaggio da una
all'altra, ogni punto della lista: è **determinato** dal contratto, dagli ADR,
dalla scheda o dal codice di oggi (e allora citi dove), oppure è un **buco**.
Insisti su:

- i punti 10 e 11 (scelte libere, che cosa arriva al chiamante): che cosa
  Tecnigrafo fa dei valori arrotondati di solid-space (gli estremi dei tratti
  in PT42, i vertici dei contorni in PT43 e PT44, i punti e le stazioni
  dell'ellisse in PT50) e se ne ricava decisioni (#359); la fascia di
  `nearest` e quella degli archi di `detectBoundary`;
- i segnali dei contorni: PT43 li scrive, PT42 li legge per lo stile delle
  isole, PT44 li mette nel retino; un retino letto da un file che non li ha;
- le regole al limite: passo zero o non finito, righe oltre `kMaxHatchLines`,
  un tratto più corto dei suoi errori, un punto del tratteggio su un estremo,
  un'area indicata due volte, un contorno scelto che si attraversa, un'ellisse
  ridotta a un punto o oltre il dominio, un vertice di polilinea sull'ellisse;
- che cosa vede chi chiama `intersect` con un'ellisse come `B` (l'ordine).

Non rivedi lo stile né le scelte che gli ADR e le decisioni del 2026-11-12
([DECISIONI.md](../../DECISIONI.md)) hanno già preso: se una regola esistente
risponde, la citi e vai avanti ([revisione.md](../../agenti/revisione.md),
«Che cosa blocca una revisione»).

## Il verbale

Nella issue, scheda per scheda: i buchi, ciascuno con un esempio e le scelte
possibili, senza scegliere; poi i punti determinati, in una riga. I buchi li
decide il responsabile in [DECISIONI.md](../../DECISIONI.md), e una pull request
li applica alle schede e chiude la issue; senza buchi, la chiude il responsabile
dopo il verbale. Il verbale dice strumento, modello e impegno.

## Fatto quando

- Il verbale è nella issue e copre tutte le schede della tabella.
