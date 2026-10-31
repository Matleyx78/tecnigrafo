# Gli strumenti di `tecnigrafo-mcp`

Elenco degli strumenti del server MCP di sviluppo, con argomenti, risultato ed
esempio ([ADR 0027](../adr/0027-server-mcp-di-sviluppo.md)). Per compilarlo e
collegarlo a un agente vedi
[AMBIENTE-SVILUPPO.md](../AMBIENTE-SVILUPPO.md), sezione 7.

`tecnigrafo-mcp` è `tecnigrafo-script` con un canale dal vivo: l'agente
esegue un comando e ne vede subito il risultato, invece di scrivere uno script,
lanciarlo e leggere il rapporto. Non sostituisce la CI: **fa fede solo la CI**
(ADR 0027, punto 7).

## Come parla

- MCP su **stdio**: un messaggio JSON-RPC 2.0 per riga su stdin e stdout, i
  log su stderr. Nessuna porta di rete, nessun socket. Il processo finisce con
  l'input.
- Revisione del protocollo `2026-07-28`, e per i client più vecchi la
  stretta di mano `initialize` della `2025-11-25`. Metodi: `initialize`,
  `notifications/initialized`, `ping`, `tools/list`, `tools/call`; niente
  risorse, prompt o notifiche di modifica.
- Limiti: un messaggio in entrata oltre 4 MiB si rifiuta, e così una richiesta
  con più di 10 000 righe di script; non si esegue nulla.
- Un errore **dello strumento** (riga non valida, argomento mancante o di
  tipo sbagliato, file assente, nome non ammesso) è un risultato con
  `isError: true` e il messaggio. Un errore **di protocollo** (strumento
  sconosciuto, JSON o richiesta non validi) è un errore JSON-RPC, per esempio
  `-32602` per uno strumento sconosciuto.
- Un risultato è un blocco di testo; gli strumenti che rispondono con dati
  strutturati (`list_commands`, `describe_command`, `check_case`, `ask`) danno
  anche lo stesso oggetto come `structuredContent`.

## Le opzioni

```
tecnigrafo-mcp [--root <cartella>] [--sources <cartella>] [--questions]
```

| Opzione | Effetto |
|---|---|
| `--root <cartella>` | la **sola** cartella in cui i comandi dei file leggono e scrivono, e dove `check_case` cerca i casi. Solo nomi relativi: niente `..`, niente percorsi assoluti. Senza `--root`, ogni strumento dei file risponde `File access is not available.` |
| `--sources <cartella>` | i file pubblici di `tests/verifica/sorgenti.json` già scaricati, in sola lettura, come `@sources/<nome>` (vale solo insieme a `--root`) |
| `--questions` | offre anche lo strumento `ask`, per le domande al modello |

Una cartella che non esiste, o un'opzione ripetuta o sconosciuta, si rifiuta
con un messaggio su stderr e il codice di uscita 2. `--help` e `--version`
stampano l'uso e la versione ed escono con 0.

## La sessione dal vivo e gli strumenti separati

Il processo tiene **una sessione dal vivo**: un documento, l'editor e la riga
di comando con tutti i comandi di `cmd`, come al livello 1 di
`tecnigrafo-script` ([script.md](script.md)): stesso eco nella cronologia,
stesse direttive, stesso determinismo (UTC, formattazione inglese europea,
numerazione degli identificativi da 257 in un disegno nuovo). Lo stato resta
tra una chiamata e l'altra: un comando lasciato a metà aspetta la riga
successiva.

| Strumento | Lavora su | Argomenti |
|---|---|---|
| `run`, `report`, `reset`, `open`, `save` | la sessione dal vivo | vedi sotto |
| `list_commands`, `describe_command` | niente: leggono le descrizioni dei comandi | — |
| `run_script`, `check_case`, `ask` | una sessione nuova a ogni chiamata, **separata** da quella dal vivo | vedi sotto |

## Strumenti della sessione

### `run`

Esegue righe di script (`.tgs`, solo direttive del livello 1) nella sessione.

- **Argomenti:** `lines`, array di stringhe, al massimo 10 000. Una riga
  normale è ciò che l'utente scrive sulla riga di comando, seguito da `Invio`;
  la stringa vuota è `Invio` da solo; `;` apre un commento; `!move`, `!click`,
  `!rightclick`, `!esc`, `!snap`, `!ortho`, `!view` e `!report` funzionano
  come in uno script. Una riga non contiene mai un ritorno a capo.
- **Risultato:** le righe scritte nella cronologia **dalla chiamata
  precedente**, le sezioni di un eventuale `!report`, e infine
  `prompt: <la domanda attuale>`.
- **Errore:** una riga non valida, una direttiva del livello 2 (`!key`,
  `!screenshot`, `!level 2`) o troppe righe: nessuna riga si esegue.

```json
{"lines": ["LINE", "0,0", "10,0", ""]}
```

