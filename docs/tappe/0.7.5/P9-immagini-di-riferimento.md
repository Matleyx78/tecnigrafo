# [0.7.5-P9] Immagini di riferimento dell'interfaccia (ADR 0039)

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: sono immagini di un ADR
- **Dipende da:** —
- **Modulo:** `docs`
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md)
  per intero; [ADR 0040](../../adr/0040-icone-a-ruoli.md), punti 1 e 3; il
  mockup «prima versione» nel raccoglitore privato del responsabile, con
  l'accesso in sola lettura che lui ti dà nella chat

## Perché

L'ADR 0039 descrive il riferimento a parole e con i gettoni; il confronto con la
galleria di schermate e la prova manuale chiedono anche un'immagine. Il mockup
sta in un repository privato e niente di lì entra così com'è: le immagini si
rifanno come file propri, nel repository, con la licenza del progetto.

## File

- `docs/adr/0039/finestra-chiaro.svg`, `docs/adr/0039/finestra-scuro.svg`
- `docs/adr/0039/ribbon.svg`, `proprieta.svg`, `layer.svg`, `stato.svg`,
  `immissione-dinamica.svg` (nel tema chiaro)
- `docs/adr/0039/README.md`: che cosa mostra ogni immagine e da quale punto
  dell'ADR
- `docs/adr/0039-decisione-visiva-dell-interfaccia.md`: solo i collegamenti alle
  immagini nel punto 7

## Cosa fare

1. Disegna le immagini in SVG, a mano, a 1600×1000 per la finestra intera,
   seguendo l'ADR 0039 area per area e i suoi gettoni. Non copiare, non
   tracciare e non incorporare le immagini del mockup: lo guardi, poi disegni.
2. Le immagini mostrano l'interfaccia come sarà **alla fine della 0.7.5**: testi
   in inglese, solo i comandi che esistono (punto 6 dell'ADR), con il posto per
   l'indicatore dell'ambiente e per le schede dei documenti (punto 5). Il
   disegno nell'area di lavoro è una piastra semplice con fori e una quota
   disegnata a linee, come nel mockup.
3. Le icone nelle immagini sono segnaposto a ruoli (ADR 0040): forme semplici
   nei colori dei ruoli, non le icone definitive, che arrivano con P19–P22.
4. Ogni file SVG ha l'intestazione SPDX in commento, come le icone
   ([ICONE.md](../../ICONE.md)), ed è leggibile come testo (niente immagini
   raster dentro).

## Regole della tappa che valgono qui

- Niente del raccoglitore entra nel repository, nemmeno un collegamento o il
  suo nome (scheda della tappa, «Interfaccia», riferimento visivo).

## Test richiesti

Nessuno: sono immagini. Il responsabile le confronta con il mockup prima di
unire.

## Fatto quando

- Le sette immagini esistono, si aprono in un browser e seguono l'ADR 0039.
- Il punto 7 dell'ADR le collega.
- La CI è verde.
