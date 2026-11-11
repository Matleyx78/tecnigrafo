# [0.8-PT39] Icone e voci dei comandi nuovi, prima parte

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: azioni sopra comandi verificati; test Qt e
  controllo della geometria delle icone
- **Dipende da:** PT12, PT15, PT20, PT21, PT26, PT27, PT28, PT33, PT38
- **Modulo:** `app`
- **Da leggere:** [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md),
  «Comandi nell'interfaccia»; [ADR 0040](../../adr/0040-icone-a-ruoli.md),
  [ICONE.md](../../ICONE.md); [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punto 5 (schede del ribbon); `app/src/builtin_actions.cpp`,
  `app/src/ui_builder.cpp`; la pull request di 0.7.5-P20 come esempio

## File

- le icone (a 16 e 32 px, coppie) in `resources/`: `PEDIT`, `-BLOCK`,
  `-INSERT`, `-STYLE`, `MTEXT`, `-DIMSTYLE`, `DIMLINEAR`, `DIMALIGNED`,
  `DIMANGULAR`, `DIMRADIUS`, `DIMDIAMETER` (11 coppie, contate come 11 file)
- `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp`
- `docs/ICONE.md`
- `tests/app/…`

## Cosa fare

Per ogni comando dell'elenco: l'icona a ruoli con le regole di ICONE.md (tratto
da 1 px), l'azione nel registro con il nome del comando, la voce nel ribbon e
nei menu e barre dell'interfaccia classica:

- scheda *Home* del ribbon: `MTEXT` accanto a `TEXT` (gruppo *Annotation*);
  `PEDIT` nel gruppo *Modify*; `-INSERT` e `-BLOCK` in un gruppo *Block*;
- scheda *Annotate* (nuova se non c'è): gruppo *Dimensions* con le cinque quote
  (le prime due come varianti di un pulsante a discesa, come `CIRCLE`) e
  `-DIMSTYLE`; gruppo *Text* con `MTEXT` e `-STYLE`;
- menu *Draw*: `MTEXT`, *Block* → `-BLOCK`; menu *Insert*: `-INSERT`; menu
  *Dimension*: le cinque quote e `-DIMSTYLE`; menu *Format*: `-STYLE`,
  `-DIMSTYLE`; menu *Modify*: `PEDIT`.

Niente scorciatoie nuove (AutoCAD non ne ha per questi comandi).

## Regole della tappa che valgono qui

- `builtin_actions.cpp`, `ui_builder.cpp` e `ICONE.md` in catena: … → PT38 →
  PT39 → PT45.

## Test richiesti

- Ogni azione nuova ha un'icona valida e un comando che esiste nel registro; le
  voci stanno nel ribbon e nei menu dell'ambiente `drawing2d`.
- Il controllo della geometria delle icone passa.

## Fatto quando

- I comandi nuovi della tappa (tranne `-HATCH`, PT45) hanno icona e voci; la CI
  è verde.
