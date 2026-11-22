# [0.8.1-PT37] Icone e voci dei comandi nuovi

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: azioni sopra comandi verificati; test Qt e
  controllo della geometria delle icone
- **Dipende da:** PT11, PT14, PT17, PT18, PT19, PT20, PT21, PT26, PT28, PT32,
  PT35, PT36
- **Modulo:** `app` (più le icone)
- **Da leggere:** [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md),
  "Comandi nell'interfaccia"; [ADR 0040](../../adr/0040-icone-a-ruoli.md);
  [ICONE.md](../../ICONE.md); la scheda della 0.8
  [PT39](../0.8/PT39-icone-e-voci-1.md) (lo stesso lavoro) e la voce del
  2026-11-20 «Home di F6 con i comandi della 0.8»

## File

- `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp`
- `resources/icons/` (a coppie 16/32)
- `tests/app/builtin_actions_test.cpp`, `tests/app/ui_builder_test.cpp`

## Cosa fare

Per `TEXTEDIT`, `ATTDEF`, `ATTEDIT`, `ATTSYNC`, `BATTMAN`, `ATTDISP`,
`HATCHEDIT`, `RECTANG`, `POLYGON`, `GRID`, `SNAP`, `UCSICON`: l'icona a ruoli, la
voce nel registro delle azioni (menu, ribbon, barre dell'interfaccia classica)
dove AutoCAD li mette (*Draw*: `RECTANG` e `POLYGON` accanto a `ELLIPSE`;
*Block* e *Insert*: gli attributi; *Modify*: `HATCHEDIT`, `TEXTEDIT`; *View*:
`UCSICON`), e la scorciatoia se ne ha una. `F7` e `F9` li ha già PT32.

**Senza icona né voce**, come in AutoCAD, che li usa solo dalla riga di comando
(verbale della revisione del piano, [bloccante] 9): `SELECT` (PT34) e le
variabili `AFLAGS` (PT14), `ATTREQ` (PT17), `GRIDMODE`, `GRIDUNIT`,
`SNAPMODE`, `SNAPUNIT`, `SNAPBASE`, `GRIDDISPLAY`, `GRIDMAJOR` (PT28). Sono
nel manuale (PT38) e nel catalogo di parità.

## Regole della tappa che valgono qui

- Le icone approvate non cambiano più; ogni pacchetto aggiunge le sue
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-08). La disposizione di Home resta
  quella del 2026-11-20.

## Test richiesti

- Ogni comando nuovo dell'elenco ha la sua azione, l'icona nelle due misure e
  una voce di menu e di ribbon; nessuno di quelli «senza icona» ne ha una.

## Fatto quando

- I test passano; la CI è verde; il criterio di chiusura «Ogni comando nuovo
  ha la sua icona e le sue voci» è verificato.
