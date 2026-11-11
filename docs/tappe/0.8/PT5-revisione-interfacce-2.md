# [0.8-PT5] Revisione delle interfacce: testo multiriga, quote, retini, unione con distanza

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
  punti 1–11; [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), [0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md), [0047](../../adr/0047-quote-e-stili-di-quota.md), [0048](../../adr/0048-retini.md) e [0050](../../adr/0050-pedit.md) per intero; le schede qui sotto; il codice che toccano, con le
  intestazioni che citano

## Perché

Le schede qui sotto introducono tipi e funzioni nuove in `core`. Si controllano
prima che i loro pacchetti partano ([tappe/0000-modello.md](../0000-modello.md),
sezione 6). Sono le schede con le regole geometriche scritte da Tecnigrafo (impaginazione, grafica delle quote, unione con distanza): i loro pacchetti sono in fascia media, quindi le decisioni devono essere complete prima.

## Che cosa si rivede

In quest'ordine, che è quello in cui le interfacce si usano l'una con l'altra:

| Scheda | Interfacce nuove | Come usa le altre |
|---|---|---|
| [PT13](PT13-mtext-core.md) | `MText`, impaginazione, righe generate | PT6, PT11 |
| [PT22](PT22-stili-di-quota.md) | `DimStyle`, `DimStyleTable`, le variabili essenziali e correnti | PT6, PT11 |
| [PT23](PT23-quote-lineari.md) | valore e testo, grafica delle quote lineari e allineate, rigenerazione | PT10, PT13, PT22 |
| [PT24](PT24-quote-angolari-radiali.md) | grafica delle quote angolari, radiali e diametrali | PT23 |
| [PT30](PT30-retini-core.md) | `Hatch` e le sue funzioni, il riempimento pieno | PT6, PT8 |
| [PT32](PT32-unione-con-distanza.md) | `joinWithGaps`, `reversed` | `join` (0.7-P22), le intersezioni di solid-space |

## Come

Per ogni tipo e ogni funzione delle schede, e per ogni passaggio da una
all'altra, ogni punto della lista: è **determinato** dagli ADR, dalla scheda o
dal codice di oggi (e allora citi dove), oppure è un **buco**. Insisti su:

- i punti 10 e 11: che cosa arriva da solid-space (un'intersezione, un punto
  esatto) e che cosa le schede passano avanti come numero arrotondato (#359);
- le regole al limite: testo che non sta, frecce fuori, quote di lunghezza
  zero, archi di 360°, distanza di tolleranza uguale alla distanza fra due
  estremi, una riga di testo vuota;
- che cosa succede a una quota che non si può rigenerare in ogni comando che la
  tocca.

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
