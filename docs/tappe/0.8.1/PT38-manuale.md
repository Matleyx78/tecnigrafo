# [0.8.1-PT38] Manuale e catalogo di parità della tappa

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** codex
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** PT37
- **Modulo:** `docs/manuale`, `docs/parita`
- **Da leggere:** la scheda della 0.8 [PT46](../0.8/PT46-manuale.md); gli ADR
  0054–0057

## File

- `docs/manuale/` (le pagine dei comandi e degli aiuti)
- `docs/parita/comandi.md`

## Cosa fare

1. Nel manuale: `ELLIPSE` (che mancava), i comandi nuovi della tappa, la
   griglia e lo snap alla griglia (con `SNAPANG` e lo stile isometrico
   conservati ma senza effetto), l'icona dell'UCS, gli snap nuovi, *Last* e
   *Previous*, il prefisso `#`, l'editor del testo e la frase sui font (ADR
   0054, «Conseguenze»).
2. Nel catalogo di parità: le righe della 0.8.1 da «assente» a «presente», con
   ciò che resta fuori.

## Regole della tappa che valgono qui

- Nessuna.

## Test richiesti

- Nessuno; `scripts/controlla.sh --solo formato`.

## Fatto quando

- Ogni comando e aiuto nuovo è nel manuale e nel catalogo; la CI è verde.
