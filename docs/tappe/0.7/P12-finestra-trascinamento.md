# [0.7-P12] `app`: finestra di selezione chiusa anche trascinando

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno medio
- **Riferimenti:** non servono: gesto della vista, che gli script non
  simulano; test Qt e prova manuale
- **Dipende da:** P11
- **Modulo:** `app`
- **Da leggere:** l'idea [#61](https://github.com/Matleyx78/tecnigrafo/issues/61);
  [ADR 0008](../../adr/0008-selezione-e-snap.md) (finestra e intersezione);
  `app/src/cad_view.cpp` (`mousePressEvent`, `mouseMoveEvent`,
  `mouseReleaseEvent`); `tests/app/selection_test.cpp`

## File

- `app/src/cad_view.cpp`, `app/src/cad_view.hpp`
- `tests/app/selection_test.cpp`

## Cosa fare

Il comportamento di `PICKDRAG` = 2 di AutoCAD, sempre attivo, senza opzioni:

- **clic fermo** (pressione e rilascio a meno di 5 pixel l'uno dall'altro): come
  oggi, il rettangolo resta aperto fino al secondo clic;
- **pressione, trascinamento oltre 5 pixel, rilascio**, con un rettangolo di
  selezione aperto da quella pressione: il rilascio vale come secondo clic e
  chiude il rettangolo;
- la direzione decide tra finestra e intersezione, `Shift` al rilascio toglie
  dalla selezione ed `Esc` annulla, come oggi;
- il trascinamento chiude **solo** un rettangolo di selezione: durante una
  richiesta di punto o di oggetto il rilascio non fa nulla, e non sposta un
  grip.

Il motore dei comandi non cambia: la vista manda a `CommandRunner::click` il
punto del rilascio. Quando un rettangolo è aperto lo dice
`CommandRunner::selectionRectangle()`.

## Regole della tappa che valgono qui

- `cad_view.cpp`: prima di te P11.
- I 5 pixel sono la soglia di trascinamento della vista, non una tolleranza
  geometrica.

## Test richiesti

- Test Qt con eventi simulati: clic e clic; pressione, trascinamento e rilascio
  verso destra (finestra) e verso sinistra (intersezione); rilascio con `Shift`;
  trascinamento sotto i 5 pixel (resta aperto); trascinamento durante una
  richiesta di punto (nessun effetto).

## Fatto quando

- I test qui sopra esistono e passano; quelli di selezione esistenti passano
  ancora.
- La CI è verde.
