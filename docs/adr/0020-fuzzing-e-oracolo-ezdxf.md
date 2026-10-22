# ADR 0020: Fuzzing dei lettori e oracolo ezdxf

- **Stato:** Accettato (2026-10-22)
- **Data:** 2026-10-22

## Contesto

Dalla tappa 0.5 Tecnigrafo legge file scritti da chiunque: DXF e archivi
`.cadx` (ADR 0017 e 0019). Un file costruito apposta non deve mai far cadere il
programma. La scheda della tappa 0.5 (idea #106, punti 3 e 5) chiede:

- un **bersaglio di fuzzing** per il lettore DXF, scritto insieme al lettore,
  eseguito in CI per qualche minuto con i sanitizer dell'ADR 0016 a ogni pull
  request che tocca `io`;
- **ezdxf** (Python, licenza MIT) come oracolo: legge i DXF pubblici e dà i
  valori attesi dei casi di verifica, mai collegato a Tecnigrafo (ADR 0011,
  punto 6).

Due vincoli pratici:

- una pull request di riferimenti può toccare solo `tests/verifica/casi/`,
  `tests/verifica/sorgenti.json` e `tests/verifica/in-attesa.txt` (ADR 0011,
  punto 7): non può aggiungere lo script dell'oracolo né il passo della CI,
  come invece prevedeva la scheda;
- non tutti gli agenti possono installare pacchetti Python: le sessioni cloud
  di Claude raggiungono solo GitHub, e la sandbox di Codex può non avere la
  rete.

## Decisione

### 1. Fuzzing

- **Strumento:** libFuzzer, incluso in Clang. Si usa Clang 18 di Ubuntu 24.04,
  lo stesso della CI.
- **Opzione CMake `TG_FUZZ`** (spenta per difetto, solo con Clang): compila i
  bersagli di `tests/fuzz/` con `-fsanitize=fuzzer,address,undefined`.
- **Bersagli:** `fuzz_dxf` chiama `io::readDxf` su un input qualunque;
  `fuzz_cadx` il lettore dei `.cadx`, contenitore ZIP e zlib compresi (ADR
  0019). Ogni lettore nuovo di `io` arriva con il suo bersaglio.
- **Corpus di partenza:** i file di `tests/verifica/sorgenti.json` (scaricati
  come per la verifica) e pochi file piccoli scritti da noi in
  `tests/fuzz/corpus/`.
- **Job della CI `Fuzzing (io)`** su `ubuntu-24.04`: parte solo se la pull
  request cambia `io/`, `third_party/` o `tests/fuzz/`, e fa girare ogni
  bersaglio per 3 minuti. È un controllo obbligatorio come gli altri quando
  parte. Un input che fa cadere il lettore si allega come artefatto e si
  stampa in esadecimale nel log.
- **Ogni crash trovato** si corregge e diventa un caso di verifica permanente,
  scritto da un altro agente come riferimento (ADR 0011, punto 9).
- Il fuzzing di 24 ore resta un criterio della tappa 1.0.

### 2. Oracolo ezdxf

- ezdxf si usa **solo in CI**, con versione e hash fissati in
  `tests/verifica/oracolo/requirements.txt` e installato con
  `pip install --require-hashes`. Non si collega mai a Tecnigrafo e non entra
  nel programma distribuito.
- Lo script `tests/verifica/oracolo/ezdxf_attesi.py` legge un DXF e scrive,
  nella sintassi delle domande al modello (ADR 0011, punto 3), le risposte di
  ezdxf: le entità gestite nel formato del rapporto, i valori grezzi di quota,
  spessore ed estrusione, e il numero di entità non gestite per tipo.
- **Passo `Oracolo ezdxf`** nel job `Verifica`:
  1. per ogni file di `sorgenti.json` stampa nel log le risposte di ezdxf;
  2. per ogni caso che lo dichiara nella prima riga (`; oracle: ezdxf`),
     controlla che le risposte scritte nel caso coincidano con quelle di ezdxf,
     con le regole di confronto della modalità di controllo;
  3. passa `ezdxf audit` su ogni DXF scritto dai casi (anche quelli estratti da
     un `.cadx`): un errore di audit fa fallire il passo.
- **Come si scrivono i riferimenti dai file pubblici.** Chi scrive il pacchetto
  di riferimenti aggiunge il file a `sorgenti.json` e apre la pull request; la
  CI stampa le risposte di ezdxf; l'agente le copia nei casi e pubblica di
  nuovo. Nessun agente installa ezdxf: i valori vengono comunque da ezdxf, mai
  da Tecnigrafo. Il log della CI non è leggibile da una sessione cloud di
  Claude: i riferimenti che usano l'oracolo si assegnano a Gemini o a Codex.
- Lo script e il passo sono **infrastruttura**: li aggiunge un pacchetto di
  lavoro (tappa 0.5, P9), non un pacchetto di riferimenti. Precisa l'ADR 0011,
  punto 6.3, che prevedeva il contrario.

### 3. Cosa resta fuori

- ezdxf non giudica le conversioni proprie di Tecnigrafo (per esempio dal
  sistema dell'oggetto al sistema globale, ADR 0018, punto 5): stampa i
  valori grezzi del file, e chi scrive i riferimenti fa il calcolo a mano.
- L'oracolo non sostituisce la prova manuale con altri CAD alla chiusura della
  tappa: `ezdxf audit` trova gli errori di struttura, non dice se AutoCAD
  apre il file.

## Alternative considerate

- **AFL++ o honggfuzz:** più potenti per campagne lunghe, ma da installare a
  parte; libFuzzer è già in Clang e basta per pochi minuti in CI.
- **Fuzzing con GCC:** GCC non ha un motore di fuzzing integrato.
- **Fuzzing a ogni pull request, anche senza modifiche a `io`:** minuti di CI
  spesi senza ragione.
- **ezdxf installato da chi scrive i riferimenti:** dipende dalla rete
  dell'agente; con il log della CI il risultato è lo stesso e non serve
  installare niente.
- **Oracolo aggiunto dal primo pacchetto di riferimenti:** viola la divisione
  tra riferimenti e lavoro che la CI fa rispettare.

## Conseguenze

- Nascono `tests/fuzz/`, l'opzione `TG_FUZZ`, il job `Fuzzing (io)` e
  `tests/verifica/oracolo/`. Le modifiche a `.github/` le approva il
  responsabile.
- Il job `Verifica` richiede Python e qualche secondo in più.
- Scrivere riferimenti da file pubblici richiede due giri di CI (aggiungere il
  file, poi copiare le risposte); il secondo giro è comunque necessario per
  vedere i casi in verde.
- Un aggiornamento di ezdxf può cambiare le sue risposte: si fa con un
  pacchetto dedicato, che ricontrolla tutti i casi dichiarati "da ezdxf".