```
Command: LINE
Specify first point: 0,0
Specify next point or [Undo]: 10,0
Specify next point or [Undo]:
prompt: Command:
```

Con `{"lines": ["LINE", "!bogus"]}` il risultato è un errore, e `LINE` non è
partito: `line 2: unknown directive '!bogus'`.

### `report`

Lo stato attuale nel formato del [rapporto](rapporto.md): la riga
`tecnigrafo-report 1` e una sezione `[final]`. Le righe di `messages:` sono
quelle scritte dall'ultimo `report` o `!report`; il resto (`entities:`,
`prompt:`, `undo:`…) è lo stato in quel momento. Nessun argomento.

Dopo l'esempio di `run`:

```
tecnigrafo-report 1

[final]
entities:
  257 LINE start=0,0 end=10,0
messages:
  Command: LINE
  Specify first point: 0,0
  Specify next point or [Undo]: 10,0
  Specify next point or [Undo]:
prompt: Command:
preview:
selection: none
rectangle: none
undo:
  Line
redo:
snap: on
ortho: off
pointer: none
active snap: none
statistics:
  LINE 1
```

### `reset`

Ricomincia da un documento vuoto, come all'inizio di uno script: gli
identificativi ripartono da 257 e le sezioni di `!report` da 1. Nessun
argomento. Risultato: `prompt: Command:`.

### `open` e `save`

Eseguono i comandi `OPEN` e `SAVEAS` con il nome dato, dentro `--root`: il
risultato è quello che darebbe `run` con le due righe, perché passano dai
comandi e non da `io` direttamente.

| Strumento | Argomenti |
|---|---|
| `open` | `path`: il file `.dxf` o `.cadx`, relativo a `--root` |
| `save` | `path`: il file da scrivere (senza estensione, `.cadx`); `replace` (facoltativo, `false`): se il file esiste, `true` risponde `Y` alla domanda se sostituirlo, altrimenti la domanda resta |

```json
{"path": "disegno.dxf"}
```

```
Command: SAVEAS
Save drawing as <Drawing1.cadx>: disegno.dxf
prompt: Command:
```

```
Command: OPEN
Enter name of drawing to open: disegno.dxf
prompt: Command:
```

Con modifiche non salvate `open` si ferma alla domanda se salvarle: si
risponde con `run`. Un nome non ammesso (`../x`, `/etc/x`, `C:x`) dà
l'errore `Invalid file name.`; un nome senza `--root`,
`File access is not available.`

## Strumenti sui comandi

### `list_commands`

Nome, alias e sintesi di ogni comando ([ADR 0026](../adr/0026-descrizione-dei-comandi.md)),
in ordine alfabetico di nome. Nessun argomento.

```json
{"commands": [
  {"name": "-LAYER", "aliases": ["-LA"], "summary": "Creates and renames layers, sets the current layer and changes layer properties."},
  {"name": "ARC", "aliases": ["A"], "summary": "Draws an arc through three points, or from a start point, a center and an end point."}
]}
```

### `describe_command`

La descrizione completa di un comando: alias, sintesi, se seleziona prima gli
oggetti, e ogni domanda che può mostrare, con il tipo di valore, le parole
chiave e se `Invio` da solo è una risposta.

- **Argomenti:** `name`, il nome o un alias, in qualunque caso (`LINE`, `line`,
  `L`).
- **Errore:** `Unknown command "<nome>".`

```json
{"name": "LINE", "aliases": ["L"],
 "summary": "Draws a straight line segment, or a chain of them.",
 "selectsFirst": false,
 "prompts": [
   {"id": "first-point", "text": "Specify first point:", "value": "point",
    "keywords": [], "acceptsEmpty": false},
   {"id": "next-point", "text": "Specify next point or [Undo]:", "value": "point",
    "keywords": [{"name": "Undo", "shortcut": "U"}], "acceptsEmpty": true},
   {"id": "next-point-close", "text": "Specify next point or [Close/Undo]:", "value": "point",
    "keywords": [{"name": "Close", "shortcut": "C"}, {"name": "Undo", "shortcut": "U"}],
    "acceptsEmpty": true}
 ]}
```

Sono le stesse descrizioni di `tecnigrafo-script describe`, con gli stessi
nomi dei tipi di valore, in JSON.

## Strumenti come `tecnigrafo-script`

Lavorano ognuno su una sessione nuova e vuota, **senza toccare** la sessione
dal vivo, e leggono e scrivono i file solo dentro `--root`.

### `run_script`

Esegue un intero script `.tgs` al livello 1.

- **Argomenti:** `script`, il testo dello script (anche con `\n`, al massimo
  10 000 righe).
- **Risultato:** il rapporto completo, con le sezioni di ogni `!report` e la
  sezione `[final]`: **identico**, carattere per carattere, all'uscita di
  `tecnigrafo-script run` sullo stesso testo.
