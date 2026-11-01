# [0.7-P25] Revisione delle interfacce delle schede P15–P23

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #464
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è una revisione di documenti, non cambia il
  comportamento del programma
- **Dipende da:** —
- **Modulo:** nessuno: il lavoro sta nella issue, non in una pull request
- **Da leggere:** la lista di controllo di
  [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  punti 1–11; le schede [P15](P15-snap-intersezione.md)–[P23](P23-join.md);
  [ADR 0031](../../adr/0031-modifiche-alle-entita.md),
  [0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md),
  [0035](../../adr/0035-taglio-offset-raccordi-unione.md) e
  [0036](../../adr/0036-tolleranze-di-tecnigrafo.md) nei punti che le schede
  citano; le intestazioni di `core` già unite che le schede usano
  (`intersections.hpp`, `tolerance.hpp`, `core/src/solid_space_bridge.hpp` di
  P14); i commenti delle intestazioni di solid-space che le schede citano, al
  commit del sottomodulo in `main`

## Perché

Le schede della seconda metà della tappa sono state scritte il 2026-10-29 sul
contratto di solid-space, prima che esistesse la revisione con la lista, e
nessuno le ha controllate. P15 si è fermato il 2026-11-01 (#359) su un buco che
non sta nel contratto di solid-space ma nella scheda: `intersect` e `cutsAlong`
decidono esattamente quali tagli sono distinti e in che ordine stanno, ma
`partOf` riceve solo punti e stazioni arrotondati, e due tagli a meno di un ulp
arrivano uguali. È il punto 11 della lista applicato a un'interfaccia di
Tecnigrafo. Questo pacchetto cerca i buchi dello stesso tipo nelle schede
rimaste, prima che i loro pacchetti partano (decisione del 2026-11-01,
[DECISIONI.md](../../DECISIONI.md)).

## Che cosa si rivede

In quest'ordine, che è quello in cui le interfacce si usano l'una con l'altra:

| Scheda | Interfacce nuove in `core` | Come usa le altre |
|---|---|---|
| [P15](P15-snap-intersezione.md) | `curve_parts.hpp` (`CurvePoint`, `partOf`); lo snap a intersezione in `findSnap` | `intersect`, `selfIntersections`, `cutsAlong` (P14) |
| [P16](P16-trim-extend.md) | — (`cmd`) | `cutsAlong`, `partOf`, la regola delle tolleranze |
| [P17](P17-curva-parallela.md) | `offset.hpp` | `offsetCurve`, `offsetPolyline` di solid-space, la traduzione di P14 |
| [P18](P18-offset.md) | — (`cmd`) | `offset.hpp` (P17) |
| [P19](P19-raccordi-core.md) | `fillet.hpp` (raccordo, tangenze, snap a tangente) | `tangency.hpp` di solid-space, `partOf` (P15) |
| [P20](P20-fillet.md) | — (`cmd`) | `fillet.hpp` (P19) |
| [P21](P21-chamfer.md) | `chamfer.hpp` | `intersect` (P14), `partOf` (P15), `CornerResult` (P19) |
| [P22](P22-unione-core.md) | `join.hpp` | `coincidence.hpp` di solid-space, la regola delle tolleranze |
| [P23](P23-join.md) | — (`cmd`) | `join.hpp` (P22) |

Comincia da `curve_parts.hpp` di P15. Il buco della #359 è già aperto e lo
decide il responsabile: citalo e non ridiscuterlo. Se la decisione è già unita
quando cominci, rivedi `partOf` sul testo dopo la correzione: una correzione
chiude un buco, ma può aprirne uno accanto (punti 7, 10 e 11).

## Come

Per ogni tipo e ogni funzione delle schede, e per ogni passaggio di un
risultato da una funzione all'altra, ogni punto della lista: è **determinato**
dall'intestazione, dall'ADR o dalla scheda (e allora citi dove), oppure è un
**buco**. Per le schede di `cmd` contano soprattutto i punti 4, 5, 8, 10 e 11:
come il comando usa i risultati di `core` e che cosa ne mostra all'utente.

Insisti sui punti 10 e 11, i due che hanno fermato P15 e R19 di solid-space:

- **10.** Dove una scheda arrotonda o sceglie un valore entro un errore (un
  punto, un angolo, un bulge, una stazione), la scelta può cambiare un esito,
  un ordine, un'uguaglianza? Provalo con un esempio al limite e due scelte
  lecite.
- **11.** Ciò che solid-space o una funzione di `core` decide esattamente
  (quale taglio è quale, l'ordine, il lato, l'appartenenza a un pezzo) arriva a
  chi lo usa, o chi lo usa dovrebbe ricostruirlo da valori arrotondati?

**Che cosa non si rivede.** Il contratto di solid-space in sé: un punto che
solo un'intestazione di solid-space determina si cita. Le funzioni di
`offset.hpp` e `tangency.hpp` sono in revisione in solid-space
(Matleyx78/solid-space#133): di loro rivedi solo come le schede di P17–P21 le
usano e ne passano i risultati, non il contratto, per questo quella issue non
blocca questo pacchetto. Se trovi un punto che manca anche nell'intestazione di
solid-space, è un buco nel contratto: scrivi nella issue il testo della issue
`contratto` come dice AGENTS.md ("Se serve qualcosa a solid-space").

## Il verbale

Tutto nella issue #464, niente file:

- un commento **per ogni buco**, intitolato in grassetto
  `<scheda>: <intestazione>: <funzione>: <punto>` (per esempio
  `P15: curve_parts.hpp: partOf: 11, ordine dei tagli`), con che cosa manca,
  un esempio con i dati, le scelte possibili e quella che consigli, con il
  motivo e con riferimenti assoluti;
- alla fine un commento che comincia con **Verbale della revisione delle
  interfacce**: per ogni scheda e ogni funzione la lista spuntata, con il
  punto della scheda o dell'intestazione che determina ciascun punto, e i
  collegamenti ai commenti dei buchi.

Non correggere le schede, non aprire una pull request e non scrivere in
solid-space.

## Regole della tappa che valgono qui

- La regola "Quando fermarsi" di AGENTS.md sulla CI non si applica: non c'è una
  pull request. Il pacchetto finisce con il verbale; poi togli
  `AA-Presa in carico` e termina la sessione.
- Dopo il verbale decide il responsabile, buco per buco, in
  [DECISIONI.md](../../DECISIONI.md). Le decisioni le applica una pull request
  scritta con lui (schede, DECISIONI.md, "Modifiche al piano"), che chiude
  questa issue; prima del merge la rileggi con la lista sulle sole interfacce
  corrette, con un verbale nella pull request, se il responsabile te lo chiede.
  Senza buchi, la issue la chiude il responsabile dopo il verbale.
- P15–P23 dipendono da questo pacchetto: partono quando la issue è chiusa.

## Fatto quando

- Nella issue #464 c'è il verbale, con la lista spuntata per ogni funzione e
  ogni passaggio di risultati delle schede P15–P23.
- Ogni buco ha il suo commento, con esempio, scelte e consiglio.
- Nessun file del repository è cambiato.
