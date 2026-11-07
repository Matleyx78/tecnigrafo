# [0.7.5-P18] `app`: pannelli che si nascondono da soli

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: comportamento dei pannelli, che non tocca il
  disegno; test Qt e prova manuale
- **Dipende da:** P14, P16
- **Modulo:** `app`
- **Da leggere:** la [#534](https://github.com/Matleyx78/tecnigrafo/issues/534);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-07 («Pannelli che si nascondono
  da soli»); [ADR 0009](../../adr/0009-interfaccia-grafica.md), punto 3;
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punti 2 e 3;
  `app/src/main_window.cpp` (`addPanel`, `saveUiSettings`,
  `restoreUiSettings`, `setInterface`), `app/src/style.hpp` (P10),
  `app/src/layer_panel.hpp`, `app/src/properties_panel.hpp`

## Perché

I pannelli agganciati ai lati occupano spazio anche quando non servono. In
AutoCAD una palette si può lasciar nascondere: resta una linguetta sul bordo, e il
pannello scivola fuori quando ci passa il puntatore. I `QDockWidget` di Qt non lo
fanno, e il responsabile non vuole librerie esterne: lo si costruisce con Qt
(decisione del 2026-11-07).

## File

- `app/src/auto_hide.hpp`, `app/src/auto_hide.cpp` (nuovi: le barre delle
  linguette, il riquadro sovrapposto, la barra del titolo con la puntina)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/style.cpp`, solo per l'aspetto di linguette e riquadro con i gettoni
- `resources/icons/ui-pin-16.svg`, `ui-unpin-16.svg` (nuove, con le regole di
  ICONE.md)
- `app/CMakeLists.txt`
- `tests/app/auto_hide_test.cpp` (nuovo), `tests/CMakeLists.txt`

## Cosa fare

1. **Barra del titolo** dei pannelli agganciati (`QDockWidget::setTitleBarWidget`):
   il nome, una **puntina** (*Auto-hide*) e la crocetta, con lo stile dei gettoni.
   Un pannello mobile (staccato) non ha la puntina.
2. **Nascondere:** con la puntina tolta, il pannello lascia il suo posto e sul
   bordo dov'era agganciato (sinistra, destra o in basso) compare una
   **linguetta**: icona e nome, in verticale ai lati, in orizzontale in basso. I
   pannelli nascosti sullo stesso bordo stanno nella stessa barra, nell'ordine in
   cui li si è nascosti. L'area di disegno si allarga.
3. **Mostrare:** il puntatore sulla linguetta per un momento, o un clic, fa
   apparire il contenuto del pannello in un **riquadro sovrapposto** all'area di
   disegno, dentro la finestra principale (non una finestra a parte: sotto
   Wayland una finestra mobile non si posiziona), della larghezza che il pannello
   aveva agganciato e alto quanto l'area di disegno.
4. **Richiudere:** il riquadro si richiude quando il puntatore ne esce per un
   momento e il fuoco della tastiera non è dentro di lui (una cella in modifica
   lo tiene aperto), con `Esc`, o quando si apre un'altra linguetta.
5. **Rimettere la puntina** riaggancia il pannello dov'era.
6. **Salvare:** quali pannelli sono nascosti, e su quale bordo, si salvano con la
   disposizione e si ripristinano all'avvio, insieme a `saveState`; il passaggio
   tra ribbon e interfaccia classica non li cambia. In modo script niente si
   salva e niente si nasconde.
7. I tempi («un momento») sono costanti nel codice, scelte da te e scritte nella
   pull request.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P16, dopo di te P24. `properties_panel.*` e
  `layer_panel.*`: non li tocchi, il pannello resta com'è dentro il suo
  `QDockWidget`.
- Nessuna libreria nuova (ADR 0009, punto 3). Interfaccia in inglese.

## Test richiesti

`auto_hide_test.cpp` (test Qt):

- togliere la puntina a *Properties* agganciato a destra mette la linguetta a
  destra e allarga l'area di disegno; rimetterla lo riaggancia a destra;
- un clic sulla linguetta mostra il riquadro, dentro la finestra; `Esc` lo
  richiude; con il fuoco in una cella resta aperto;
- lo stato nascosto si salva e si ripristina in una finestra nuova con le stesse
  impostazioni; il passaggio a interfaccia classica e ritorno lo conserva;
- un pannello mobile non ha la puntina.

## Fatto quando

- I pannelli agganciati si nascondono e si mostrano come qui sopra, con Qt solo.
- I test qui sopra esistono e passano.
- La CI è verde.
