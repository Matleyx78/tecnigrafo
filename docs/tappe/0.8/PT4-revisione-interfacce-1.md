# [0.8-PT4] Revisione delle interfacce: scheletro, tabelle, `SOLID`, blocchi

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
  punti 1–11; [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md) e [ADR 0045](../../adr/0045-blocchi-e-attributi.md) per intero; [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md), punto 1; le schede qui sotto; il codice che toccano, con le
  intestazioni che citano

## Perché

Le schede qui sotto introducono tipi e funzioni nuove in `core` (o, per PT7 e PT8, `edit` e `render`,
interfacce su cui crescono gli altri pacchetti della tappa). Si controllano
prima che i loro pacchetti partano ([tappe/0000-modello.md](../0000-modello.md),
sezione 6). Lo scheletro (PT6) fissa i tipi e le funzioni che tutti i pacchetti dei tipi realizzano: un buco qui si ripete in sei pacchetti.

## Che cosa si rivede

In quest'ordine, che è quello in cui le interfacce si usano l'una con l'altra:

| Scheda | Interfacce nuove | Come usa le altre |
|---|---|---|
| [PT6](PT6-scheletro-core.md) | i sette tipi, `transformedShape`, `isSupported`, `GeneratedItem`, `generate`, `extent`, `distance`, `transformedIn`, `moveGripIn`, le tre tabelle in `Document` | le visite di oggi, `document_query`, l'indice spaziale |
| [PT7](PT7-tabelle-edit.md) | i due cambiamenti generici di `Editor`, `DocumentTables::replace…` | PT6; i comandi di trasformazione |
| [PT8](PT8-geometria-generata-render.md) | `Painter::fillLoops`; il disegno della geometria generata | PT6, punto 3 |
| [PT10](PT10-solid.md) | `Solid` e le sue funzioni; il codec DXF | PT6 |
| [PT48](PT48-ellisse.md) | `Ellipse`, le sue trasformazioni affini esatte, il codec DXF | PT6, PT8 |
| [PT11](PT11-stili-di-testo.md) | `TextStyle`, `TextStyleTable`; il codec della tabella | PT6, PT7 |
| [PT16](PT16-blocchi-core.md) | `BlockDefinition`, `BlockTable`, `Insert` e la sua geometria generata, `dependsOnBlock` | PT6, PT8 |
| [PT17](PT17-attributi-core.md) | `AttributeDefinition`, `Attribute`, gli attributi di un inserimento nel disegno | PT16 |
| [PT29](PT29-explode-blocchi-quote.md) | `explodeInsert`, `explodeDimension` | PT16, PT17, PT23 |

## Come

Per ogni tipo e ogni funzione delle schede, e per ogni passaggio da una
all'altra, ogni punto della lista: è **determinato** dagli ADR, dalla scheda o
dal codice di oggi (e allora citi dove), oppure è un **buco**. Insisti su:

- i punti 10 e 11 (scelte libere, che cosa arriva al chiamante): che cosa
  arriva a `render` e alla selezione da un inserimento con scala non uniforme;
  che cosa vede chi chiama `distance` su un'entità composta;
- la vita dei dati: chi possiede la geometria generata, gli `ATTRIB`, le entità
  di un blocco; che cosa succede a un inserimento quando la sua definizione si
  toglie o cambia, anche attraverso blocchi annidati; gli handle delle copie;
- le proprietà dentro un blocco (layer `0`, *DaBlocco*, *DaLayer*) lungo
  inserimenti annidati;
- la precondizione `isSupported` fra un pacchetto e l'altro.

Non rivedi lo stile né le scelte che gli ADR hanno già preso: se una regola
esistente risponde, la citi e vai avanti
([revisione.md](../../agenti/revisione.md), «Che cosa blocca una revisione»).

## Il verbale

Nella issue, scheda per scheda: i buchi, ciascuno con un esempio e le scelte
possibili, senza scegliere; poi i punti determinati, in una riga. I buchi li
decide il responsabile in [DECISIONI.md](../../DECISIONI.md), e una pull request
li applica alle schede e chiude la issue; senza buchi, la chiude il responsabile
dopo il verbale. Il verbale dice strumento, modello e impegno.

## Fatto quando

- Il verbale è nella issue e copre tutte le schede della tabella.
