# [0.6-P27] Manuale e architettura

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #261
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** P20, P24
- **Modulo:** documentazione
- **Da leggere:** le schede dei pacchetti P12, P13, P14, P16, P17, P20, P22,
  P24 di questa tappa (per i testi dei comandi e delle finestre); gli ADR
  0009, 0023–0027 (solo le decisioni); `docs/manuale/comandi.md` e
  `docs/ARCHITETTURA.md` (come sono scritti oggi)

## File

- `docs/manuale/comandi.md`
- `docs/manuale/interfaccia.md` (nuovo)
- `docs/ARCHITETTURA.md`
- `README.md` (solo l'elenco delle funzioni, se c'è)

## Cosa fare

- **`comandi.md`:** una sezione per `-LAYER` e `CLAYER`, `COLOR`,
  `-LINETYPE`, `LTSCALE` e `CELTSCALE`, `LWEIGHT` e `LWDISPLAY`, `CHPROP`, con
  le domande, le opzioni, i messaggi e un esempio da scrivere nella riga di
  comando, come le sezioni esistenti. Una sezione "Layer bloccati, spenti e
  congelati" con la tabella dell'ADR 0023, punto 4.
- **`interfaccia.md`:** ribbon e interfaccia classica (`RIBBON`,
  `RIBBONCLOSE`, pulsante), pannelli (`LAYER`, `PROPERTIES`) e come
  agganciarli, `QSELECT`, temi, scorciatoie (finestra, esportare e
  importare), `LWT`. Collegala da `comandi.md`.
- **`ARCHITETTURA.md`:** il modulo `mcp` nello schema e nella tabella dei
  moduli; registro delle azioni, `UiBuilder`, temi e pannelli in `app`;
  layer e proprietà in `core`; `third_party/` con SARibbon e nlohmann/json.

## Regole della tappa che valgono qui

- Testi in italiano; comandi, domande e messaggi come li scrive il programma,
  in inglese.
- Descrivi ciò che il programma fa (le schede e i test), non ciò che fa
  AutoCAD; non copiare testi di Autodesk.

## Test richiesti

Nessuno: documentazione. I collegamenti tra i documenti devono funzionare.

## Fatto quando

- Ogni comando e ogni finestra della tappa sono nel manuale.
- La CI è verde.
