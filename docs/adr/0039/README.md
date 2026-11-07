# Immagini di riferimento dell'ADR 0039

Le immagini di riferimento della
[decisione visiva dell'interfaccia](../0039-decisione-visiva-dell-interfaccia.md)
(punto 7), rifatte come file propri del progetto (0.7.5-P9), con la licenza del
progetto. Sono SVG scritti come testo, senza immagini raster dentro: si aprono
in un browser.

Mostrano l'interfaccia come sarà **alla fine della tappa 0.7.5**: testi in
inglese, solo i comandi che esistono (punto 6), il posto per l'indicatore
dell'ambiente e per le schede dei documenti (punto 5). Le icone sono
**segnaposto a ruoli** ([ADR 0040](../0040-icone-a-ruoli.md), punti 1 e 3):
forme semplici nei colori dei gettoni `icon.geo`, `icon.pt`, `icon.cost` e
`icon.esist`, alle griglie da 32 e 16 px; le icone definitive arrivano con
0.7.5-P19–P22. I colori sono i gettoni del punto 2; quelli dell'area di disegno
sono di `render::SceneStyle` (`app/src/theme.cpp`), con lo sfondo su
`surface.canvas`.

La scena è la stessa in tutte le immagini: una piastra con quattro fori, un foro
centrale, gli assi dei fori e una quota disegnata a linee; la piastra (una
polilinea sul layer *Plate*) è selezionata ed è in corso `MOVE`, alla richiesta
del secondo punto, con il punto base sullo spigolo in basso a sinistra.

| File | Che cosa mostra | Punto dell'ADR |
|---|---|---|
| [`finestra-chiaro.svg`](finestra-chiaro.svg) | la finestra intera, 1600×1000, tema chiaro | punto 1, aree 1–10; punti 2, 3 e 5 |
| [`finestra-scuro.svg`](finestra-scuro.svg) | la stessa finestra nel tema scuro | punto 1, aree 1–10; punti 2, 3 e 5 |
| [`ribbon.svg`](ribbon.svg) | barra del titolo con la barra di accesso rapido, schede del ribbon con *File* nel colore d'accento, scheda *Home* a gruppi (*Draw*, *Modify*, *Annotation*, *Layers*, *Properties*, *Utilities*) con il menu delle cinque varianti di *Circle* aperto, riga delle schede dei documenti | punto 1, aree 1–4; punto 3 |
| [`proprieta.svg`](proprieta.svg) | pannello *Properties* con una polilinea selezionata: combo della selezione con filtro e selezione rapida, griglia a gruppi *General* e *Geometry*, scorrimento dei vertici | punto 1, area 8; punto 3 |
| [`layer.svg`](layer.svg) | pannello *Layers*: barra di icone, campo di ricerca, colonne a icone e di testo, layer corrente con la spunta e il fondo di selezione | punto 1, area 9; punto 3 |
| [`stato.svg`](stato.svg) | barra di stato: posto vuoto per *Model* e *Layout*, coordinate, indicatore dell'ambiente, interruttori a icone (orto, polare, snap a oggetto, spessori, immissione dinamica, ribbon) con quelli accesi sul fondo di selezione, scala, impostazioni | punto 1, area 10; punto 5 |
| [`immissione-dinamica.svg`](immissione-dinamica.svg) | immissione dinamica vicino al cursore durante `MOVE`: la richiesta e i due campi, lunghezza (attivo, nel colore d'accento) e angolo | punto 1, area 7 |

I dettagli sono nel tema chiaro e a scala doppia, tranne `ribbon.svg`, a scala
uguale alla finestra perché ne prende tutta la larghezza.

## Che cosa del riferimento resta fuori

Come chiede il punto 6, e per le schede dei pacchetti della tappa:

- nel ribbon solo i comandi della 0.7: niente *Stretch*, *Array*, *Dimension*,
  *Table*, *Measure*; il gruppo *Annotation* ha il solo *Text*;
- nella barra di stato niente schede *Model* e *Layout*, né gli interruttori
  della griglia e dello snap alla griglia, che non esistono ancora; c'è
  l'interruttore del ribbon, che diventa un'icona (0.7.5-P12);
- nel pannello *Properties* niente area della polilinea, che `core` non calcola
  (0.7.5-P14, punto 3);
- nessuna scheda contestuale nel ribbon: il disegno non ne chiede, e il loro
  posto è a destra della scheda *View* (punto 5).
