# [0.8.1-PT14] `cmd`: `-ATTDEF` e `AFLAGS`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT14]` (#…), agente claude
- **Dipende da:** PT15, RT14
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 2; `cmd/src/commands/text_command.cpp`
  (le domande di `TEXT` da riusare); `cmd/src/commands/pedit_command.cpp`
  (`PEDITACCEPT`, un valore della sessione); la cronologia
  `autocad-attributi.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/attdef_command.hpp`,
  `cmd/src/commands/attdef_command.cpp` (nuovi)
- `cmd/src/commands/text_command.cpp`, `cmd/src/text.hpp` (solo se serve
  esporre le domande del punto iniziale)
- `cmd/include/tg/cmd/command.hpp` (file comune: `CommandDefaults::attributeFlags`)
- `cmd/include/tg/cmd/command_protocol.hpp`, `cmd/src/command_runner.cpp`,
  `cmd/src/command_registry.cpp`, `cmd/src/dynamic_input.cpp` (file comuni:
  `PromptKind::Name`)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/attdef_command_test.cpp` (nuovo),
  `tests/cmd/command_runner_test.cpp`, `tests/cmd/command_registry_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `AFLAGS` come valore della sessione (`Enter new value for AFLAGS <16>:`,
   da 0 a 31), che all'avvio vale 16.
2. `-ATTDEF` (alias `ATT`, anche `ATTDEF`) con le richieste dell'ADR 0056,
   punto 2, nell'ordine della cronologia; le domande del testo sono quelle
   qui sotto («Le domande del testo»), che `TEXT` di oggi non ha; l'`ATTDEF` entra nel disegno sul layer e con le proprietà correnti, con
   `lockPosition` (PT15) dal modo. La richiesta dell'etichetta non è di testo
   (ADR 0014, punto 3): nella riga di comando lo spazio vale Invio, come in
   AutoCAD, che con `DUE PAROLE` prende `DUE` e passa `PAROLE` alla richiesta
   dopo (seduta del 2026-11-22). Etichetta vuota: `Tag cannot be null.` e la
   richiesta si ripete, come AutoCAD; una riga con uno spazio che arriva intera
   (script al livello 1): `Invalid tag name.` e la richiesta si ripete.
3. Un passo di annullamento `Attdef`. Esc a metà comando, anche dopo aver
   cambiato un modo o lo stile: nessun `ATTDEF`, nessun passo `Attdef`,
   `AFLAGS` resta quello di prima, e torna lo stile corrente di prima (il
   cambio di *Style* sta nel passo `Attdef`, che non c'è; decisione del
   responsabile sulla revisione della #1026, punto 1).

### Le domande del testo

Decisione del responsabile sulla #954 (scelte A dei punti 1–5;
[DECISIONI.md](../../DECISIONI.md), 2026-11-23, «Domande del testo di
`-ATTDEF` (PT14)»), traduzione in inglese della cronologia
`autocad-attributi.txt` con le parole di AutoCAD in inglese:

