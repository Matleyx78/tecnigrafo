# [0.6-P19] `app`: icone, seconda parte

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: disegni; test che ogni azione abbia un'icona,
  prova manuale
- **Dipende da:** P18
- **Modulo:** `app` (risorse)
- **Da leggere:** `docs/ICONE.md` (P18), le icone di P18 (lo stile da seguire),
  `app/src/builtin_actions.cpp`

## File

- `resources/icons/*.svg` (nuovi)
- `app/CMakeLists.txt` (l'elenco delle risorse)
- `app/src/builtin_actions.cpp` (campo `icon`)
- `tests/app/icons_test.cpp` (il controllo diventa: **ogni** azione ha
  un'icona, salvo `file.recent.*` e `file.quit`)

## Cosa fare

Le icone di tutte le azioni rimaste e di quelle che arrivano nella tappa,
anche prima che il loro pacchetto le usi:

- vista e aiuti: `view-zoom-extents`, `aids-osnap`, `aids-ortho`,
  `aids-snap-endpoint`, `aids-snap-midpoint`, `aids-snap-center`,
  `aids-snap-insertion`, `aids-lwdisplay`;
- interfaccia: `ui-ribbon`, `ui-theme`, `tools-settings`, `help-about`;
- layer e proprietà: `layer-panel`, `layer-on`, `layer-off`, `layer-frozen`,
  `layer-thawed`, `layer-locked`, `layer-unlocked`, `layer-current`,
  `properties-panel`, `quick-select`, `property-color`, `property-linetype`,
  `property-lineweight`.

Le icone degli snap riprendono la forma dei marcatori della vista (quadrato,
triangolo, cerchio, due quadrati, ADR 0008 e 0013), con il punto in colore
d'accento.

## Regole della tappa che valgono qui

- Le regole di `docs/ICONE.md`, senza eccezioni; nessuna icona copiata.
- `builtin_actions.cpp`, `app/CMakeLists.txt`: solo le tue righe. Le azioni
  di layer e proprietà che non esistono ancora ricevono l'icona da P22 e P24.

## Test richiesti

- Il test di P18 esteso: ogni azione registrata ha un'icona, ogni icona è
  valida, nessun file senza azione salvo le icone di layer e proprietà, che
  collegano P22 e P24 (allora il test le include).

## Fatto quando

- Tutte le icone qui sopra esistono e i test passano.
- La CI è verde.
