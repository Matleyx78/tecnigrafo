# [0.7.5-P40] `app`: correzioni della prova manuale (barra di accesso rapido, schede del ribbon, pannelli a scomparsa, icone nitide)

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: aspetto e icone, il comportamento dei comandi
  non cambia; test Qt, controllo della geometria delle icone, galleria e prova
  manuale
- **Dipende da:** P39 (ultimo pacchetto unito della tappa)
- **Nata:** 2026-11-10, dalla prova manuale della chiusura
  ([#647](https://github.com/Matleyx78/tecnigrafo/pull/647)) su Debian, sotto
  Wayland, a scala 100%: i punti 2, 4 e 7 della sezione 7 del piano non
  passano ([DECISIONI.md](../../DECISIONI.md), 2026-11-10, «Icone a tratto da
  1 px e correzioni della prova manuale»)
- **Misura:** oltre la quindicina di file, per decisione del responsabile: le
  icone ridisegnate contano come un gruppo solo (stessa decisione)
- **Modulo:** `app` (più i suoi test, le icone di `resources/icons/`, il
  controllo `tools/icone.py` e una riga di `.github/workflows/ci.yml`)
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punti 1 (aree 1–3), 3 e 4; [ADR 0040](../../adr/0040-icone-a-ruoli.md),
  punti 1–4, con il tratto cambiato dalla decisione del 2026-11-10;
  [ICONE.md](../../ICONE.md); le immagini di `docs/adr/0039/` (`ribbon.svg`,
  `finestra-chiaro.svg`); in `app/src`: `windowStyles()` in `main_window.cpp`,
  `UiBuilder::buildRibbon` e `fillQuickAccess` in `ui_builder.cpp`, il foglio
  di stile di `style.cpp`, `AutoHide::placeOverlay` in `auto_hide.cpp`,
  `themedIcon` e `drawnIcon` in `theme.cpp`; in
  `third_party/saribbon/SARibbon.cpp`, solo da leggere,
  `SARibbonBarLayout::resizeInCompactStyle` (dove lo stile compatto mette la
  barra di accesso rapido, il pulsante *File* e le schede)

## File

- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/ui_builder.hpp`, `app/src/ui_builder.cpp`
- `app/src/style.cpp`
- `app/src/auto_hide.hpp`, `app/src/auto_hide.cpp`
- `app/src/theme.cpp`
- `app/tool/gallery.cpp`
- `.github/workflows/ci.yml` (solo il numero delle immagini della galleria,
  da 12 a 14: la scheda lo permette)
- `tests/app/ui_builder_test.cpp`, `tests/app/style_test.cpp`,
  `tests/app/auto_hide_test.cpp`, `tests/app/icons_test.cpp`,
  `tests/app/gallery_test.cpp`
- `tools/icone.py`, `tools/icone_test.py`
- `docs/ICONE.md`
- `resources/icons/*.svg`: tutte le icone, ridisegnate (un gruppo)

## Cosa fare

### 1. Barra di accesso rapido a sinistra anche con la cornice del sistema

Oggi, con la cornice del sistema (sotto Wayland, `windowStyles()`),
`UiBuilder::buildRibbon` sceglie lo stile compatto di SARibbon, che mette la
barra di accesso rapido **a destra** delle schede. Il riferimento la vuole in
alto a sinistra (ADR 0039, punto 1, area 1). La galleria non lo mostrava,
perché la piattaforma `offscreen` usa la cornice propria.

1. **Scelta della cornice esplicita.** In `main_window.hpp`:

   ```cpp
   /// The frame of the window (docs/adr/0039, point 4).
   enum class WindowFrame : std::uint8_t {
       Automatic,  ///< The system's under Wayland, the ribbon's own elsewhere.
       System,     ///< Always the frame of the system.
       Own,        ///< Always the ribbon's own frame.
   };
   ```

   Il costruttore di `MainWindow` riceve `WindowFrame frame =
   WindowFrame::Automatic` come ultimo argomento, dopo quelli di oggi, che non
   cambiano. `Automatic` fa ciò che fa oggi `windowStyles()`. Il programma usa
   sempre `Automatic`; `System` e `Own` servono a test e galleria.
2. **Con la cornice del sistema**, nello stile compatto, la barra di accesso
   rapido di SARibbon si nasconde e al suo posto si mette una `QToolBar`
   riempita da `fillQuickAccess`, come widget d'angolo `Qt::TopLeftCorner` del
   ribbon: lo stile compatto lo dispone per primo, prima del pulsante *File*
   (`resizeInCompactStyle`). L'`objectName` resta `quickAccessBar`, come nel
   menu classico. Con la cornice propria non cambia niente.

### 2. Schede del ribbon alte quanto *File*

Le regole `QTabBar::tab` del foglio di stile (padding 4px 12px, bordo superiore
di 2 px) sono quelle delle schede dei documenti, ma valgono anche per
`SARibbonTabBar`, che deriva da `QTabBar`: le schede del ribbon escono dalla
loro riga e scendono sotto *File*.

1. Le regole delle schede dei documenti si limitano alla loro barra,
   `QTabBar#documentTabBar::tab` (e `:hover`, `:selected`).
2. Le schede del ribbon hanno regole loro, `SARibbonTabBar::tab`, senza bordi
   e senza il padding verticale delle schede dei documenti, con il testo
   centrato in altezza; la selezionata sul fondo del ribbon, come oggi.
3. Con le due cornici, il pulsante *File* e le schede del ribbon hanno lo
   **stesso bordo superiore e la stessa altezza**, entro 1 px: se lo stile
   compatto dà a *File* un'altezza diversa da quella della riga delle schede
   (`scaleSizeByHeight(..., 1.5)`), la si fissa da `styleRibbon` con
   l'altezza della riga.

### 3. Pannelli a scomparsa che si allargano

Il riquadro che si apre dalla linguetta ha oggi la larghezza (o l'altezza, per
i bordi in alto e in basso) che il pannello aveva quando lo si è nascosto, e
non la si può cambiare.

1. Sul bordo del riquadro verso l'area di disegno c'è una **maniglia** di 4 px,
   con il cursore di ridimensionamento: trascinandola cambia l'estensione del
   riquadro, fra un minimo di 120 px e l'area di disegno meno 120 px.
2. L'estensione nuova è quella della voce nascosta (`extent`): la salva
   `saveState` come oggi, quindi resta dopo il riavvio, ed è quella con cui il
   pannello torna agganciato quando si rimette la puntina.
3. Trascinare la maniglia non chiude il riquadro: durante il trascinamento il
   puntatore conta come dentro.

### 4. Icone nitide

**Disegno alla misura esatta.** Oggi `themedIcon` disegna immagini a misure
fisse (16, 20, 24, 32, 40, 48, 64, 96, 128) senza rapporto di densità, e Qt le
riscala per le altre. Al loro posto un `QIconEngine` (in `theme.cpp`, non
esposto) che per ogni richiesta disegna l'SVG già colorato alla misura in pixel
del dispositivo, `size × devicePixelRatio`, e restituisce un'immagine con quel
rapporto di densità. La griglia si sceglie dalla **misura logica**: fino a 24
il file `-16`, oltre il `-32` (a 200% un'icona da 16 si disegna dal `-16` a 32
pixel, con tratto da 2). Le immagini disegnate si conservano per misura e
densità; la cache per icona e tema di P35 resta com'è. Firma e comportamento di
`themedIcon` non cambiano.

**Tratto da 1 px su mezzo pixel** (decisione del 2026-11-10, che sostituisce il
tratto dell'ADR 0040, punto 3):

| Griglia | Tratto | Linee orizzontali e verticali | Punti (raggio) | Tratteggio, almeno | Margine |
|---|---|---|---|---|---|
| 32 px | 1 | coordinata della mezzeria in `n + 0,5` | 2,5 | 2 | 2 px |
| 16 px | 1 | coordinata della mezzeria in `n + 0,5` | 1,5 | 2 | 1 px |

- Ogni segmento orizzontale o verticale con tratto (`line`, `polyline`,
  `polygon`, `rect`, `H` e `V` di un `path`) ha la mezzeria su un mezzo pixel,
  così il tratto copre una colonna o una riga intera di pixel.
- Un cerchio o un arco con tratto ha `c ± r` su un mezzo pixel, in x e in y.
- Un punto (pieno, senza tratto) ha i bordi su pixel interi: il centro in
  `n + 0,5` e il raggio della tabella, così sta sulla mezzeria della linea su
  cui si trova (`data-on`).
- Un riempimento senza tratto (`rect`, `polygon`) ha i lati orizzontali e
  verticali su pixel interi.
- Diagonali e curve restano sfumate: non si allineano.

Tutte le icone di `resources/icons/` si ridisegnano con queste regole, senza
cambiare forma, ruoli, `id` e `data-on`. Lo spostamento di mezzo pixel si
sceglie verso l'interno, perché il margine resti libero. ICONE.md cambia la
tabella delle due griglie e dice le regole qui sopra.

**Controllo automatico.** `tools/icone.py` aggiunge il controllo
dell'allineamento: per ogni elemento dei casi qui sopra segnala il file, l'`id`
o la posizione e la coordinata fuori dal mezzo pixel (o dal pixel intero), con
tolleranza 0,01. Il tratto ammesso diventa 1 nelle due griglie, e il
tratteggio minimo 2. Il controllo vale per tutte le icone del catalogo.

### 5. Galleria

La galleria aggiunge `ribbon-cornice-sistema-chiaro.png` e
`ribbon-cornice-sistema-scuro.png`, il ribbon di una finestra con
`WindowFrame::System`; le immagini diventano 14 (in `gallery_test.cpp` e nel
passo *Generate interface gallery* di `ci.yml`). Le altre si fanno con
`WindowFrame::Own`, come oggi le dà la piattaforma `offscreen`.

## Regole della tappa che valgono qui

- Testi dell'interfaccia in inglese fino alla 0.9.
- File comuni in catena: `app/src/main_window.*`, `app/src/ui_builder.cpp` e
  `docs/ICONE.md` vengono dopo tutti i pacchetti della tappa che li toccano.
- Nessuna libreria nuova (ADR 0039, punto 4); SARibbon non si modifica.
- I casi di verifica non cambiano: il pacchetto non tocca `core`, `edit`,
  `cmd`, `render`.

## Test richiesti

- `ui_builder_test.cpp`: con `WindowFrame::System`, la barra
  `quickAccessBar` è visibile, è il widget d'angolo `Qt::TopLeftCorner` e il
  suo bordo destro sta a sinistra del pulsante *File*; la barra di accesso
  rapido di SARibbon è nascosta; le azioni sono nuovo, apri, salva, annulla,
  ripeti. Con `WindowFrame::Own`, la barra è nella riga del titolo, a sinistra
  del titolo, come oggi.
- `style_test.cpp` (o `ui_builder_test.cpp`): con le due cornici e nei due
  temi, il rettangolo di ogni scheda del ribbon (`tabRect`, in coordinate del
  ribbon) ha bordo superiore e altezza uguali a quelli del pulsante *File*
  entro 1 px; una scheda della barra dei documenti ha ancora il bordo
  superiore d'accento quando è selezionata.
- `auto_hide_test.cpp`: un pannello nascosto a destra, aperto dalla
  linguetta; trascinare la maniglia di 80 px verso l'area di disegno allarga il
  riquadro di 80 px; `saveState` riporta l'estensione nuova; dopo
  `restoreState` in una finestra nuova il riquadro si apre largo uguale; i due
  limiti (120 px e area meno 120 px); il riquadro resta aperto durante il
  trascinamento.
- `icons_test.cpp`: `themedIcon(...).pixmap(QSize(16, 16), 2.0)` ha 32×32
  pixel e rapporto di densità 2, ed è disegnata dal file `-16`; a 1,25
  l'immagine è 20×20; a 1,0 una linea orizzontale di `file-save-16` copre una
  riga sola di pixel pieni (nessun pixel parzialmente coperto sopra o sotto
  la mezzeria, in un'icona di prova delle risorse dei test).
- `tools/icone_test.py`: un'icona con una linea orizzontale su coordinata
  intera è segnalata; la stessa su mezzo pixel no; un punto con il centro
  fuori dal mezzo pixel è segnalato; un cerchio con `c + r` intero è
  segnalato; un tratto da 1,25 è segnalato.
- `gallery_test.cpp`: 14 immagini, con le due nuove.

## Fatto quando

- Con la cornice del sistema la barra di accesso rapido sta in alto a
  sinistra, prima di *File*; con quella propria resta nella riga del titolo.
- *File*, *Home* e *View* stanno sulla stessa riga e sono alti uguali, nelle
  due cornici e nei due temi.
- Il riquadro di un pannello a scomparsa si allarga e si stringe con la
  maniglia, e la misura resta dopo il riavvio.
- Tutte le icone hanno tratto da 1 px e passano il controllo
  dell'allineamento di `tools/icone.py` in CI; a 100% e 200% le linee
  orizzontali e verticali sono di pixel pieni.
- La galleria ha le 14 immagini; nelle due nuove la barra di accesso rapido è
  a sinistra di *File*.
- I test richiesti esistono e la CI è verde.
- Il responsabile ripete i punti 2, 4 e 7 della prova manuale su Debian
  (Wayland) e Windows: è la condizione per riprendere la chiusura (#647), non
  per unire il pacchetto.