- **Errore:** una riga non valida (`line <n>: <motivo>`), o uno script che
  richiede il livello 2 (`needs level 2: run it with tecnigrafo --script`).

```json
{"script": "LINE\n0,0\n10,0\n\n"}
```

Il risultato è il rapporto mostrato sopra per `report`.

### `check_case`

Esegue un caso di `tests/verifica/casi/` e lo confronta con il risultato
atteso.

- **Argomenti:** `path`, la cartella del caso **relativa a `--root`**. Per
  usarlo si avvia `tecnigrafo-mcp --root tests/verifica/casi` (e `--sources`
  se il caso legge `@sources/<nome>`): il percorso è allora `<area>/<nome>`.
- **Risultato:** un oggetto con `outcome` (`passed`, `not passed`, `invalid`,
  `needs level 2`) e `output`, il testo che stampa `tecnigrafo-script check`
  con le differenze (fino a 20, con il numero di riga del file atteso). Il
  percorso nel testo è quello dato, non quello del computer.
- **Cosa scrive:** il risultato ottenuto (`ottenuto.txt`, `ottenuto.tgq`) va
  in una cartella temporanea che si cancella alla fine, **mai dentro
  `--root`**.
- **Errore:** `Invalid file name.` per un nome non ammesso;
  `File access is not available.` senza `--root`.

```json
{"path": "line/annulla-ripeti"}
```

```json
{"outcome": "passed", "output": "passed: line/annulla-ripeti\n"}
```

Un caso elencato in `tests/verifica/in-attesa/` si esegue lo stesso, senza
l'inversione di `--pending`: se il programma non lo supera ancora, l'esito è
`not passed`.

### `ask`

Solo con `--questions`. Risponde a un file di domande al modello (`.tgq`,
[domande.md](domande.md)) come `tecnigrafo-script check` per un caso di
domande.

- **Argomenti:** `questions`, il testo del file `.tgq`.
- **Risultato:** `outcome` (`passed` o `not passed`), `answers` (il file con
  le risposte ottenute al posto di quelle attese) e `differences`, una per
  ogni risposta diversa, con `line`, `expected` e `obtained`.
- Senza `--questions` lo strumento non compare in `tools/list` e chiamarlo dà
  l'errore JSON-RPC `-32602` `Unknown tool: ask`.

```json
{"questions": "A = LWPOLYLINE vertex=0,0 vertex=10,0 vertex=10,10\n? bbox A -> 0,0 10,10\n? length A -> 99\n"}
```

```json
{"outcome": "not passed",
 "answers": "A = LWPOLYLINE vertex=0,0 vertex=10,0 vertex=10,10\n? bbox A -> 0,0 10,10\n? length A -> 20\n",
 "differences": [{"line": 3, "expected": "99", "obtained": "20"}]}
```

## Il rapporto con `tecnigrafo-script`

`mcp` non ha casi propri: il suo riferimento è `tecnigrafo-script`
(ADR 0027, punto 6). Un test di CI esegue ogni caso di livello 1 di
`tests/verifica/casi/` con `run_script` e `check_case` e lo confronta con
`tecnigrafo-script`: i risultati devono essere identici, anche per i casi
in attesa.

| `tecnigrafo-mcp` | `tecnigrafo-script` | Uguale |
|---|---|---|
| `run_script` | `run <script.tgs>` | il rapporto, carattere per carattere |
| `check_case` | `check <caso> --output <cartella>` | l'esito e le differenze |
| `ask` | `check` su un caso con `domande.tgq` | le risposte e l'esito |
| `list_commands`, `describe_command` | `describe [<comando>]` | le descrizioni, in JSON |
| `run`, `report`, `reset`, `open`, `save` | — | usano le stesse regole del livello 1, ma sulla sessione dal vivo |

Cosa cambia per chi lo usa: i file stanno dentro `--root` e non nella cartella
corrente; i casi si indicano con un percorso relativo a `--root`; i codici di
uscita di `tecnigrafo-script` (0, 1, 2, 77) diventano i valori di `outcome`.

## Regole per gli agenti

- **Chi scrive riferimenti non usa `tecnigrafo-mcp` né `tecnigrafo-script`
  per ricavare un valore atteso**: registrerebbe il comportamento del
  programma, errori compresi (ADR 0011, punto 1; ADR 0027, punto 7;
  [AGENTS.md](../../AGENTS.md)).
- Chi lavora a un pacchetto lo usa per provare e capire; non modifica mai
  `tests/verifica/casi/` né `sorgenti.json`, nemmeno se `--root` punta lì:
  con `--root tests/verifica/casi` `save` scriverebbe dentro i casi, quindi in
  quella cartella si usa solo `check_case`.
- Un caso che sembra sbagliato si segnala con la regola "fermati del tutto"
  di AGENTS.md; non si aggiusta il codice né il riferimento.
- Fa fede solo la CI: un risultato di `tecnigrafo-mcp` non la sostituisce.
