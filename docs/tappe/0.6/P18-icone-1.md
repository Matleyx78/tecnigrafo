# [0.6-P18] `app`: icone, prima parte, e `docs/ICONE.md`

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: disegni; test che ogni icona sia valida, prova
  manuale
- **Dipende da:** P17
- **Modulo:** `app` (risorse) e documentazione
- **Da leggere:** [ADR 0009](../../adr/0009-interfaccia-grafica.md) punto 4;
  `app/src/builtin_actions.cpp` (P7), `app/CMakeLists.txt` (come sono incluse
  le risorse del font)

## File

- `docs/ICONE.md` (nuovo)
- `resources/icons/*.svg` (nuovi; i disegni non contano nel limite dei
  quindici file)
- `resources/icons/README.md` (nuovo: licenza, rimando a `docs/ICONE.md`)
- `app/CMakeLists.txt` (risorse `/tg/icons`; `Qt6::Svg` se serve, vedi sotto)
- `app/src/builtin_actions.cpp` (campo `icon` delle azioni disegnate)
- `tests/app/icons_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `.github/workflows/ci.yml` solo per il modulo SVG di Qt (lo approva il
  responsabile)

## Cosa fare

### Le regole (`docs/ICONE.md`)

- Griglia 24×24 con 2 px di margine; tratto 1,5 px, estremità e giunzioni
  arrotondate; nessun riempimento salvo i punti (cerchi pieni di 3 px).
- Un solo colore, `currentColor`: il tema lo sostituisce. Il secondo colore
  d'accento (classe `accent`) solo per un punto agganciato o un elemento
  attivo.
- Niente testo nelle icone; niente forme copiate da altri programmi o raccolte.
- Nome del file uguale al campo `icon` dell'azione (`draw-line.svg`).
- Licenza: quella del progetto (GPL-3.0-or-later), intestazione SPDX in un
  commento XML.

### Le icone di questa parte

Disegno e modifica, file e annullamento: `draw-line`, `draw-pline`,
`draw-circle`, `draw-arc`, `draw-text`, `edit-erase`, `edit-undo`,
`edit-redo`, `file-new`, `file-open`, `file-save`, `file-saveas`.

### SVG in Qt

Le icone SVG richiedono il modulo SVG di Qt (componente `Svg`, pacchetto
`qt6-svg-dev` su Ubuntu; con `install-qt-action` si verifica che ci sia).
È un componente di Qt, non una libreria nuova (ADR 0002). Se la CI non lo
installa, aggiungilo a `.github/workflows/ci.yml` e scrivilo nella pull
request.

## Regole della tappa che valgono qui

- Le icone sono **nostre**: nessuna copia, nemmeno ridisegnata a mano da
  un'altra raccolta (ADR 0009, punto 4).
- `builtin_actions.cpp`, `app/CMakeLists.txt`: solo le tue righe.

## Test richiesti

- Ogni file in `resources/icons/` è un SVG valido 24×24 che `QSvgRenderer`
  legge; usa solo `currentColor` e la classe `accent`.
- Ogni azione con `icon` ha il suo file.

## Fatto quando

- `docs/ICONE.md` e le dodici icone esistono; i test passano.
- La CI è verde.
