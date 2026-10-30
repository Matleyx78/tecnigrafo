# Casi di verifica

Come si scrive, si esegue e si legge un caso di verifica della modalità di
controllo ([ADR 0011](../adr/0011-modalita-di-controllo.md), punti 6 e 7). Le
regole su chi scrive cosa sono in [AGENTS.md](../../AGENTS.md), sezione
"Modalità di controllo e riferimenti".

## La cartella di un caso

Ogni caso è una cartella `tests/verifica/casi/<area>/<nome>/`, con `<area>` e
`<nome>` fatti di lettere, cifre, `_`, `-` e `.` (per esempio
`casi/pline/coordinate-polari/`). Contiene **una** di queste due cose:

- uno **script**: `script.tgs` ([linguaggio](script.md)) e `atteso.txt`, il
  [rapporto](rapporto.md) atteso; al livello 2 anche `atteso-<nome>.png` per
  ogni `!screenshot <nome>` da confrontare;
- delle **domande**: il solo `domande.tgq` ([catalogo](domande.md)), che
  contiene già le risposte attese.

Una cartella con entrambi, o con nessuno dei due, non è un caso valido.

Nel repository stanno solo i file creati da noi. I file presi da raccolte
pubbliche non si copiano: si elencano in `tests/verifica/sorgenti.json` (vedi
sotto).

### File del caso

Dalla tappa 0.5 un caso può contenere, accanto a `script.tgs` o a
`domande.tgq`, dei **file creati da noi** che lo script apre o le domande
leggono: per esempio un DXF scritto a mano, anche danneggiato apposta, o un
`.cadx` costruito con lo strumento `zip`. Anche in sottocartelle (`dati/`).
Lo script o le domande dicono, con un commento, come è fatto ogni file
creato a mano e perché.

