# [0.7.5-P22] Icone a ruoli: layer, proprietà e pannelli

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: icone; test delle icone e controllo della
  geometria
- **Dipende da:** P13, P21
- **Modulo:** le icone (`resources/icons/`)
- **Da leggere:** [ADR 0040](../../adr/0040-icone-a-ruoli.md);
  [ICONE.md](../../ICONE.md); le icone fatte da P13, P19, P20 e P21, come esempio
  di stile

## File

Le icone di oggi da 24 px rifatte a 16 e 32 px, con gli stessi nomi di base, e
i file da 24 px tolti:

- layer: `layer-current`, `layer-delete`, `layer-frozen`, `layer-thawed`,
  `layer-locked`, `layer-unlocked`, `layer-new`, `layer-off`, `layer-on`,
  `layer-panel`;
- proprietà: `properties-panel`, `property-color`, `property-linetype`,
  `property-lineweight`, `quick-select`;

più `docs/ICONE.md` (l'elenco; la griglia da 24 px esce dalle regole, perché non
resta nessuna icona che la usa) e `tests/app/icons_test.cpp` (che non accetta più
la griglia da 24 px). Le icone contano come coppie 16/32.

## Cosa fare

1. Come P21: stesso significato di oggi, ruoli, 16 px disegnata a parte.
2. Le icone di stato dei layer (acceso, congelato, bloccato, con le loro opposte)
   si leggono a 16 px nelle righe del pannello *Layers* e nella combo: provale lì,
   nella galleria.
3. Le icone nuove che P13 ha aggiunto restano come sono, se seguono già le
   regole.
4. Quando non resta nessuna icona da 24 px, `themedIcon` e il test non la
   cercano più: togli il ripiego di P11 e scrivilo nella pull request.

## Regole della tappa che valgono qui

- `docs/ICONE.md`: prima di te P21.
- `app/src/theme.cpp`: solo il ripiego sulla griglia da 24 px, che si toglie.

## Test richiesti

- Il controllo di P11 e `icons_test.cpp` passano; un'icona da 24 px è rifiutata.

## Fatto quando

- Le 15 icone sono rifatte, nessuna icona da 24 px resta, e il ripiego è tolto.
- La CI è verde.
