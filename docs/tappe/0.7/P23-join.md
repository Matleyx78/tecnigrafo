# [0.7-P23] `cmd`: `JOIN`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R23]` (#…), agente gemini
- **Dipende da:** P21, P22, P25, R23
- **Corretta:** 2026-11-01, con le decisioni del responsabile sui buchi B11 e
  B16 della #464 ([DECISIONI.md](../../DECISIONI.md)); 2026-11-02, con la
  revisione di B11 dopo le prove in AutoCAD (#468); 2026-11-06, con le
  [decisioni del responsabile su *cLose*](https://github.com/Matleyx78/tecnigrafo/issues/375#issuecomment-6684760832)
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 17–22; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punto 7;
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punti 3 e 8; `core/include/tg/core/join.hpp` (P22),
  `core/include/tg/core/tolerance.hpp` (P14);
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-unione.dxf`, messaggi osservati;
  `autocad-prove-a-mano.dxf`; "Che cosa resta da controllare", punti 5, 6 e 7)

## File

- `cmd/include/tg/cmd/commands/join_command.hpp`,
  `cmd/src/commands/join_command.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/join_command_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R23.txt` (da cancellare)

## Cosa fare

**Sequenza interattiva:** una richiesta di selezione (`PromptKind::Selection`)
con la domanda `Select source object or multiple objects to join at once:`, poi:

- il primo oggetto scelto è la sorgente, gli altri seguono nell'ordine in cui
  sono stati scelti (ADR 0035, punto 18);
- `join` decide, con `ToleranceRule::of` del documento;
- la sorgente cambia con `replace`, oppure nasce un'entità con `addCopy` dalla
  sorgente e la sorgente si cancella;
- le entità assorbite (`consumed`) si cancellano; le altre scelte restano e
  contano tra le scartate del messaggio, anche quelle che una scelta mista
  lascia fuori dalla catena (P22, B16);
- **archi che formano un cerchio** (B11, rivista il 2026-11-02, come AutoCAD):
  quando `join` dà `formsCircle`, prima di cambiare qualunque cosa il comando
  chiede

  ```
  Arc segments combined form a circle. Convert to circle? [Yes/No] <Yes>:
  ```

  (testo osservato in AutoCAD e tradotto, ADR 0035, punto 22), con le parole chiave
  *Yes* e *No* e *Yes* predefinito, come `Enter Trim mode option [Trim/No trim]
  <Trim>:` di `FILLET` (P20). Con *Yes* nasce il cerchio di `created` con
  `addCopy` dalla sorgente, la sorgente e gli archi di `consumed` si cancellano,
  in un solo passo di annulla. Con *No*, e con `Esc` (ADR 0035, punto 21),
  niente si unisce e il disegno non cambia, senza passo di annulla; nei
  messaggi osservati in AutoCAD non ce n'è uno dopo *No*.

**Sequenza degli script:**

```
Select source object:
Select lines to join to source:
Select arcs to join to source or [cLose]:
Select objects to join to source:
```

- La seconda domanda dipende dal tipo della sorgente: linee, archi o oggetti
  per le polilinee.
- Per gli archi la scelta è un oggetto per clic. La domanda
  `Select arcs to join to source or [cLose]:` si ripete fino a Invio e
  *cLose* resta disponibile anche dopo aver scelto uno o più archi. Per gli
  altri tipi è una richiesta di selezione (`PromptKind::Selection`).
- `joinToSource` decide, con la regola del documento.
- Quando gli archi coprono il giro intero senza *cLose* (`formsCircle` di
  `joinToSource`), anche questa sequenza fa la domanda del cerchio, con lo
  stesso testo e le stesse risposte di quella interattiva (prova 7 di
  [dxf-di-prova.md](dxf-di-prova.md), 2026-11-02: dopo l'ultimo arco e Invio
  compare la domanda, e con «Sì» il messaggio è lo stesso). Con *cLose* il
  comando crea direttamente il cerchio restituito da `joinToSource`,
  senza domanda di conferma.

**Messaggi**, come quelli di AutoCAD in `autocad-unione.dxf`, in inglese (la
tabella di [dxf-di-prova.md](dxf-di-prova.md), punto 3, ADR 0035 punto 22):

- `<n> lines joined into 1 line`;
- `<n> objects converted to 1 polyline`;
- `<n> segments added to polyline`;
- `<n> arcs joined and converted to circle`, dopo *Yes* alla domanda del cerchio
  (osservato in italiano: «2 archi uniti e convertiti in cerchio»);
- `Arc converted to circle.` dopo *cLose*;
- `<n> arcs joined into 1 arc` e `1 line joined to source`, per un arco o una
  linea che si unisce alla sorgente;
- con scartate, il messaggio dell'esito finisce con `, <m> objects discarded
  from the operation.` (per esempio `2 objects converted to 1 polyline, 1
  object discarded from the operation.`, con `1 object` al singolare);
- `0 objects joined, <n> objects discarded from the operation.`

Un passo di annulla `Join`.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P21.
- Le descrizioni elencano le domande di entrambe le sequenze (ADR 0032, punto 9).
- Nessuna decisione geometrica propria: tutto viene da `join.hpp`.
- Le prove a mano dei punti 5, 6 e 7 di [dxf-di-prova.md](dxf-di-prova.md)
  ("Che cosa resta da controllare") sono fatte (2026-11-02): la 6 ha confermato
  B16; la 5 ha dato la regola di B11 di adesso (P22), e la 7 l'ha confermata,
  anche per la domanda del cerchio nella sequenza degli script. Non c'è niente
  da controllare nella issue.

## Test richiesti

- I casi di `autocad-unione.dxf`, ognuno nella sequenza con cui è stato fatto
  (a, b, c negli script; gli altri interattivi).
- Sorgente polilinea nelle due sequenze; arco con *cLose*; niente da unire;
  una scelta mista (l'esempio di B16 in P22), con la linea staccata che resta;
  annulla e ripeti.
- Archi che formano un cerchio, nella sequenza interattiva (il caso 0°→90° con
  270°→45°, del punto 5 della prova a mano): *Yes* (anche con Invio), con il
  cerchio e un solo passo di annulla, e annulla e ripeti; *No* e `Esc`, che non
  cambiano niente. Archi che si uniscono in un arco più grande (0°→90° con
  200°→300°: 0°→300°) senza domanda.
- Archi che formano un cerchio nella sequenza degli script (sorgente 0°→90° e
  arco 300°→30°, un clic ciascuno e Invio): la stessa domanda, *Yes* e *No*
  come sopra.
- In sequenza script, un arco di centro (0,0), raggio 10, da 0° a 180°,
  seguito da *cLose*: cerchio senza conferma, messaggio
  `Arc converted to circle.`. Con sorgente 0°→60° e secondo arco 90°→150°
  di centro (80,0) e raggio 10, dopo il secondo clic compare ancora
  `Select arcs to join to source or [cLose]:`. Scegliendo *cLose* si ottiene
  il cerchio direttamente con lo stesso messaggio; con Invio si ottiene
  l'arco 0°→150°.

## Fatto quando

- I casi di `[0.7-R23]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