1. Dopo il valore (o il predefinito), prima del punto iniziale:
   `Current text style:  "<stile>"  Text height:  <altezza>  Justify:  <giustificazione>`,
   con due spazi dopo ogni due punti e prima di ogni voce, lo stile corrente
   del disegno, l'altezza che propone la domanda dell'altezza con quattro
   decimali (`2.5000`) e la giustificazione con il nome dell'opzione del
   punto 3 (`Left` all'inizio di ogni comando: come `TEXT`, `-ATTDEF` non
   ricorda la giustificazione dell'ultima volta; decisione del responsabile
   sulla revisione della #1026, punto 2).
2. `Specify start point of text or [Justify/Style]:`, con le parole chiave
   `J` e `S`; un punto è il punto iniziale (giustificazione `Left`).
3. *Justify*: `Enter an option [Left/Center/Right/Align/Middle/Fit/TL/TC/TR/ML/MC/MR/BL/BC/BR]:`,
   tutte e quindici, poi la richiesta del punto:
   - `Left`: `Specify start point of text:`;
   - `Center`: `Specify center point of text:`;
   - `Right`: `Specify right endpoint of text baseline:`;
   - `Middle` e `MC`: `Specify middle point of text:`;
   - `TL`, `TC`, `TR`: `Specify top-left point of text:`,
     `Specify top-center point of text:`, `Specify top-right point of text:`;
   - `ML`, `MR`: `Specify middle-left point of text:`,
     `Specify middle-right point of text:`;
   - `BL`, `BC`, `BR`: `Specify bottom-left point of text:`,
     `Specify bottom-center point of text:`, `Specify bottom-right point of text:`;
   - `Align` e `Fit`: `Specify first endpoint of text baseline:` e
     `Specify second endpoint of text baseline:`; *Align* non chiede né
     l'altezza né la rotazione, *Fit* non chiede la rotazione.
   Le altre poi chiedono altezza e rotazione come il punto iniziale.
4. *Style*: `Enter style name or [?] <stile corrente>:` (Invio tiene lo stile
   corrente). Lo stile scelto **diventa quello corrente** del disegno, come in
   AutoCAD, e si ripetono la riga del punto 1, **con il nome nuovo**, e la
   richiesta del punto 2. Un nome che non esiste: `Cannot find text style
   "<nome>".` e la domanda si ripete, come `MTEXT`. Anche `?` è un nome che
   non esiste: `Cannot find text style "?".` e la domanda si ripete, senza
   elenco degli stili (decisione del responsabile sulla revisione della #1026,
   punto 3).

## Regole della tappa che valgono qui

- File comuni: `cmd/include/tg/cmd/command.hpp` (un campo in
  `CommandDefaults`, e la riga di `CommandRunner::setDocumentFile` non serve:
  `defaults` si copia intero), `cmd/src/builtin_commands.cpp`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`.
- `AFLAGS` all'avvio vale 16, come AutoCAD 2021
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del piano
  della 0.8.1», punto 4).

## Test richiesti

- Ognuno dei sei `ATTDEF` di `autocad-attributi`; *Constant* senza richiesta;
  spazi nella richiesta e nel predefinito; etichetta vuota; etichetta con uno
  spazio al livello 1.
- Le domande del testo (decisione della #954): la riga dello stile corrente
  prima del punto iniziale,
  `Current text style:  "Standard"  Text height:  2.5000  Justify:  Left`; *Justify* con `MC` (`Specify middle point of
  text:`), con una delle nove `TL`…`BR` diversa da `MC` e con *Align* (due
  punti, né altezza né rotazione); *Fit* (`Specify first endpoint of text
  baseline:`, `Specify second endpoint of text baseline:`, poi l'altezza e
  nessuna rotazione); *Style* con `ARIAL`, come `TENSIONE` della
  cronologia: la riga ripetuta dice `"ARIAL"` e dopo il comando lo stile
  corrente è `ARIAL`; *Style* con un nome che non esiste (`Cannot find text
  style "<nome>".`, la domanda si ripete); *Style* con `?` (`Cannot find
  text style "?".`, la domanda si ripete).
- Esc a metà comando dopo aver acceso un modo: nessun `ATTDEF`, nessun passo
  `Attdef`, `AFLAGS` invariato.
- Esc a metà comando dopo aver scelto `ARIAL` con *Style* (stile corrente
  `Standard` prima del comando): nessun `ATTDEF`, nessun passo `Attdef`, e lo
  stile corrente torna `Standard`.

## Fatto quando

- I casi di RT14 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/attdef/modi/script.tgs
AFLAGS
0
-ATTDEF

CODICE
Codice componente
K1
2,15
2.5
0
AFLAGS
0
-ATTDEF
C

COSTRUTTORE
ACME
2,7
2.5
0
AFLAGS
0
-ATTDEF
L

SIGLA
Sigla
S1
J
MC
30,5
2.5
10
```

Righe da controllare: in `entities:` tre `ATTDEF` (l'ultimo con
`lock-position=yes`, `horizontal=center vertical=middle`), le richieste dei modi
e del testo in `messages:` (la riga dello stile corrente, `Specify start point
of text or [Justify/Style]:`, l'elenco di *Justify*, `Specify middle point of
text:`), in `undo:` tre passi `Attdef`.

- **Strumenti:** righe normali, su `main`; il campo `lock-position` di PT15.
- **Dipendenze:** PT15 (il campo); nient'altro.
- **Fonte:** `autocad-attributi.txt` (richieste, ordine, *Constant*); la
  decisione della #954 per le domande del testo e l'Esc; la seduta
  del 2026-11-22 per l'etichetta vuota e lo spazio
  ([dxf-di-prova.md](dxf-di-prova.md)).
- **Risposte condivise:** nessuna.
