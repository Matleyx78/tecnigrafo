# ADR 0017: Modulo `io` e lettura e scrittura del DXF

- **Stato:** Proposto
- **Data:** 2026-10-22

## Contesto

La tappa 0.5 apre e salva i disegni: formato nativo `.cadx` e DXF ASCII
(scheda della [tappa 0.5](../tappe/0.5-file-e-dxf.md)). L'ADR 0006 ha fissato
il modello dati (quello del DXF) e la struttura di `.cadx`; restano da decidere
dove sta il codice, quali versioni del DXF si leggono e si scrivono, come si
tratta la codifica dei caratteri, come si segnalano gli errori e come ci si
difende dai file costruiti apposta per far cadere il programma.

Vincoli:

- `core`, `render`, `edit` e `cmd` restano senza Qt e senza librerie esterne
  (ADR 0001); la modalità di controllo deve poter aprire e salvare file al
  livello 1, senza Qt (ADR 0011).
- Le eccezioni non attraversano i confini tra moduli (CODING_STYLE.md).
- Il DXF arriva da chiunque: è il primo punto in cui Tecnigrafo legge dati che
  non ha scritto (ADR 0016).
- Il lettore e lo scrittore si scrivono nel progetto, dalla specifica pubblica
  di Autodesk (scheda della tappa 0.5).

## Decisione

### 1. Il modulo

- Nuovo modulo **`io`** (`tg::io`), che dipende solo da `core`. Gli strati
  diventano:

  ```
  core <- render <- app
  core <- edit <- cmd <- app
  core <- io   <- cmd
                  cmd <- script
  ```

- `io` legge e scrive DXF e `.cadx` da e verso un `core::Document`; non sa
  niente di finestre, comandi o annullamento. Le finestre di dialogo stanno in
  `app`, i comandi in `cmd` (ADR 0021).
- `io` non usa Qt. L'unica libreria esterna ammessa è quella dell'ADR 0019, per
  i soli archivi ZIP.
- Interfaccia pubblica (i nomi esatti li fissa la scheda del pacchetto):

  ```cpp
  struct ReadResult  { core::Document document; ReadReport report; };
  Result<ReadResult> readDxf(std::string_view bytes);
  Result<std::string> writeDxf(const core::Document&, DxfVersion);
  ```

  dove `Result<T>` contiene il valore oppure un `io::Error`, e `ReadReport`
  dice cosa non è gestito (numero di entità non gestite per tipo, conversioni
  fatte). I byte si leggono e si scrivono attraverso `io::FileAccess` (ADR
  0021), non con percorsi aperti da `io`.

### 2. Versioni

- **Si leggono** i DXF ASCII da AutoCAD 2000 (`AC1015`) ad AutoCAD 2018
  (`AC1032`). Un DXF binario, una versione precedente (R12, `AC1009`, e prima)
  o sconosciuta dà un errore che nomina la versione trovata.
- **Si scrive**:
  - con `SAVE` di un DXF aperto, nella **versione letta**: è il modo sicuro
    perché ciò che Tecnigrafo non capisce resti valido per chi lo ha scritto;
  - con `SAVEAS` e per i disegni nuovi, in **R2018** (`AC1032`);
  - dentro un `.cadx`, sempre in **R2018** (ADR 0006, punto 3): un disegno
    nato da un DXF meno recente si converte. Se la conversione non può
    conservare qualcosa, Tecnigrafo lo dice prima di salvare (ADR 0006,
    punto 2).
- Le cinque entità della tappa (`LINE`, `CIRCLE`, `ARC`, `LWPOLYLINE`, `TEXT`)
  hanno gli stessi gruppi in tutte queste versioni: lo scrittore cambia solo
  intestazione e codifica.
- Un disegno nuovo nasce da uno **scheletro R2018** minimo e valido (tabelle,
  blocchi `*Model_Space` e `*Paper_Space`, dizionari e layout obbligatori),
  scritto da noi e incluso in `io`. La sua validità si controlla con `ezdxf
  audit` in CI (ADR 0020) e a mano in due CAD alla chiusura della tappa.

### 3. Codifica dei caratteri

