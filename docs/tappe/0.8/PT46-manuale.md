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
   `PEDIT`, `PEDITACCEPT`, con le opzioni che ci sono e quelle rimaste fuori;
   `EXPLODE` con inserimenti e quote; il tracciamento (`F11`).
2. **Differenze da AutoCAD** scritte dove servono: testo delle quote che resta
   in mezzo, inserimenti a scala non uniforme non esplodibili, codici di formato
   di `MTEXT` mostrati come testo semplice, retini senza associatività.
3. **Assistente:** `assistente.md` spiega come accendere il canale, come
   configurare un assistente (`tecnigrafo --mcp`, con un esempio per Claude
   Desktop e uno per Claude Code), che cosa può fare e che cosa no, come
   annullare.
4. **Architettura:** la geometria generata, le tabelle nuove, il canale.

Il catalogo di parità lo aggiorna la pull request di chiusura (Codex), non
questo pacchetto.

## Fatto quando

- Manuale e architettura descrivono la tappa; la CI è verde.
