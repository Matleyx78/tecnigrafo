# [0.8-PT46] Manuale e catalogo di parità della tappa

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** codex
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** PT45
- **Modulo:** documenti
- **Da leggere:** gli ADR [0044](../../adr/0044-entita-composte-e-geometria-generata.md)–[0051](../../adr/0051-canale-mcp-dell-assistente.md);
  `docs/manuale/comandi.md`, `docs/manuale/interfaccia.md`;
  `docs/parita/comandi.md`; `docs/ARCHITETTURA.md`

## File

- `docs/manuale/comandi.md`, `docs/manuale/interfaccia.md`
- `docs/manuale/assistente.md` (nuovo)
- `docs/ARCHITETTURA.md`

## Cosa fare

1. **Comandi:** una voce per `-STYLE`, `TEXTSTYLE`, `MTEXT`, `-BLOCK`,
   `-INSERT` (anche con un file), `DIMLINEAR`, `DIMALIGNED`, `DIMANGULAR`,
   `DIMRADIUS`, `DIMDIAMETER`, `-DIMSTYLE` e le variabili di quota, `-HATCH`,
   `HPISLANDDETECTION`, `PEDIT`, `PEDITACCEPT`, con le opzioni che ci sono e
   quelle rimaste fuori;
   `EXPLODE` con inserimenti e quote; il tracciamento (`F11`).
2. **Differenze da AutoCAD** scritte dove servono: quote con `DIMATFIT`,
   `DIMTMOVE`, `DIMTIX`, `DIMSOXD` o `DIMTOFL` diversi da `ISO-25` non
   rigenerabili, che restano come sono e si spostano soltanto
   ([ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punti 2 e 4;
   decisione del 2026-11-11, punto 6); ellissi senza `TRIM`, `EXTEND`,
   `OFFSET`, `FILLET` e gli altri comandi di modifica
   ([ADR 0052](../../adr/0052-ellisse.md), punto 6; idea
   [#663](https://github.com/Matleyx78/tecnigrafo/issues/663)); codici di formato
   di `MTEXT` mostrati come testo semplice, retini senza associatività, contorni
   dei retini rilevati solo fra linee, archi, cerchi e polilinee (non dentro
   blocchi e quote, [PT43](PT43-contorni-dei-retini.md)), niente snap *Vicino*
   (idea [#665](https://github.com/Matleyx78/tecnigrafo/issues/665)).
3. **Assistente:** `assistente.md` spiega come accendere il canale, come
   configurare un assistente (`tecnigrafo --mcp`, con un esempio per Claude
   Desktop e uno per Claude Code), che cosa può fare e che cosa no, come
   annullare.
4. **Architettura:** la geometria generata, le tabelle nuove, il canale.

Il catalogo di parità lo aggiorna la pull request di chiusura (Codex), non
questo pacchetto.

## Fatto quando

- Manuale e architettura descrivono la tappa; la CI è verde.
