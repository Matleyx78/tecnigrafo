# [0.8-PT38] `app`: interruttore e pannello dell'assistente

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: test Qt e prova manuale
- **Dipende da:** PT35, PT37
- **Modulo:** `app`
- **Da leggere:** [ADR 0051](../../adr/0051-canale-mcp-dell-assistente.md),
  punti 3 e 8; [ADR 0009](../../adr/0009-interfaccia-grafica.md) (pannelli
  agganciabili); [ADR 0040](../../adr/0040-icone-a-ruoli.md) e
  [ICONE.md](../../ICONE.md); `app/src/builtin_actions.cpp`,
  `app/src/ui_builder.cpp`, `app/src/main_window.*`, un pannello esistente
  (`app/src/layer_panel.*`); la scheda [PT37](PT37-mcp-canale.md)

## File

- `app/src/assistant_panel.hpp`, `app/src/assistant_panel.cpp` (nuovi)
- `app/src/builtin_actions.cpp`, `app/src/ui_builder.cpp`, `app/src/main_window.cpp`
- le icone dell'interruttore e del pannello (due coppie 16/32) in `resources/`
- `docs/ICONE.md`
- `tests/app/assistant_panel_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Interruttore** *AI Assistant*: un'azione con icona nel ribbon (scheda
   *Manage*, o quella dove stanno gli strumenti del programma) e nella barra di
   stato; accende e spegne il canale (PT37); spento a ogni avvio; il pulsante
   della barra di stato mostra tre stati: spento, acceso, collegato.
2. **Pannello *Assistant*** (agganciabile, nascosto per difetto): stato del
   canale, comando da scrivere nell'assistente (`tecnigrafo --mcp`, con il
   percorso dell'eseguibile, copiabile), e l'elenco delle richieste con ora,
   strumento, prime righe ed esito, dalla più recente; un pulsante *Clear*.
3. Testi con `tr()`.

## Regole della tappa che valgono qui

- `builtin_actions.cpp`, `ui_builder.cpp`, `main_window.*` e `ICONE.md` in
  catena: PT35 → PT37 → PT38 → PT39 → PT45.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `assistant_panel_test.cpp`: l'interruttore accende e spegne il canale; il
  pannello mostra una richiesta arrivata da un client di prova e il suo esito;
  lo stato della barra cambia con il collegamento.
- Il controllo della geometria delle icone passa.

## Fatto quando

- L'utente accende il canale e vede che cosa fa l'assistente; la CI è verde.
