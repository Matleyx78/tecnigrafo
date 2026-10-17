# ADR 0010: Segnalazione di errori e crash

- **Stato:** Proposto
- **Data:** 2026-10-17
- **Idea:** [#62](https://github.com/Matleyx78/tecnigrafo/issues/62)

## Contesto

Quando Tecnigrafo va in errore o si chiude all'improvviso, l'utente non ha un modo
semplice per avvisarci e i dati che servono per il debug vanno persi. Vogliamo che
il programma lo dica all'utente e gli proponga di aprire una issue su GitHub con un
rapporto il più completo possibile.

Vincoli:

- Il progetto è open source: nessun segreto (token, chiavi) può stare dentro il
  programma distribuito, perché chiunque lo estrarrebbe.
- Non vogliamo, per ora, un servizio online da gestire.
- La issue è pubblica: il rapporto non deve contenere dati personali.
- `core`, `render`, `edit` e `cmd` non includono Qt né librerie esterne (ADR 0001).
- Il progetto è in C++20 (ADR 0002), che non ha `std::stacktrace` (arriva con
  C++23): per la traccia dello stack serve una libreria.
- Un **errore intercettato** (eccezione arrivata fino al ciclo degli eventi) lascia
  il programma vivo. Un **crash** (`SIGSEGV`, `SIGABRT`, eccezione non gestita di
  Windows) lascia il processo in uno stato in cui allocare memoria o aprire una
  finestra Qt non è sicuro.

## Decisione

### 1. Invio con issue precompilata

- Il programma **non crea issue da solo**. Il pulsante *Segnala su GitHub* apre il
  browser predefinito (`QDesktopServices::openUrl`) sulla pagina di nuova issue del
  repository, con titolo e testo già compilati tramite i parametri dell'indirizzo.
  L'utente controlla il testo su GitHub e lo invia lui.
- Si usa un modello di issue dedicato, `crash`, con l'etichetta `crash`
  assegnata dal modello stesso. L'indirizzo **non** usa il parametro `labels`,
  perché GitHub lo accetta solo da chi ha il permesso di mettere etichette e per
  gli altri restituisce un errore.
- L'indirizzo ha una lunghezza massima (oltre, GitHub risponde `414`). Il testo
  inserito nell'indirizzo si limita a **6.000 caratteri** dopo la codifica; se il
  rapporto è più lungo, si accorcia la traccia e il log e si scrive nella issue di
  trascinare il file del rapporto completo.
- La finestra offre anche il link *Cerca segnalazioni simili*, che apre la ricerca
  delle issue con la firma del crash (vedi punto 5), per evitare doppioni.
- Il modo di invio è un'interfaccia del modulo `app` (`ReportSender`) con una sola
  realizzazione, `GithubIssueUrlSender`. Un futuro invio tramite servizio
  intermediario sarà una seconda realizzazione, senza toccare la raccolta dei dati.

### 2. Due percorsi: errore intercettato e crash

**Errore intercettato.** `QApplication::notify` si ridefinisce in una sottoclasse e
cattura le eccezioni sfuggite a un gestore di eventi. Il programma registra
l'errore, compone il rapporto e mostra subito la finestra di segnalazione, poi
continua. Lo stesso vale per i messaggi `qCritical`. Annullare l'operazione in corso
in modo coerente con lo storico dell'annullamento resta compito di `edit` e `cmd`:
qui si segnala soltanto.

**Crash.** Si gestisce in tre momenti.

1. **All'avvio**: si compone in memoria la parte fissa del rapporto (punto 3), si
   apre in anticipo il file del crash e si riservano i buffer. Si chiama una volta
   la funzione di traccia per caricarne le parti pigre.
2. **Nel gestore del crash** (gestori di segnale su Linux e macOS per `SIGSEGV`,
   `SIGBUS`, `SIGFPE`, `SIGILL`, `SIGABRT`; filtro delle eccezioni non gestite su
   Windows; `std::set_terminate` per le eccezioni non catturate): solo operazioni
   sicure in quel contesto. Si scrivono sul file già aperto, con `write` o
   l'equivalente di Windows, la parte fissa già pronta, il tipo di crash, gli
   indirizzi grezzi dello stack con modulo e indirizzo relativo, e la cronologia
   dei comandi (punto 4) già presente nei buffer. Niente allocazioni, niente Qt,
   niente risoluzione dei simboli. Poi si ripristina il gestore predefinito e si
   rilancia il segnale, così il sistema operativo chiude il processo come farebbe
   comunque. Un indicatore impedisce di rientrare nel gestore.
3. **All'avvio successivo**: se esiste un file di crash non ancora segnalato, si
   risolvono i simboli (nomi delle funzioni, file e righe, se disponibili), si
   compone il rapporto e si mostra la finestra "L'ultima volta Tecnigrafo si è
   chiuso in modo inatteso".

Se su un sistema la libreria non garantisce una traccia sicura dentro il gestore,
non si tenta una traccia insicura: il rapporto contiene tipo di crash e cronologia
dei comandi, e dice esplicitamente che la traccia manca.

### 3. Contenuto del rapporto

Parte fissa:

- versione del programma, hash del commit, tipo di build, compilatore e versione,
  versione di Qt con cui è compilato e quella in uso;
- sistema operativo e versione (`QSysInfo`), architettura, lingua, piattaforma
  grafica di Qt (per esempio `wayland`, `xcb`, `windows`), numero di schermi e
  rapporto dei pixel.

Parte variabile:

- data e ora (UTC), tempo dall'avvio;
- tipo di errore: testo dell'eccezione, numero e nome del segnale o codice
  dell'eccezione di Windows;
- traccia dello stack: per ogni livello funzione, file e riga se disponibili,
  altrimenti nome del modulo e indirizzo relativo;
- gli ultimi **20 comandi** e le ultime **50 righe** della riga di comando;
- le ultime **100 righe** di log (`qInstallMessageHandler` in un buffer circolare);
- statistiche del disegno aperto: numero di entità per tipo e di livelli;
- il testo libero "Cosa stavi facendo?" scritto dall'utente.

Il **disegno** non si allega mai in automatico. Se serve, lo si chiede nella issue.

### 4. Cronologia dei comandi

La cronologia la tiene il modulo `app`, osservando ciò che passa già dalla riga di
comando (`CommandRunner::setMessageSink` e il testo inserito dall'utente). `cmd`
non cambia. Per essere leggibile dal gestore di crash, la cronologia sta in un
buffer circolare di dimensione fissa, allocato all'avvio, con righe di lunghezza
massima fissa (le righe più lunghe si troncano).

### 5. Firma e titolo

Il titolo della issue è `Crash: <segnale o eccezione> in <prima funzione di
Tecnigrafo nella traccia> (v<versione>)`, per esempio
`Crash: SIGSEGV in tg::edit::Editor::erase (v0.4.0)`. La parte senza versione è la
**firma** usata per cercare segnalazioni simili.

### 6. Privacy

- Prima di mostrare il rapporto si sostituiscono la cartella personale con `~` e il
  nome utente con `<utente>`, in tutto il testo, log compreso.
- I nomi dei file dei disegni diventano `<disegno>.cadx` o `<disegno>.dxf`.
- L'utente vede il rapporto intero prima dell'invio e lo può modificare.
- Niente parte dal programma: l'invio lo fa l'utente sulla pagina di GitHub.
- *Non chiedermelo più* si salva nelle impostazioni e si può riattivare da lì.

### 7. File dei rapporti

- I rapporti si salvano in `QStandardPaths::AppLocalDataLocation`, cartella
  `crash/`, in testo semplice. Si tengono gli ultimi 10.
- Il pulsante *Salva il rapporto* ne fa una copia dove sceglie l'utente.
- Il menu *Help* ha la voce *Apri la cartella dei rapporti*.

### 8. Libreria per la traccia: cpptrace

- La traccia dello stack si ottiene con **cpptrace**
  (<https://github.com/jeremy-rifkin/cpptrace>, licenza MIT, compatibile con la
  GPLv3 per l'ADR 0003). Offre funzioni pensate per i gestori di segnale, che
  registrano solo indirizzi grezzi da risolvere dopo, ed è disponibile su Windows,
  Linux e macOS.
- È ammessa **solo nel modulo `app`**, insieme a ciò che le serve per risolvere i
  simboli (libdwarf e zstd su Linux e macOS, dbghelp su Windows). Su Linux la
  traccia sicura dentro il gestore richiede anche libunwind: è ammessa come
  dipendenza di sistema.
- La versione si fissa (tag o commit) e il modo di includerla si decide nel piano
  della tappa.
- Prima di usarla, un **pacchetto di prova** verifica sui tre sistemi:
  - che si compili senza avvisi con le versioni minime dell'ADR 0002;
  - su quali sistemi la traccia dentro il gestore è sicura;
  - che una traccia registrata in un crash si risolva all'avvio successivo;
  - che GitHub accetti l'indirizzo precompilato con il modello `crash` e il limite
    di 6.000 caratteri.

  Se una verifica fallisce, serve un ADR che cambi la libreria o il metodo, prima
  di procedere.

### 9. Simboli di debug

- Il rapporto contiene **sempre** modulo e indirizzo relativo di ogni livello,
  insieme all'hash del commit: da questi chi sviluppa può risolvere la traccia
  anche quando sulla macchina dell'utente i simboli mancano.
- Come distribuire o archiviare i simboli delle versioni pubblicate (insieme agli
  installatori, o solo negli artefatti della CI) si decide nella tappa 0.9.

## Alternative considerate

- **Token di GitHub dentro il programma**: scartato. Chiunque lo estrarrebbe dal
  programma e potrebbe riempire il repository di spam.
- **Servizio intermediario** (per esempio un Cloudflare Worker con una GitHub App
  autorizzata solo a creare issue): funziona anche per chi non ha un account
  GitHub. Rimandato: richiede un servizio da gestire, difese contro lo spam e
  un'informativa GDPR. L'interfaccia `ReportSender` permette di aggiungerlo dopo
  con un nuovo ADR.
- **Servizi di raccolta crash** (Sentry, GlitchTip, Backtrace): dati fuori da
  GitHub, piani gratuiti limitati o un server da ospitare.
- **Crashpad o Breakpad** (minidump con un processo separato): più robusti, ma molto
  più pesanti da compilare e integrare, e i minidump si leggono solo con strumenti
  dedicati, non dentro una issue.
- **Passare a C++23 per `std::stacktrace`**: non è utilizzabile dentro un gestore di
  segnale e non è ancora completo su tutti i compilatori dell'ADR 0002.
- **Mostrare la finestra dentro il gestore del crash**: scartato, non è sicuro e può
  bloccare il programma invece di chiuderlo.

## Conseguenze

- Le segnalazioni arrivano su GitHub con dati uniformi e con un'etichetta propria,
  facili da cercare e da raggruppare per firma.
- Serve un account GitHub per segnalare. Chi non ce l'ha può salvare il rapporto e
  mandarlo in altro modo.
- Una nuova dipendenza nel modulo `app` (cpptrace e ciò che le serve), da tenere
  aggiornata.
- Va creato `.github/ISSUE_TEMPLATE/crash.md` (o `crash.yml`) e l'etichetta `crash`;
  `.github/` lo modifica solo il responsabile.
- Il gestore di crash va provato in CI con un'opzione di avvio nascosta che provoca
  un crash di proposito, e un test che controlla il rapporto all'avvio successivo.
- Il recupero del lavoro dopo un crash (salvataggio automatico, tappa 1.0) resta un
  tema separato, ma potrà usare lo stesso rilevamento del crash all'avvio.