- Da `AC1021` (AutoCAD 2007) il file è in UTF-8.
- Prima di `AC1021` il testo è nella code page di `$DWGCODEPAGE`. Si leggono
  almeno `ANSI_1252` e `ANSI_1250`, e le sequenze `\U+XXXX` (un carattere
  Unicode qualunque). Una code page non gestita dà un avviso nel rapporto di
  lettura e si legge come `ANSI_1252`, mai in silenzio.
- In memoria il testo è sempre UTF-8. Si scrive in UTF-8 da `AC1021`; nelle
  versioni precedenti nella code page del file, con `\U+XXXX` per i caratteri
  che non ci stanno.

### 4. Numeri

- Si leggono con il punto come separatore decimale, indipendentemente dalla
  lingua del sistema. Un numero non valido, infinito o `NaN` è un errore del
  file.
- Si scrivono con la **rappresentazione più breve che, riletta, dà lo stesso
  valore** (come il rapporto della modalità di controllo, ADR 0011, punto 5).
- Gli angoli sono in gradi nel DXF e in radianti nel modello (ADR 0012): un
  angolo letto e riscritto può differire nell'ultima cifra. Per questo le
  entità gestite si confrontano entro una tolleranza (ADR 0018, punto 6); i
  dati non gestiti si riscrivono invece carattere per carattere.

### 5. Handle e identificativi

- L'handle DXF (gruppo 5, esadecimale) diventa l'`EntityId` (ADR 0006,
  punto 1). Il documento continua la numerazione da `$HANDSEED`, o
  dall'handle più alto più uno se è maggiore, e lo scrittore aggiorna
  `$HANDSEED`.
- Un'entità senza handle ne riceve uno nuovo; un handle doppio dà un errore
  del file. Entrambi i casi si contano nel rapporto di lettura.
- Gli handle delle tabelle, dei blocchi e degli oggetti stanno nello stesso
  spazio: il documento li registra perché un'entità nuova non ne riusi uno.

### 6. Errori

- Gli errori sono **valori** (`io::Error`: tipo, numero di riga del file,
  messaggio in inglese che dice cosa si aspettava e cosa ha trovato); dentro
  `io` le eccezioni non si usano per i dati del file e nessuna esce dal modulo.
- Un file danneggiato produce sempre un errore, mai un crash né un documento a
  metà: o si apre tutto, o non si apre niente.

### 7. Limiti di sicurezza

- Una riga più lunga di 64 KiB, un codice di gruppo fuori dall'intervallo del
  DXF o una sezione non chiusa sono errori del file.
- Il lettore non fa ricorsione sui dati del file e non riserva memoria in base
  a un numero letto dal file (per esempio il numero di vertici, gruppo 90),
  ma solo in base ai dati che arrivano davvero.
- Il lettore ha un bersaglio di fuzzing in CI (ADR 0020).

## Alternative considerate

- **Libreria esterna per il DXF** (libdxfrw, dxflib): esclusa dalla scheda;
  sono pensate per il loro modello, non conservano ciò che non capiscono e ci
  legherebbero alla loro licenza e ai loro errori.
- **Lettura e scrittura in `core`:** `core` diventerebbe enorme e ogni modifica
  al formato toccherebbe il modello.
- **Sempre R2018 anche con `SAVE`:** un file R2000 aggiornato d'ufficio può non
  aprirsi più nel programma che lo ha prodotto, e i dati non gestiti di una
  versione precedente potrebbero non essere validi in R2018. Scelto solo per
  `.cadx`, come fissa l'ADR 0006.
- **Scrivere tutte le versioni da R2000 in poi a scelta (`SAVEAS` con
  formato):** più lavoro e più casi da verificare; si potrà aggiungere dopo.
- **Errori con eccezioni:** il lettore è il codice più esposto ai file
  malevoli; un percorso di errore esplicito si prova meglio e non salta i
  limiti.

## Conseguenze

- Nasce il modulo `io`, da aggiungere in `docs/ARCHITETTURA.md` e al
  `CMakeLists.txt` principale; `cmd` dipende da `io`.
- Lo scheletro R2018 dei disegni nuovi è un file nostro da mantenere: va
  controllato con ezdxf e con altri CAD.
- La conversione a R2018 nei `.cadx` è il punto più delicato della tappa: la
  prova manuale apre in due CAD il `drawing.dxf` di un disegno nato da un R2000.
- Le code page oltre `ANSI_1252` e `ANSI_1250` si aggiungono quando servono,
  con lo stesso schema.
