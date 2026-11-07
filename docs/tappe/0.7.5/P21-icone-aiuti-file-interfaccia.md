# [0.7.5-P21] Icone a ruoli: aiuti al disegno, file, annullamento, vista e interfaccia

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: icone; test delle icone e controllo della
  geometria
- **Dipende da:** P20
- **Modulo:** le icone (`resources/icons/`)
- **Da leggere:** [ADR 0040](../../adr/0040-icone-a-ruoli.md);
  [ICONE.md](../../ICONE.md); le icone fatte da P19 e P20, come esempio di stile

## File

Le icone di oggi da 24 px rifatte a 16 e 32 px, con gli stessi nomi di base,
e i file da 24 px tolti:

- aiuti al disegno: `aids-lwdisplay`, `aids-ortho`, `aids-osnap`, `aids-polar`,
  `aids-snap-center`, `aids-snap-endpoint`, `aids-snap-insertion`,
  `aids-snap-intersection`, `aids-snap-midpoint`, `aids-snap-perpendicular`,
  `aids-snap-quadrant`, `aids-snap-tangent`;
- file e annullamento: `file-new`, `file-open`, `file-save`, `file-saveas`,
  `edit-erase`, `edit-undo`, `edit-redo`;
- vista, strumenti e interfaccia: `view-zoom-extents`, `tools-settings`,
  `ui-ribbon`, `ui-theme`, `help-about`;

più `docs/ICONE.md` (l'elenco) e `tests/app/icons_test.cpp` se un nome cambia.
Le icone contano come coppie 16/32.

## Cosa fare

1. Ridisegna ogni icona con i ruoli: le icone degli snap mostrano l'oggetto in
   `esist` e il punto agganciato in `pt`, con `data-on` o `data-center-of`; le
   icone senza geometria (file, annulla, impostazioni) usano `esist` e, per
   l'elemento attivo, `pt`.
2. Lo stesso significato di oggi per ogni icona (l'elenco di ICONE.md): cambia il
   disegno, non che cosa rappresenta. Nessuna forma copiata da altri programmi.
3. A 16 px i dettagli si semplificano (ADR 0040, punto 3): non rimpicciolire la
   32.

## Regole della tappa che valgono qui

- `docs/ICONE.md`: prima di te P20, dopo di te P22.
- Nessun file di codice: se un'icona chiede un cambiamento in `app`, fermati e
  scrivilo nella issue.

## Test richiesti

- Il controllo di P11 e `icons_test.cpp` passano.

## Fatto quando

- Le 24 icone sono rifatte a 16 e 32 px e i file da 24 px tolti.
- La CI è verde.