- Le **domande** li leggono con `file=<nome>`, direttamente dalla cartella
  del caso, in sola lettura ([domande.md](domande.md#file)).
- Gli **script** li aprono con un nome relativo
  ([script.md](script.md#file)). Prima di eseguire lo script, `check`
  svuota la cartella `file/` dentro quella di `--output` e ci copia tutto il
  contenuto del caso tranne `script.tgs` e i file `atteso*`: lo script legge
  e scrive lì, sulle copie. Così un caso non cambia mai il repository, e un
  file salvato da un'esecuzione precedente non cambia la successiva. La CI
  allega la cartella di uscita, con i file scritti dagli script, agli
  artefatti.
- I file pubblici di `sorgenti.json` si leggono con `@sources/<nome>` negli
  script e con `source=<nome>` nelle domande, dalla cartella di `--sources`.

### Casi controllati dall'oracolo

Un caso di domande i cui valori attesi vengono da ezdxf
([ADR 0020](../adr/0020-fuzzing-e-oracolo-ezdxf.md), punto 2) lo dichiara
nella **prima riga**:

```
; oracle: ezdxf
D = DXF source=librecad-sample
? count D -> ARC 2; LINE 12
```

La riga è un commento per `tecnigrafo-script`. Il passo `Oracolo ezdxf`
della CI (pacchetto P9) controlla che le risposte scritte nel caso coincidano
con quelle di ezdxf, con le stesse regole di confronto; le domande che ezdxf
non sa dare (per esempio `roundtrip`) si saltano.

## Eseguire un caso

```sh
cmake --preset ci && cmake --build --preset ci
ctest --preset ci -L verifica                     # tutti i casi
ctest --preset ci -R verifica/pline/              # i casi di un'area
./build/ci/script/tecnigrafo-script check tests/verifica/casi/<area>/<nome> --output /tmp/caso \
    --sources build/verifica-sorgenti
./build/ci/script/tecnigrafo-script run <script.tgs> [--report <file>] [--sources <cartella>]
```

- `run` esegue uno script al livello 1 e scrive il rapporto (senza `--report`,
  sullo schermo).
- `check` esegue il caso, lo confronta con il risultato atteso e stampa le
  differenze, fino a 20, con il numero della riga del file atteso. Scrive nella
  cartella di `--output` (per difetto quella corrente) il risultato ottenuto:
  `ottenuto.txt` per uno script, `ottenuto.tgq` per le domande, con le risposte
  del programma al posto di quelle attese.
- Con `--pending` il risultato si capovolge, come per i casi in attesa (vedi
  sotto).
- `--sources <cartella>` è la cartella dei file di `sorgenti.json` già
  scaricati e controllati (in CMake `TG_VERIFICA_SOURCES_DIR`, per difetto
  `build<suffisso>/verifica-sorgenti`). `ctest` la passa sempre. Senza,
  `@sources/<nome>` e `source=<nome>` sono un errore che lo dice, e un caso
  di domande che li usa non è valido.
- `run` legge e scrive i file dello script nella cartella corrente.

Codici di uscita:

| Codice | Significato |
|---|---|
| 0 | superato; con `--pending`, non superato (con un avviso) |
| 1 | non superato; con `--pending`, superato: va tolto da `in-attesa.txt` |
| 2 | caso non valido: file mancante o scritto male, argomenti sbagliati. Vale anche con `--pending` |
| 77 | il caso richiede il livello 2 (`!level 2`): `ctest` lo conta come saltato |

Ciò che il catalogo prevede ma il programma non ha ancora **non** è un caso non
valido: è un caso non superato, codice 1 (con `--pending`, codice 0 e un
avviso). Vale per una domanda non realizzata, per un tipo di entità che arriva
con un pacchetto successivo e per ciò che le specifiche descrivono in anticipo:
una direttiva (`!osnap`, `!polar`, `!scr`), un modo di snap (`intersection`,
`perpendicular`, `tangent`, `quadrant`) o un argomento di domanda (`from=`),
fino al pacchetto che li realizza. Ciò che le specifiche **non** descrivono, un
nome sbagliato o un argomento che non esiste, resta un caso non valido.

## Come si confronta

- **Rapporti**: riga per riga. I numeri sono uguali se differiscono al più di
  `1e-9` (tolleranza assoluta, `!tolerance absolute`) o di una frazione del loro
  valore (`!tolerance relative`, per difetto 0); il resto della riga deve
  coincidere carattere per carattere. Un numero è una sequenza di cifre con
  segno, punto ed esponente facoltativi che non segue una lettera, una cifra,
  `_` o `.`: in `R10` le cifre sono testo. Il terminatore di riga (LF o CRLF),
  un terminatore finale e il BOM non contano; le righe vuote sì.
- **Domande**: come sopra, risposta per risposta, con gli elenchi in qualunque
  ordine ([domande.md](domande.md#le-risposte)).
- **Immagini** (livello 2, solo Linux): un pixel è diverso se un canale
  differisce di più di 8 su 255 (`!tolerance channel`), e il caso fallisce se i
  pixel diversi superano lo 0,1% (`!tolerance pixels`).

Le tolleranze si cambiano solo nel file del caso, cioè solo da chi scrive i
riferimenti.

## `tests/verifica/in-attesa.txt`

L'elenco dei casi il cui lavoro non è ancora unito.

```
# commento
pline/esc-conserva #90
cerchio/domande-base #82
```

- Una riga per caso: `<area>/<nome>`, uno o più spazi, `#` e il numero della
  **issue del lavoro** che lo farà passare. Le righe che iniziano con `#` sono
  commenti; le righe vuote non contano.
- Un caso elencato si esegue con `--pending`: il suo fallimento non blocca la
  CI, ma se **passa** la CI fallisce, con il messaggio di toglierlo dall'elenco.
  Così un lavoro finito non resta mai non verificato.
- La pull request di **riferimenti** aggiunge i suoi casi e le loro righe; la
  pull request del **lavoro** toglie solo le righe dei casi del suo pacchetto, e
  da quel momento i casi sono obbligatori.
- Una riga scritta male, o che nomina un caso che non esiste, è un errore di
  configurazione di CMake.

## `tests/verifica/sorgenti.json`

I file presi da raccolte pubbliche (DXF dalla tappa 0.5, in futuro STEP) non
stanno nel repository: questo file li elenca, e prima dei test CMake li scarica
e ne controlla l'hash (pacchetto P4). La CI li conserva in una cache, così i
test girano senza rete.

```json
{
  "format": 1,
  "sources": [
    {
      "name": "esempio-planimetria",
      "url": "https://example.org/archivio/planimetria-1.2.dxf",
      "version": "1.2",
      "sha256": "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
      "license": "CC-BY-4.0",
      "license-url": "https://creativecommons.org/licenses/by/4.0/"
    }
  ]
}
```

| Chiave | Contenuto |
|---|---|
| `format` | versione del formato di questo file: `1` |
| `sources` | l'elenco dei file, uno per oggetto, con le chiavi che seguono |
| `name` | nome con cui i casi indicano il file: lettere minuscole, cifre e `-`, unico |
| `url` | indirizzo da cui scaricare il file, fisso (non "l'ultima versione") |
| `version` | versione o data del file nella raccolta |
| `sha256` | hash SHA-256 del file, 64 cifre esadecimali minuscole |
| `license` | identificativo SPDX della licenza |
| `license-url` | pagina della licenza, da riportare anche nella pull request |

Si accettano solo file con una licenza compatibile con la GPLv3 che ne permetta
la ridistribuzione ([ADR 0003](../adr/0003-licenza.md)). Nel dubbio, il file
non si usa e lo si scrive nella pull request: la CI controlla l'hash, non la
licenza.

## Scrivere un caso di riferimenti

1. Per ogni condizione "Fatto quando" del pacchetto, almeno un caso; più i casi
   limite (valori nulli, punti coincidenti o quasi allineati, `Esc` a metà
   comando, annulla e ripeti).
2. Valori attesi da calcoli, file pubblici o programmi di riferimento, **mai**
   eseguendo Tecnigrafo: `ottenuto.txt` e `ottenuto.tgq` servono a chi fa il
   lavoro, non a chi scrive i riferimenti.
3. Nel rapporto atteso gli identificativi delle entità non si riusano mai
   ([ADR 0004](../adr/0004-modello-geometrico-2d.md)): un caso che annulla un
   segmento si aspetta un buco nella numerazione.
4. Per i comandi, conviene un `!report` nei punti che il pacchetto descrive
   (dopo ogni domanda importante, prima e dopo `Esc`): la sezione finale da sola
   nasconde gli stati intermedi.
5. Elencare i nuovi casi in `in-attesa.txt` con il numero della issue del
   lavoro.
6. Controllare in locale che i casi siano validi: `ctest --preset ci -L
   verifica` deve passare, e `tecnigrafo-script check <caso> --pending` deve
   scrivere "pending" (o "skipped" per il livello 2), mai "INVALID". Se dà
   "INVALID" per una direttiva, un modo di snap o un argomento che le
   specifiche descrivono, il programma non lo annuncia ancora (gli elenchi sono
   in `script/src/script.cpp` e `script/src/question_table.cpp`): non
   cambiare il caso e scrivilo nella issue.
