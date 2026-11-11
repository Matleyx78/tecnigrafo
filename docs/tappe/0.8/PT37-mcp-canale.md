# [0.8-PT37] `app`: canale locale, ponte `--mcp` e coda comune

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: test Qt con un client locale e prova manuale con
  un assistente vero (ADR 0051, punto 9)
- **Dipende da:** PT35 (stessi file di `app`), PT36
- **Modulo:** `app`
- **Da leggere:** [ADR 0051](../../adr/0051-canale-mcp-dell-assistente.md) per
  intero; [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punto 2;
  `app/src/main.cpp`, `app/src/main_window.*`, `app/src/document_session.*`,
  `app/src/script_session.*` (come uno script guida la sessione attiva),
  `app/src/command_line.*`; la scheda [PT36](PT36-mcp-sessione-prestata.md)

## Perché resta impegnativa

Utente e assistente sulla stessa sessione, sicurezza del canale e tre sistemi
operativi, con decisioni che emergono scrivendo il codice (piano, «Motivi»).

## File

- `app/src/assistant_channel.hpp`, `app/src/assistant_channel.cpp` (nuovi: server
  locale, coda, ospite degli strumenti)
- `app/src/assistant_bridge.hpp`, `app/src/assistant_bridge.cpp` (nuovi: il
  ponte `--mcp`)
- `app/src/main.cpp`, `app/src/main_window.hpp`, `app/src/main_window.cpp`,
  `app/src/command_line.cpp` (la riga `[AI] `)
- `app/CMakeLists.txt` (file comune: `Qt::Network` per `QLocalServer`, che è
  già parte di Qt; nessuna libreria esterna nuova)
- `tests/app/assistant_channel_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Canale** (ADR 0051, punti 2 e 3): `AssistantChannel` con `QLocalServer`,
   nome `tecnigrafo-assistant-<hash dell'utente>`, `UserAccessOption`; un client
   alla volta (il secondo si chiude subito con un errore JSON-RPC
   `Another assistant is connected.`); `start()` e `stop()` (lo spegnimento
   chiude il client). Spento all'avvio. Segnali per l'interfaccia: acceso,
   collegato, richiesta ricevuta ed esito.
2. **Ospite** (PT36): `run`, `report`, `open`, `save` sulla sessione **attiva**
   della finestra; senza documenti, `No drawing is open.` (tranne `open`).
3. **Coda** (punto 6): le richieste in ordine, nel thread dell'interfaccia; una
   richiesta parte solo se il runner attivo è a `Command:`; altrimenti aspetta,
   controllando a ogni fine di comando dell'utente, fino a 30 s, poi
   `The user is running a command.`; mentre gira, la riga di comando e la vista
   non accettano input (lo accodano).
4. **Cronologia** (punto 8): le righe scritte dall'assistente compaiono con
   `[AI] ` davanti (solo nella finestra; il rapporto degli script non cambia).
5. **Ponte** (punto 1): `tecnigrafo --mcp` non crea finestre, si collega al
   server locale e copia le righe fra stdin/stdout e il socket finché uno dei
   due si chiude; se non c'è il server risponde a `initialize` con l'errore
   dell'ADR ed esce con codice 1.
6. Ogni comando dell'assistente è un passo di annullamento come quelli
   dell'utente (punto 7).

## Regole della tappa che valgono qui

- `main_window.*` è in catena: PT35 → PT37 → PT38 → PT39 → PT45.
- File comuni: `app/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `assistant_channel_test.cpp` con un `QLocalSocket` nel test: canale spento,
  nessun collegamento; acceso, `initialize`, `tools/list` con i sei strumenti,
  `run` di `LINE` che crea una linea nella sessione attiva e `U` che la toglie;
  secondo client rifiutato; un comando dell'utente in corso fa aspettare la
  richiesta, che parte quando il comando finisce (con il tempo simulato anche il
  rifiuto dopo 30 s); senza documenti, `No drawing is open.`.
- Il ponte: con il server, le righe passano nei due sensi; senza, l'errore.

## Fatto quando

- Un client locale disegna nella sessione attiva solo con i comandi, e `U`
  annulla; il canale è spento all'avvio e non è una porta di rete.
- I test passano su Windows, Linux e macOS; la CI è verde; la prova manuale con
  un assistente vero resta al responsabile (piano, sezione 7, punto 7).
