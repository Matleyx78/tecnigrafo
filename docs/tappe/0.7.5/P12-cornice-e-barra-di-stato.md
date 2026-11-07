# [0.7.5-P12] `app`: ribbon compatto, scheda *File*, barra di accesso rapido, barra di stato a icone

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: aspetto dell'interfaccia; test Qt, galleria di
  schermate e prova manuale
- **Dipende da:** P11
- **Modulo:** `app`
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punti 1 (aree 1, 2, 3 e 10), 3, 4 e 5;
  [ADR 0009](../../adr/0009-interfaccia-grafica.md), punti 1 e 2;
  `app/src/ui_builder.hpp`, `app/src/ui_builder.cpp`, `app/src/main_window.hpp`,
  `app/src/main_window.cpp` (costruzione, `createStatusBar`, `createModeButton`),
  `app/src/builtin_actions.cpp`; la documentazione di SARibbon 2.9.5 (stile
  compatto, `SARibbonMainWindow`, barra di accesso rapido, finestra senza cornice)

## File

- `app/src/ui_builder.hpp`, `app/src/ui_builder.cpp`
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/builtin_actions.cpp` (dove le azioni dicono il loro posto nella barra
  di accesso rapido e nella barra di stato)
- `app/src/action_registry.hpp` (un campo per la barra di accesso rapido, se
  serve)
- `tests/app/ribbon_smoke_test.cpp`, `tests/app/drawing_aids_test.cpp`,
  `tests/app/ui_builder_test.cpp`

## Cosa fare

1. **Ribbon compatto** (stile compatto di SARibbon): nessuna fascia vuota sopra
   le schede; la scheda **File** intera, nel colore d'accento, che apre il menu
   dell'applicazione (nuovo, apri, salva, salva con nome, recenti, esci); la
   scheda corrente come nell'ADR 0039, area 2.
2. **Barra di accesso rapido** con nuovo, apri, salva, annulla, ripeti, icone
   da 16 px senza testo, costruita dal registro delle azioni come il resto.
3. **Finestra senza cornice** (`SARibbonMainWindow`), con il titolo
   `<documento> — Tecnigrafo` e la barra di accesso rapido nella fascia del
   titolo. Prima di fissarla, provala su Windows e Linux (X11 e Wayland, dalla CI
   con `offscreen` e a mano dove puoi): se sotto Wayland non regge (finestra
   che non si sposta, che non si ridimensiona, pulsanti che non rispondono),
   sotto Wayland tieni la cornice del sistema con la barra di accesso rapido
   sotto il titolo. Scrivi nella pull request che cosa hai provato e con quale
   esito: il responsabile lo riporta nell'ADR 0039, punto 4. Se cambiare la
   classe base di `MainWindow` tocca altro (script, MCP, test), dillo nella pull
   request.
4. **Barra di stato a icone** (ADR 0039, area 10): gli interruttori diventano
   pulsanti con l'icona che hanno già (`aids-ortho`, `aids-polar`, `aids-osnap`,
   `aids-lwdisplay`, `ui-ribbon`; P21 le rifà a ruoli) e il suggerimento con il nome e la scorciatoia,
   acceso con il fondo `selection`; ordine da destra: impostazioni, scala,
   interruttori (lwdisplay, osnap, polar, ortho, nell'ordine del riferimento),
   poi il **posto vuoto** per l'indicatore dell'ambiente (ADR 0039, punto 5); a
   sinistra le coordinate. Il pulsante RIBBON diventa un'icona.
5. Nessun pulsante di solo testo dove esiste un'icona.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P10, dopo di te P13 e P16. `ui_builder.*` e
  `builtin_actions.cpp`: dopo di te P19 e P20.
- Interfaccia in inglese; nessuna libreria nuova.
- L'interruttore dell'immissione dinamica non si fa qui: lo aggiunge P16.

## Test richiesti

- `ribbon_smoke_test.cpp`: la scheda *File* esiste e apre il menu
  dell'applicazione; la barra di accesso rapido ha le cinque azioni, nell'ordine.
- `drawing_aids_test.cpp`: ogni interruttore della barra di stato ha un'icona,
  nessun testo, e cambia stato con la sua azione, nei due sensi.
- `ui_builder_test.cpp`: il passaggio tra ribbon e classica funziona ancora,
  con la barra di accesso rapido in entrambe.

## Fatto quando

- La galleria mostra ribbon compatto, scheda *File*, barra di accesso rapido e
  barra di stato a icone come nell'ADR 0039.
- Il comportamento sotto Wayland è scritto nella pull request.
- I test qui sopra esistono e passano.
- La CI è verde.
