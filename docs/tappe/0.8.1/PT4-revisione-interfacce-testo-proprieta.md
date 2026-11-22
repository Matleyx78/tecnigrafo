# [0.8.1-PT4] Revisione delle interfacce: testo formattato e proprietà

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è una revisione di documenti; verbale nella
  issue
- **Dipende da:** —
- **Modulo:** nessuno: il lavoro sta nella issue, non in una pull request
- **Da leggere:** la lista di controllo di
  [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  punti 1–11; [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md) e
  [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md) per intero;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto 2;
  le schede qui sotto e il codice che toccano

## Perché

Le schede qui sotto introducono tipi e funzioni nuove in `core` e, per PT11,
in `cmd`, su cui crescono gli altri pacchetti della tappa: l'impaginazione
(PT8) la usano il disegno (PT9), l'editor (PT10, PT12) e le quote; il modello
delle proprietà (PT22) lo usano PT23, PT24 e il pannello (PT25). Si controllano
prima che i loro pacchetti partano ([tappe/0000-modello.md](../0000-modello.md),
sezione 6).

## Che cosa si rivede

| Scheda | Interfacce nuove | Come usa le altre |
|---|---|---|
| [PT6](PT6-varianti-del-font.md) | `FontFace`, `advanceWidth`/`textWidth`/`fontMetrics` con la variante | le tabelle generate da `tools/fontmetrics` |
| [PT7](PT7-codici-di-formato.md) | `MTextRun`, `MTextFormat`, `parseFormattedParagraphs` | `mtext_layout::paragraphs` di oggi |
| [PT8](PT8-impaginazione-formati.md) | `FormattedText` in `GeneratedShape`, il nuovo `mtext_layout::layout` | PT6, PT7; `dimension.cpp` |
| [PT10](PT10-modello-di-modifica.md) | `MTextEditModel` e le posizioni del cursore | PT7, PT8 |
| [PT11](PT11-textedit.md) | `Prompt::mtextEditing` | `MTextCommand`, i comandi delle quote |
| [PT22](PT22-proprieta-core.md) | `entityTypeName`, `OwnProperty`, `ownProperties`, `withOwnProperty` | `Document`, `generate()` |
| [PT23](PT23-proprieta-retini-quote.md) | `withPatternOrigin`, le proprietà di `HATCH` e `DIMENSION` | PT22; `hatch_patterns`, `dimension` |
| [PT24](PT24-proprieta-testi-inserimenti.md) | le proprietà di `MTEXT`, `ATTDEF`, `INSERT` | PT22, PT15 |

## Come

Per ogni tipo e funzione, e per ogni passaggio da una all'altra, ogni punto
della lista: è **determinato** dagli ADR, dalla scheda o dal codice di oggi
(cita dove), oppure è un **buco**. Insisti su:

- che cosa arriva a `render` e alla selezione da un pezzo formattato (punto 11):
  variante, colore, righe di decorazione; che cosa vede chi chiama `distance`
  su un `MTEXT` formattato;
- la corrispondenza fra posizione nel valore (con i codici) e posizione del
  cursore nell'editor (PT10), e che cosa succede ai codici non mostrati quando
  si scrive o si cancella accanto a loro;
- il modello delle proprietà: unità (gradi o radianti), valori rifiutati,
  proprietà derivate, e che cosa fa un cambiamento che rende l'entità non
  valida (raggio 0, rapporto > 1, blocco inesistente);
- l'ADR 0054, punti 3 e 6, come li ha decisi il responsabile
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano
  della 0.8.1», punti 1 e 2): che le schede li applichino.

Non rivedi lo stile né le scelte già prese dagli ADR o dal responsabile: se una
regola esistente risponde, la citi e vai avanti
([revisione.md](../../agenti/revisione.md), «Che cosa blocca una revisione»).

## Il verbale

Nella issue, con le classi [bloccante], [decisione], [facoltativa]; i buchi li
decide il responsabile in [DECISIONI.md](../../DECISIONI.md) e una pull request
li applica alle schede. Senza buchi chiude il responsabile.
