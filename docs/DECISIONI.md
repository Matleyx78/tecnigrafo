# Decisioni del responsabile

Decisioni del responsabile che cambiano le regole o le tappe, in ordine di data.
Stavano nella sezione "Decisioni" di [ROADMAP.md](ROADMAP.md) fino al
2026-10-31; si sono spostate qui perché la ROADMAP la leggono gli agenti
all'inizio di ogni sessione, e a loro la storia delle decisioni non serve: le
regole che ne derivano sono già in [AGENTS.md](../AGENTS.md),
[LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md) e nei piani delle tappe.

Una decisione nuova si aggiunge in fondo, con la data, un titolo in grassetto e
i rimandi ai documenti che cambia.

- **2026-10-16 — Prova manuale su macOS non obbligatoria.** Il responsabile non
  dispone di un Mac. macOS resta un sistema supportato: la CI lo compila e ne
  esegue tutti i test a ogni pull request, come per Windows e Linux. La prova
  manuale su macOS (scorciatoie con `Cmd`, menu, schermi Retina, trackpad) si fa
  quando un revisore che lo usa è disponibile, e il suo esito, o la sua mancanza,
  si scrive nelle note di rilascio. Vale dalla tappa 0.2 (vedi "Modifiche al
  piano" in [tappe/0.2](tappe/0.2-comandi-e-annullamento.md)).
- **2026-10-16 — Impianto dell'interfaccia nella tappa 0.6.** Il responsabile
  sceglie: ribbon come in AutoCAD, con la possibilità per l'utente di passare
  all'interfaccia classica a menu e barre; pannelli agganciabili; icone tutte
  nostre in un unico stile; tema chiaro o scuro a scelta; scorciatoie
  personalizzabili. Le aree di lavoro si decideranno insieme agli ambienti del 3D.
  Il lavoro entra nella tappa 0.6, che introduce i primi pannelli, secondo
  l'[ADR 0009](adr/0009-interfaccia-grafica.md) (proposto, da accettare
  all'apertura della tappa).
- **2026-10-18 — Modalità di controllo, con la tappa 0.4 come banco di prova.**
  Il responsabile sceglie la verifica automatica proposta nell'[ADR 0011](adr/0011-modalita-di-controllo.md)
  (proposto, da accettare all'apertura della tappa 0.4):
  per ogni pacchetto che cambia il comportamento, un altro agente scrive prima i
  casi di verifica, e il lavoro è finito quando la CI conferma che il programma li
  rispetta. Di norma i riferimenti li scrive Gemini; Claude lavora ai pacchetti
  medi e impegnativi, Codex a quelli facili e medi
  ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md)). La tappa 0.4 comincia con i
  quattro pacchetti che costruiscono il sistema di verifica (ADR 0011, punto 13);
  alla sua chiusura si valuta se il metodo ha alleggerito il lavoro. I file DXF e,
  in futuro, STEP presi da raccolte pubbliche non si copiano nel repository: si
  scaricano con hash fissato (ADR 0011, punto 6). Cambia di conseguenza il rischio
  annotato nella scheda della [tappa 0.5](tappe/0.5-file-e-dxf.md).
- **2026-10-18 — Piano della tappa 0.4 e scelte collegate.** Il piano dettagliato
  della [tappa 0.4](tappe/0.4-entita-base.md) è in preparazione, con gli ADR
  0012–0015 proposti e l'[ADR 0011](adr/0011-modalita-di-controllo.md) esteso
  alle **domande al modello**, che danno riferimenti anche alla geometria non
  ancora raggiungibile dai comandi, in 2D e in futuro per superfici e solidi. Il
  responsabile sceglie: font incluso **Liberation Sans** (licenza OFL, ADR 0013);
  **`Esc` conserva il lavoro già completato** in `LINE`, `PLINE` e `TEXT`, come
  in AutoCAD (ADR 0015, che sostituisce in parte l'ADR 0005); interlinea del
  testo 5/3 dell'altezza, da rivedere. La [tappa 0.5](tappe/0.5-file-e-dxf.md)
  eredita il disegno di tutti gli allineamenti del testo, dell'obliquo e della
  specchiatura, e la gestione di quota, spessore ed estrusione diversi dai
  valori predefiniti (ADR 0012 e 0013).
- **2026-10-18 — Accettazione degli ADR senza attesa.** Il responsabile accetta un
  ADR quando lo ritiene pronto, senza aspettare un tempo minimo; i 7 giorni di
  discussione pubblica restano solo prima di poterlo respingere, perché chi lo
  propone abbia il tempo di sostenerlo. Regola in
  [GOVERNANCE.md](../GOVERNANCE.md); cambia di conseguenza il prerequisito della
  [tappa 0.4](tappe/0.4-entita-base.md).
- **2026-10-19 — Idea #106 approvata per intero.** Il responsabile approva i
  cinque punti nati dall'analisi del metodo di ArtCraft:
  [catalogo di parità](parita/comandi.md) dei comandi di AutoCAD, aggiornato
  dalla pull request di chiusura di ogni tappa; AddressSanitizer e UBSan in CI
  ([ADR 0016](adr/0016-sanitizer-in-ci.md), job `Linux (ASan, UBSan)`);
  cartella di build separata per ogni agente (`TG_BUILD_SUFFIX`, in
  [AGENTS.md](../AGENTS.md)). Fuzzing del lettore DXF ed ezdxf come oracolo
  entrano nella scheda della [tappa 0.5](tappe/0.5-file-e-dxf.md).
- **2026-10-19 — Agenti: CI verde prima di fermarsi, pacchetti più leggeri.**
  Un agente si ferma quando la pull request ha tutti i controlli verdi, e
  aspetta la CI con un solo comando che blocca (`gh run watch`), non con
  controlli ripetuti che consumano token ([AGENTS.md](../AGENTS.md), "Quando
  fermarsi"). Dalla tappa 0.5 il piano divide il lavoro in pacchetti più leggeri:
  una scheda autosufficiente per pacchetto (`docs/tappe/X.Y/PN-<nome>.md`), un
  modulo e al massimo una quindicina di file ciascuno, e per ognuno la fascia di
  modello e l'impegno suggeriti ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
  "Preparare il piano"). La tappa 0.4 finisce con i pacchetti attuali.
- **2026-10-21 — Fasce di modello, Gemini a lotti, controllo locale in un
  comando** (#140). La tabella delle fasce nomina i modelli disponibili e li
  abbina per capacità con una misura indipendente dai produttori (Intelligence
  Index di Artificial Analysis): Gemini 3.8 Flash sta nella fascia leggera e fa
  riferimenti ordinari, revisioni, chiusura della tappa e pacchetti facili; i
  riferimenti con calcoli delicati vanno a un agente di fascia forte
  ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md)). Riferimenti e revisioni
  si fanno a lotti, più pacchetti in una sessione
  ([AGENTS.md](../AGENTS.md), "Lavoro a lotti"), con Antigravity CLI configurato
  per non chiedere conferma a ogni comando
  ([AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md), sezione 6). Le prove locali si
  fanno con `scripts/controlla.sh`, gli stessi passi della CI in un comando, con
  il log completo di ogni passo salvato; la CI non cambia. Vale dalla tappa 0.5.
- **2026-10-22 — Valutazione della modalità di controllo (ADR 0011, punto 13).**
  La tappa 0.4 ha fatto da banco di prova per la modalità di controllo a due
  livelli. Il metodo si è dimostrato efficace: la presenza di casi di verifica
  indipendenti scritti prima del codice ha impedito regressioni, ha reso
  oggettivo il criterio di "fatto" e ha permesso la collaborazione ordinata di
  agenti diversi (Claude, Codex, Gemini) su rami paralleli. I casi contestati
  (in R10, R14, R15, R13) hanno confermato che i riferimenti agiscono da
  specifica eseguibile e si correggono indipendentemente dal codice. Dalla tappa
  0.5 il metodo resta la regola di lavoro ordinaria, con i miglioramenti già
  deliberati (pacchetti più leggeri a schede autosufficienti, fasce di modello
  e lavoro a lotti per i riferimenti).
- **2026-10-22 — Piano della tappa 0.5.** Il piano dettagliato della
  [tappa 0.5](tappe/0.5-file-e-dxf.md) è pubblicato e la tappa è aperta,
  con il milestone `v0.5` e le issue #153–#178, e gli ADR
  [0017](adr/0017-modulo-io-e-dxf.md)–[0022](adr/0022-allineamenti-obliquo-specchiatura-testo.md)
  accettati. Il responsabile sceglie: `SAVE` conserva la versione del DXF
  aperto, disegni nuovi e `SAVEAS` scrivono R2018, e un `.cadx` contiene sempre
  R2018 (conversione dalle versioni precedenti); zlib per la compressione e
  contenitore ZIP scritto da noi, al posto di miniz (ADR 0019); ezdxf solo in
  CI, con le risposte stampate nel log da cui chi scrive i riferimenti le
  copia (ADR 0020), al posto del primo pacchetto di riferimenti previsto dalla
  scheda. Per ora lavora una sola sessione di Claude.
- **2026-10-25 — Descrizione dei comandi e server MCP nella tappa 0.6**
  (idea #224, approvata). Entrano nella [scheda della tappa 0.6](tappe/0.6-layer-e-proprieta.md)
  due lavori: una **descrizione dei comandi** leggibile da una macchina nel
  `CommandRegistry` (richieste, opzioni, tipi di valore), da fare finché i
  comandi sono pochi; e **`tecnigrafo-mcp`**, eseguibile di sviluppo senza Qt e
  non installato, che espone agli agenti il modulo `script` con il protocollo
  MCP, solo su stdio, senza porte di rete. È il canale dal vivo previsto
  dall'[ADR 0011](adr/0011-modalita-di-controllo.md), punto 10, e richiede un
  ADR nuovo. Il responsabile sceglie: per il JSON una **libreria già pronta**
  (per esempio nlohmann/json, licenza MIT), da decidere in quell'ADR come nuova
  dipendenza (ADR 0002); le **domande al modello** `.tgq` si espongono come
  strumenti separati, spenti di norma e attivabili con un'opzione al lancio. Il
  codice degli strumenti MCP sta in una libreria comune, che riuserà il
  programma distribuito quando l'utente potrà pilotare il disegno aperto con il
  proprio assistente IA, solo in locale e con il canale acceso da lui; lì le
  domande al modello non si espongono mai. Questa strada entra nella
  [scheda della tappa 0.8](tappe/0.8-quote-retini-blocchi.md), con un suo ADR;
  i dettagli si fissano quando si prepara quella tappa. Una chat dentro Tecnigrafo e il linguaggio
  naturale nella riga di comando restano possibili ma con l'intenzione di
  scartarli.
- **2026-10-25 — Piano della tappa 0.6.** Il piano dettagliato della
  [tappa 0.6](tappe/0.6-layer-e-proprieta.md) è in preparazione, con gli ADR
  [0023](adr/0023-layer-e-proprieta.md)–[0027](adr/0027-server-mcp-di-sviluppo.md)
  e l'[ADR 0009](adr/0009-interfaccia-grafica.md) accettati. Il
  responsabile sceglie: tratti dei tipi di linea calcolati in `core`, con
  riferimenti propri; tipi di linea predefiniti con i nomi ISO
  (`ACAD_ISO02W100`, `ACAD_ISO10W100`) e le definizioni degli altri
  programmi; rinominare un layer nominato in dati non gestiti aggiorna entità
  e blocchi e avvisa per il resto; temi, icone, scorciatoie e pannelli
  verificati con i test Qt e la prova manuale, senza riferimenti; nlohmann/json
  solo nel modulo `mcp`. I layer bloccati sbiaditi passano alla
  [tappa 0.9](tappe/0.9-stampa-e-distribuzione.md), con la trasparenza.
- **2026-10-25 — Apertura della tappa 0.6.** Il piano dettagliato e le schede
  dei pacchetti della [tappa 0.6](tappe/0.6-layer-e-proprieta.md) sono
  pubblicati (#226), con il milestone `v0.6` e le issue #227–#262. Dopo
  l'accettazione il responsabile conferma tre ritocchi: un comando può non
  avere domande (ADR 0026, punto 3); la generazione continua delle polilinee
  entra nel modello con P10; i pacchetti di `app` vanno uno dopo l'altro.
  P14 dipende anche da P10, perché i suoi riferimenti salvano e riaprono il
  disegno.
- **2026-10-25 — Versione minima di Qt 6.8.** La prova di SARibbon (0.6-P3)
  ha mostrato che la libreria non compila con Qt 6.4 su Linux. Il responsabile
  alza la versione minima a **Qt 6.8** e la CI Linux usa Qt 6.8.3 con
  `install-qt-action`, come Windows e macOS; Debian 12 e Ubuntu 24.04 restano
  fuori dai sistemi con Qt dei pacchetti di sistema
  ([ADR 0028](adr/0028-versione-minima-di-qt.md), che sostituisce in parte l'ADR
  0002). SARibbon resta alla v2.9.5, senza modifiche.
- **2026-10-25 — Handle di un disegno nuovo; ADR corretti prima del codice.**
  Gli handle da 1 a 256 di un disegno nuovo sono riservati agli oggetti di
  partenza, con una mappa fissa che scende da 256, e la prima entità è 257:
  aggiungere un oggetto di partenza non sposta più gli identificativi delle
  entità né i rapporti dei casi di verifica. Un file letto porta i suoi
  handle. Il responsabile corregge direttamente l'ADR 0023, punto 1, perché
  nessun codice lo attua ancora, e aggiunge l'eccezione a `GOVERNANCE.md`:
  un ADR accettato e non ancora attuato si può correggere, con la riga
  **Corretto:** nell'intestazione. Le conseguenze sul piano della tappa 0.6
  (pacchetto R4, schede P4, P10 e P16) sono nella sua sezione "Modifiche al
  piano".
- **2026-10-26 — Tipi di linea ISO verificati con un DXF di AutoCAD.** Il
  pacchetto 0.6-P8 (#234) ha trovato che ezdxf 1.4.5 non ha i tipi
  `ACAD_ISO02W100` e `ACAD_ISO10W100`, e si è fermato come chiedeva la
  scheda. Il responsabile conferma i nomi e le definizioni degli altri
  programmi, e le fa verificare con il DXF di AutoCAD
  `autocad-colori-layer-tipilinea` (ora con entrambi i tipi), letto con ezdxf:
  [ADR 0029](adr/0029-verifica-dei-tipi-di-linea-iso.md), in stato *Proposto*,
  che sostituisce in parte l'ADR 0025, punto 2. Le conseguenze sul piano sono
  nella sezione "Modifiche al piano" della
  [tappa 0.6](tappe/0.6-layer-e-proprieta.md).
- **2026-10-28 — solid-space, il kernel di Tecnigrafo.** Il responsabile decide
  che il kernel geometrico di Tecnigrafo è
  [solid-space](https://github.com/Matleyx78/solid-space), in un repository
  separato ma parte integrante del progetto: OCCT resta la base per ciò che fa
  bene, dietro la facciata di solid-space, che aggiunge predicati robusti,
  solver, naming e robustezza delle operazioni
  ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md), *Proposto*, che
  sostituisce in parte gli ADR 0001 e 0002). Ogni bisogno nasce da Tecnigrafo
  come richiesta ([RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md)):
  si cerca quando si prepara il piano, ma può emergere anche durante la tappa;
  comanda il piano di Tecnigrafo, e solid-space risponde con obiettivi
  raggiungibili. Cambiano di conseguenza la scheda della
  [tappa 0.7](tappe/0.7-modifiche.md) (l'ADR delle tolleranze copre solo il
  lato di Tecnigrafo, dopo la valutazione di solid-space) e quella della
  [fase 2](tappe/2.x-modellatore-3d.md) (solver, naming e feature di
  solid-space al posto di OCCT e PlaneGCS collegati direttamente). Le richieste
  della 0.7 si scrivono con il suo piano dettagliato.
- **2026-10-29 — Un flusso unico con solid-space.** Il responsabile cambia la
  cucitura tra i due progetti e corregge l'ADR 0030 (riga **Corretto:**),
  insieme all'ADR 0007 di solid-space. La richiesta si scrive solo in
  solid-space, anche con `scripts/richiesta.sh`
  ([RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md)); a ogni tappa di
  Tecnigrafo che chiede qualcosa corrisponde una tappa di solid-space, e le due
  avanzano insieme; la valutazione porta il contratto (le intestazioni
  pubbliche), e le schede dei pacchetti che ne dipendono si scrivono dopo; il
  sottomodulo entra con un pacchetto P0 all'inizio della 0.7, e lo aggiorna il
  pacchetto che usa il pezzo nuovo; `core` usa anche il nuovo componente
  `geom2d`; i valori geometrici attesi vengono dall'archivio dei casi di
  solid-space. La tappa si apre con `scripts/apri-tappa.sh`, e il Project
  «Tecnigrafo» mostra i due repository sulla stessa tavola. Il piano della 0.7
  si rifà con queste regole: la pull request #330 resta materiale da cui
  attingere.
- **2026-10-29 — Piano della tappa 0.7.** Il piano dettagliato della
  [tappa 0.7](tappe/0.7-modifiche.md) è in preparazione, rifatto con il flusso
  unico a partire dalla bozza della pull request #330, che si chiude senza
  unirla. Gli ADR [0031](adr/0031-modifiche-alle-entita.md)–[0036](adr/0036-tolleranze-di-tecnigrafo.md)
  sono accettati. Le quattro richieste a solid-space
  ([Matleyx78/solid-space#20](https://github.com/Matleyx78/solid-space/issues/20)–[#23](https://github.com/Matleyx78/solid-space/issues/23))
  hanno l'esito 3 e il loro contratto nel nuovo componente `geom2d` (ADR
  0008–0012 di solid-space), quindi tutte le schede sono scritte: la prima metà
  parte all'apertura con P0, che porta il sottomodulo; la seconda usa le
  funzioni di solid-space, e ogni suo pacchetto parte quando è unito il
  pacchetto di solid-space che gli serve. La facciata di `core` passa a
  solid-space l'unità del disegno in metri, da `INSUNITS` (ADR 0036); i
  riferimenti delle facciate prendono i valori geometrici dall'archivio dei
  casi di solid-space e verificano la traduzione, quelli dei comandi i comandi.
  I riferimenti con calcoli delicati di Tecnigrafo dei pacchetti di Claude li
  scrive Gemini 4 Argon, con l'etichetta nuova `agente:gemini-argon` e una
  sessione sua sullo stesso PC di Gemini 3.8 Flash
  ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B7).
  Il responsabile ha preparato in AutoCAD LT sei DXF di prova, da cui vengono i
  riferimenti dei comandi ([tappe/0.7/dxf-di-prova.md](tappe/0.7/dxf-di-prova.md)).
  Prepararli ha mostrato che negli script AutoCAD usa sequenze di domande
  diverse per alcuni comandi: Tecnigrafo le riconosce, insieme alla selezione
  scritta ([ADR 0032](adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)).
  Il responsabile sceglie, il 2026-10-28:
  - `TRIM` ed `EXTEND` nei due modi, rapido e standard;
  - snap a intersezione acceso per difetto;
  - `EXPLODE` che rifiuta le polilinee con quota o spessore;
  - criterio di chiusura riscritto per le entità a cui AutoCAD applica ogni
    comando.

  Entrano nella scheda l'idea [#61](https://github.com/Matleyx78/tecnigrafo/issues/61)
  (finestra chiusa trascinando), `Invio` alla prima domanda di `LINE` e `CIRCLE`
  tangente-tangente-raggio. Le opzioni di `PLINE` *Width*, *Halfwidth*, *Length*
  e le varianti dell'arco passano a dopo la 1.0, perché le larghezze non si
  disegnano ancora. L'idea [#62](https://github.com/Matleyx78/tecnigrafo/issues/62)
  resta per la tappa 0.9. La soglia dei tre punti allineati di `core` resta
  nella 0.7 e si sostituisce dopo con la regola di solid-space (ADR 0036,
  punto 7).
- **2026-10-29 — L'elenco dei casi in attesa.** Il responsabile scrive
  l'eccezione che già si applicava: `tests/verifica/in-attesa.txt` è l'unico
  file che più pacchetti toccano senza essere legati da "Dipende da"
  ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md)); la stessa eccezione è
  scritta in solid-space (Matleyx78/solid-space#30).
- **2026-10-31 — Claude Max, Copilot Pro e la quota di Gemini 4 Argon.** Il
  responsabile ha il piano Max di Claude e Copilot Pro, e la quota settimanale
  di Gemini 4 Argon basta a uno o due pacchetti di riferimenti. Decide, dalla
  tappa 0.8 (la 0.7 resta com'è):
  - Claude prende tutti i pacchetti impegnativi e la maggior parte dei medi, con
    Opus anche per i medi; può lavorare in più sessioni nello stesso momento con
    la sola etichetta `agente:claude` (`agente:claude-2` resta solo sulle issue
    già aperte);
  - Codex scrive i riferimenti con calcoli delicati dei pacchetti di Claude, in
    fascia forte, e fa i pacchetti facili e medi che il piano non dà a Claude;
  - Gemini 4 Argon non ha pacchetti nel piano: se ha quota, il responsabile gli
    passa un pacchetto di Codex cambiandone l'etichetta;
  - Copilot serve solo come revisione mirata delle pull request impegnative,
    chiesta dal responsabile; non prende pacchetti e non apre pull request.

  Regole in [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md) ("Chi fa cosa",
  "Modello e impegno") e in [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md) (B7,
  B9). Dalla stessa analisi nasce l'idea
  [#426](https://github.com/Matleyx78/tecnigrafo/issues/426): la CI controlla i
  riferimenti prima del merge.
- **2026-10-31 — La CI controlla i riferimenti prima del merge.** Il
  responsabile approva l'idea
  [#426](https://github.com/Matleyx78/tecnigrafo/issues/426) e la fa entrare
  fuori tappa, come miglioramento dei test ([ROADMAP.md](ROADMAP.md), "Cosa si
  può fare fuori dalla tappa aperta"). Le regole sono nell'[ADR
  0037](adr/0037-controllo-anticipato-dei-riferimenti.md), che il responsabile
  accetta lo stesso giorno; il lavoro in tre schede in
  [tappe/fuori-tappa/](tappe/fuori-tappa/): la forma dei casi e l'eco degli
  script ([F1](tappe/fuori-tappa/F1-forma-dei-casi.md), #430), la parte già
  verificabile degli script
  ([F2](tappe/fuori-tappa/F2-parte-gia-verificabile.md), #431) e delle domande
  ([F3](tappe/fuori-tappa/F3-domande-gia-verificabili.md), #432). Le richieste
  annunciate, terzo controllo dell'idea, si decidono con il piano della tappa
  0.8.
- **2026-10-31 — L'elenco dei casi in attesa diventa una cartella.** Il
  responsabile sostituisce `tests/verifica/in-attesa.txt` con
  `tests/verifica/in-attesa/`, un file per blocco di riferimenti
  (`0.7-R16.txt`): il pacchetto di riferimenti aggiunge il suo file e quello di
  lavoro lo cancella, quindi nessun file è condiviso e cade l'eccezione alla
  regola "Dipende da" scritta il 2026-10-29, perché con un file solo le pull
  request in parallelo andavano in conflitto una dopo l'altra. È la stessa
  decisione di solid-space (Matleyx78/solid-space#95). L'[ADR
  0011](adr/0011-modalita-di-controllo.md), punti 7 e 9, è corretto con una
  riga **Corretto:** e non sostituito; [AGENTS.md](../AGENTS.md),
  [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), `docs/verifica/` e i piani
  della tappa 0.7 lo seguono. La regola della CI (un caso in lista che passa fa
  fallire la CI) non cambia. Issue
  [#425](https://github.com/Matleyx78/tecnigrafo/issues/425).
- **2026-11-01 — Dipendenze verso solid-space e chiusura delle richieste.** Il
  2026-11-01 R14 (#356) e R22 (#372) della tappa 0.7 risultavano bloccate con
  tutti i loro pacchetti di solid-space già uniti: in "Dipende da" citavano
  ancora le richieste Matleyx78/solid-space#20–#23, che restano aperte fino
  alla fine, e la sostituzione con i pacchetti, prevista dal piano, non aveva
  un momento preciso. Il responsabile decide:
  - le richieste si sostituiscono con i pacchetti di solid-space appena si apre
    la tappa di solid-space, nel piano, nel testo delle issue e nelle relazioni
    «bloccata da» ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B6, passo 5;
    [RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md), punto 3). Per la
    0.7 è fatto lo stesso giorno, con la pull request
    [#449](https://github.com/Matleyx78/tecnigrafo/pull/449); le copie
    storiche #326–#329 sono chiuse;
  - una richiesta si chiude quando sono unite le issue di Tecnigrafo che usano
    per primi il pezzo nuovo, elencate nella colonna «Si chiude con» della
    tabella «Richieste a solid-space» del piano, perché il suo «quando è
    soddisfatta» si osserva nella CI di Tecnigrafo; non più quando sono uniti
    tutti i suoi pacchetti di solid-space (GUIDA-RESPONSABILE.md, B11 e B12;
    [tappe/0000-modello.md](tappe/0000-modello.md));
  - la riga «Sblocca:» delle pull request di solid-space resta un promemoria:
    fa fede "Dipende da";
  - lo script `scripts/dipendenze.py`, fuori tappa come strumento del
    responsabile, controlla e allinea le dipendenze tra piano, issue, relazioni
    e Project, e propone le sostituzioni
    ([F4](tappe/fuori-tappa/F4-dipendenze.md), #450).
- **2026-11-01 — Le schede seguono la revisione del contratto di solid-space.**
  Il responsabile porta in Tecnigrafo la decisione di
  [Matleyx78/solid-space#125](https://github.com/Matleyx78/solid-space/pull/125):
  appena gli ADR di valutazione sono accettati, il pacchetto `[X.Y-C]` di
  solid-space, di un agente diverso da chi ha scritto l'ADR (di norma Codex),
  controlla il contratto con la lista di `docs/CONTRATTI.md`. Per ogni buco
  apre una issue `contratto`; il responsabile decide e una sola pull request
  applica le decisioni. Senza buchi, basta il verbale e il responsabile chiude
  la revisione. Tecnigrafo scrive le schede che usano il contratto solo quando
  `[X.Y-C]` è chiusa, non alla sola accettazione delle valutazioni.
  - Non si prende un pacchetto, anche di riferimenti, se una issue `contratto`
    aperta in solid-space nomina nel titolo un'intestazione che usa.
  - Chi trova un buco si ferma prima di scrivere codice o casi, senza scegliere
    una risposta, e scrive nella issue del proprio pacchetto il testo della
    issue `contratto` da aprire: che cosa manca, un esempio con i dati, le
    scelte possibili. La apre il responsabile e decide lui.
  - Prima della pull request, chi sposta il sottomodulo controlla in
    `docs/DECISIONI.md` di solid-space e nelle intestazioni i cambiamenti dopo
    il commit di partenza della scheda: si adegua o si ferma.
  - Le pull request chiudono le issue con la parola chiave inglese
    `Closes #N`: «Chiude» non le chiude. Questa modifica delle regole non ha
    issue da chiudere.

  Corregge l'[ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 13
  e conseguenze, con una nuova riga **Corretto:**; aggiorna
  [AGENTS.md](../AGENTS.md), [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md)
  (nuovo B3a, senza rinumerare B4–B12),
  [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
  [tappe/0000-modello.md](tappe/0000-modello.md) e
  [RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md). La revisione
  `[X.Y-C]` vale dalla prossima tappa di solid-space; i blocchi e il controllo
  prima della pull request valgono subito, anche per i pacchetti rimasti della
  [tappa 0.7](tappe/0.7-modifiche.md), di cui si chiarisce il passaggio storico
  sulle schede scritte prima della revisione.
- **2026-11-01 — Raccordi, cerchi e spigoli oltre il dominio; curva parallela
  non rappresentabile.** Il responsabile porta nella tappa 0.7 due decisioni di
  solid-space prese dopo che le schede e i riferimenti R17, R19, R20 e R21
  erano scritti:
  - `fillet` e `tangentCircleNear` danno la diagnosi `outOfDomain` sul raggio
    (dato 2) anche quando il centro del cerchio, o lo spigolo del raggio 0,
    cade oltre `kMaxCoordinate`: due rette quasi parallele che si incontrano
    lontanissimo (Matleyx78/solid-space#66, #70, #122, #126; ADR 0011 di
    solid-space, righe **Corretto:**). Tecnigrafo lo tratta con il nuovo
    esito `CornerOutcome::OutOfDomain` in `fillet`, `filletSegments` e
    `circleTangentTangentRadius` (P19); `filletPolyline` lascia il vertice
    com'è e lo conta in `tooShort`. `FILLET` e `CIRCLE` *Ttr* scrivono
    `Lines are nearly parallel.` e non cambiano il disegno (P20). `CHAMFER` dà
    lo stesso esito e lo stesso messaggio quando `intersect` omette lo
    spigolo oltre il dominio (`omittedBeyondDomain`), distinto da `Lines are
    parallel.` (P21). Un raggio scritto dall'utente oltre `kMaxCoordinate`
    dà la stessa diagnosi, ma è un dato fuori dal dominio: la facciata lo
    riconosce prima di chiamare solid-space e dà `None` (ADR 0035, punto 23);
  - `offsetCurve` può dare `failed` per un arco quasi intero che i suoi
    estremi arrotondati e un bulge non rappresentano, e `offsetPolyline` ha il
    nuovo motivo `OffsetFailure::notRepresentable`
    (Matleyx78/solid-space#127). `failed` dà `Failed` anche per linea, arco e
    cerchio, dove il contratto non lo dà mai, perché un arco arriva a
    solid-space come `Arc`; per la polilinea non cambia niente. P17 aspetta
    però Matleyx78/solid-space#130 (0.2-P19, bulge ricostruito, riferimenti
    Matleyx78/solid-space#129): oggi `offsetCurve` sbaglia in silenzio sugli
    archi quasi interi, e R19 di solid-space può spostare di qualche ulp casi
    del suo archivio. In "Dipende da" di P17 la #130 prende il posto della
    Matleyx78/solid-space#63, che ne è una dipendenza. R17 (#362), già chiusa,
    non si riapre: i suoi casi non cambiano.

  Correggono l'[ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punti
  12–15, con una riga **Corretto:**; cambiano le schede
  [P17](tappe/0.7/P17-curva-parallela.md), [P19](tappe/0.7/P19-raccordi-core.md),
  [P20](tappe/0.7/P20-fillet.md) e [P21](tappe/0.7/P21-chamfer.md), il
  catalogo delle domande ([verifica/domande.md](verifica/domande.md), esito
  `out-of-domain`) e il piano della [tappa 0.7](tappe/0.7-modifiche.md) in
  "Modifiche al piano". I riferimenti già uniti non cambiano.
- **2026-11-01 — Buchi nelle schede e revisione delle interfacce.** Gemella
  della decisione di solid-space dello stesso giorno
  (Matleyx78/solid-space#134), dopo che due pacchetti di Codex si sono fermati
  su un buco: R19 di solid-space (Matleyx78/solid-space#132) e P15 qui (#359).
  Il meccanismo di arresto ha funzionato; il problema è a monte, e il buco di
  P15 non sta nel contratto di solid-space ma nella scheda: `intersect` e
  `cutsAlong` decidono esattamente quali tagli sono distinti e il loro ordine,
  ma `partOf` riceve solo punti e stazioni arrotondati. È il nuovo punto 11
  della lista di
  [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  che con il punto 10 (scelte libere entro un errore) vale anche per le
  interfacce di Tecnigrafo. Da qui:
  - **Buco nella scheda.** Quando il contratto di solid-space determina la
    risposta ma un'interfaccia di Tecnigrafo no (intestazione, ADR e scheda
    non la determinano, o si contraddicono), l'agente si ferma come per un
    buco nel contratto, prima di scrivere codice o casi e senza scegliere, e
    scrive nella issue del pacchetto che cosa manca, un esempio e le scelte
    possibili. Nessuna issue in solid-space: decide il responsabile, qui, e la
    scheda si corregge prima di riprendere il pacchetto.
  - **Revisione delle interfacce.** Le schede che introducono tipi o funzioni
    nuove in `core` si controllano con la lista, punti 1–11, prima che i loro
    pacchetti partano: un pacchetto di revisione nella tabella, di un agente
    diverso da chi ha scritto le schede (di norma Codex), da cui quei pacchetti
    dipendono. Nella tappa 0.7 è P25 (#464), per le schede P15–P23, a partire
    da `curve_parts.hpp` di P15.
  - **Correzioni del contratto di solid-space.** Una sua pull request con
    **Revisione del contratto: da fare** si unisce solo dopo il verbale della
    revisione, anche quella che applica le decisioni di un pacchetto
    `[X.Y-C]`.

  Il buco della #359 resta aperto: lo decide il responsabile a parte. Cambiano
  [AGENTS.md](../AGENTS.md) ("Se serve qualcosa a solid-space" e "Lavoro a
  lotti"), [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
  [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md) (A3, B1, B3a, B4 e C),
  [tappe/0000-modello.md](tappe/0000-modello.md) e il piano della
  [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al piano", con la nuova
  scheda [P25](tappe/0.7/P25-revisione-interfacce.md) e la dipendenza da P25
  nelle schede P15–P23. Nessun ADR cambia.
- **2026-11-01 — Buchi delle schede P15–P23: le decisioni.** Il responsabile
  decide il buco di P15 (#359) e i diciotto buchi B1–B18 del verbale della
  revisione delle interfacce (P25, #464), una decisione per commento, sempre
  con la scelta A. Tutti restano in Tecnigrafo: nessuna richiesta a
  solid-space, perché il contratto determina già le risposte.
  - [#359](https://github.com/Matleyx78/tecnigrafo/issues/359#issuecomment-6563748188), P15: `partOf` non deduce niente da punti e stazioni
    arrotondati. Lo stesso posto e il passaggio per l'inizio (vertice 0, angolo
    0) li decide il chiamante, che li ha esatti: dalla lista di
    `HostCuts::cuts` per `TRIM` ed `EXTEND`, dalla costruzione per `FILLET` e
    `CHAMFER`; `partOf` li riceve con un'indicazione esplicita
    (`PartCourse`).
  - [B1](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563748337), P15: gli estremi nuovi sono esattamente i punti ricevuti solo
    per linee e vertici di polilinea; un arco ha gli angoli di `atan2`, entro
    l'errore. Se i due angoli di un arco (o i due vertici di un segmento
    curvo) coincidono, il pezzo corto è vuoto e quello lungo non
    rappresentabile, mai un cerchio: `partOf` ha tre esiti, e nei comandi
    «non rappresentabile» vuol dire operazione non riuscita (ADR 0035, punto
    23). Come Matleyx78/solid-space#132 e #148.
  - [B2](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563748494), P15, P16, P19, P21: il prolungamento è una funzione distinta,
    `extendTo`, che per l'inizio di un arco percorre il cerchio all'indietro
    fino a un taglio `AfterEnd`; il chiamante sceglie fra le due funzioni
    guardando la `Location`.
  - [B3](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563748656), P15, P16: primo e ultimo segmento di una polilinea aperta sono i
    non puntiformi, come in P14; i vertici puntiformi all'estremo seguono il
    nuovo estremo e conservano gli altri dati.
  - [B4](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563765170), P16: un pezzo di polilinea è degenere quando lo sono tutti i
    suoi segmenti, ciascuno con `isDegenerate` del segmento.
  - [B5](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563774381), P17, P18: quando `sideOf` non dà un lato per un'entità che non è
    un testo, l'esito è `Failed`; `NoSide` resta per `Side::On`.
  - [B6](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563795446), P19, P20, P21: *Trim* vale solo fra entità distinte; sui
    segmenti di una polilinea e con *Polyline* la polilinea cambia sempre. Da
    confermare con una prova in AutoCAD prima di P20 (smentita: rivista il
    2026-11-02, vedi la decisione di quel giorno).
  - [B7](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563795913), P19: le tangenti a una polilinea vanno per segmento, un vertice
    comune compare una volta sola, sul segmento che segue, e un segmento
    rifiutato non toglie i punti degli altri.
  - [B8](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563818910), P21: con *Angle* la retta parte dal primo punto verso lo
    spigolo, ruotata dell'angolo verso la parte tenuta della seconda linea;
    una sola distanza zero dà `None`; precondizione in `core` sui valori, e
    messaggio di AutoCAD in `CHAMFER`.
  - [B9](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563819053), P21: un clic proiettato esattamente sullo spigolo dà
    `AmbiguousPick`.
  - [B10](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563871532), P22: con i due estremi di una candidata coincidenti con la
    catena vince il suo inizio.
  - [B11](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563871709), P22, P23: archi dello stesso cerchio danno il cerchio meno il
    vuoto più grande, confrontando gli angoli memorizzati. Da confermare con
    una prova in AutoCAD prima di P22 (smentita: rivista il 2026-11-02, vedi la
    decisione di quel giorno).
  - [B12](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563871873), P22: la polilinea unita scrive nei vertici le larghezze
    effettive e prende dalla sorgente la generazione del tipo di linea.
  - [B13](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563872048), P22: la coincidenza si decide sugli estremi degli archi come
    li calcola Tecnigrafo, con esito libero nella fascia dell'errore, come
    Matleyx78/solid-space#152.
  - [B14](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563795579), P19, P20, P21: con il taglio attivo un `first` o `second`
    vuoto con `Done` cancella l'entità, nella stessa operazione.
  - [B15](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563795747), P19, P21: `filletPolyline` e `chamferPolyline` visitano i
    vertici originali in ordine, ognuno sulla polilinea già modificata.
  - [B16](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563871203), P22, P23: una linea o un arco nascono solo se tutte le entità
    sono linee allineate o archi del cerchio della sorgente; altrimenti la
    catena. Da confermare con una prova in AutoCAD prima di P22.
  - [B17](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563871378), P22: la catena si ferma quando si chiude.
  - [B18](https://github.com/Matleyx78/tecnigrafo/issues/464#issuecomment-6563765305), P16: con `HostCuts::complete` falso e nessun taglio,
    `TRIM` ed `EXTEND` non riescono nei due modi e l'oggetto resta; con dei
    tagli si usano.

  Correggono l'[ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punti
  5, 6, 13, 15, 17 e 18, con una riga **Corretto:**; cambiano le schede
  [P15](tappe/0.7/P15-snap-intersezione.md)–[P23](tappe/0.7/P23-join.md), il
  catalogo delle domande ([verifica/domande.md](verifica/domande.md):
  `offset`, `fillet`, `tangents`, `chamfer` e `join`),
  [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md) ("Che cosa resta da
  controllare", punti 4–6) e il piano della [tappa 0.7](tappe/0.7-modifiche.md)
  in "Modifiche al piano". P25 (#464) si chiude; P15–P23 si riprendono dopo il
  merge.
- **2026-11-02 — B6 e B11 riviste dopo le prove in AutoCAD.** Le prove a mano
  del giorno ([#467](https://github.com/Matleyx78/tecnigrafo/issues/467),
  [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md), punti 4 e 5) hanno smentito due
  decisioni del 2026-11-01 sui buchi del verbale di P25
  ([#464](https://github.com/Matleyx78/tecnigrafo/issues/464)): B6 e B11. Il
  responsabile le rivede ([#468](https://github.com/Matleyx78/tecnigrafo/issues/468))
  scegliendo per tutte e due la **strada A, come AutoCAD**. Restano in
  Tecnigrafo: nessuna richiesta a solid-space.
  - **B6**, P19, P20, P21: *Trim No* vale anche per le polilinee. Con
    `TRIMMODE` 0 AutoCAD lascia la polilinea com'è e il raccordo o lo smusso
    nasce come entità a sé, in tutti e quattro i casi provati (`FILLET` e
    `CHAMFER`, sui due segmenti di una polilinea e con *Polilinea*).
    `filletSegments`, `filletPolyline`, `chamferSegments` e `chamferPolyline`
    ricevono di nuovo `trim`; con *Trim No* la polilinea resta invariata e la
    geometria nuova va in `added`, e per questo `PolylineCorners` ha un elenco
    di entità aggiunte; con *Trim No* ogni vertice si giudica sui segmenti
    originali, perché la polilinea non cambia (la visita di B15 su una
    polilinea che non si modifica); le proprietà dell'arco o della linea
    nuova sono quelle della polilinea (ADR 0035, punto 16: la prima entità
    scelta, che qui è la stessa).
  - **B11**, P22, P23: gli archi dello stesso cerchio si coprono come AutoCAD
    (punto 5: sorgente 0°→90° e arco 270°→45°). Ogni arco si misura in senso
    antiorario dall'inizio della sorgente, da `a` (il suo inizio meno l'inizio
    della sorgente, in [0, 2π)) fino ad `a` più la sua ampiezza, sugli angoli
    memorizzati; il risultato va dall'inizio della sorgente al più lontano di
    questi estremi, in `join` e in `joinToSource`. Se il più lontano arriva al
    giro intero (2π o oltre), gli archi formano un cerchio: `join` lo dice con
    il campo `formsCircle` di `JoinResult` (la forma la fissa la scheda P22), e
    `JOIN` interattivo (P23) chiede `Arc segments combined form a circle.
    Convert to circle? [Yes/No] <Yes>:` (testo inglese, ADR
    0035, punto 22): con *Yes* il cerchio, con *No* niente si unisce e il
    disegno non cambia. *cLose* di `joinToSource` resta com'è. Decadono il
    vuoto più grande e il pareggio fra vuoti di B11; B13 resta dove la catena
    confronta estremi di archi. I casi degli archi di `join/linee-e-archi` (R22)
    danno lo stesso risultato con la regola nuova.
  - **Prova 7**, prima di P22: il responsabile approva la proposta di una prova
    in AutoCAD con due casi sullo stesso cerchio di centro 0,0 e raggio 10
    (sorgente 0°→90° con l'arco 200°→300°, che dà 0°→300°; con l'arco
    300°→30°, che chiede il cerchio) e della domanda del cerchio nella
    sequenza degli script. Se AutoCAD smentisce, B11 si rivede prima di P22.

  Correggono l'[ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punti 13,
  15 e 17–19, con una riga **Corretto:** (nessun codice unito attua i punti
  corretti); cambiano le schede [P19](tappe/0.7/P19-raccordi-core.md),
  [P20](tappe/0.7/P20-fillet.md) e [P21](tappe/0.7/P21-chamfer.md) (B6),
  [P22](tappe/0.7/P22-unione-core.md) e [P23](tappe/0.7/P23-join.md) (B11, e le
  loro "Regole della tappa", che danno per fatte le prove dei punti 4–6), il
  catalogo delle domande ([verifica/domande.md](verifica/domande.md): `fillet`,
  `chamfer` e `join`), [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md) (la prova 7)
  e il piano della [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al
  piano". P19–P21 si riprendono dopo il merge; P22 e P23 dopo il merge e dopo
  la prova 7.
- **2026-11-02 — Prova 7 in AutoCAD: la regola degli archi di B11 è confermata.**
  Il responsabile ha fatto la prova a mano proposta nella
  [#468](https://github.com/Matleyx78/tecnigrafo/issues/468)
  ([dxf-di-prova.md](tappe/0.7/dxf-di-prova.md), punto 7). Sullo stesso cerchio,
  con sorgente 0°→90°: l'arco 200°→300° dà l'arco 0°→300°, e l'arco 300°→30°
  chiede se convertire in cerchio (con «No» non unisce niente e non scrive
  messaggi; con «Sì» dà il cerchio). **La sequenza degli script fa la stessa
  domanda del cerchio**, con lo stesso messaggio finale: P23 la fa in tutte e
  due le sequenze, con *cLose* senza domanda. P22 e P23 non aspettano più
  nessuna prova. Cambiano l'[ADR 0035](adr/0035-taglio-offset-raccordi-unione.md)
  (punto 19 e le righe che aspettavano la prova), le schede
  [P22](tappe/0.7/P22-unione-core.md) e [P23](tappe/0.7/P23-join.md),
  [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md) (l'esito e due script, in
  `dxf-di-prova/`) e il piano della [tappa 0.7](tappe/0.7-modifiche.md).

## 2026-11-02 — Testi inglesi dei messaggi (prova 3)

- Il responsabile chiude la prova 3 di [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md):
  non c'è un AutoCAD inglese, e i messaggi di Tecnigrafo non devono ripetere
  AutoCAD parola per parola. Si traducono i messaggi osservati in italiano
  (compreso `<n> arcs joined and converted to circle`) e si scrivono nello
  stesso stile i pochi che nessuna prova ha mostrato; la tabella è nel punto 3
  di quel file. L'ADR 0035, punto 22, è corretto (nessun codice lo applica
  ancora). I pacchetti P5, P16, P18, P20, P21 e P23 non hanno più testi da
  controllare.
- **2026-11-02 — Un metodo per i buchi trovati dagli agenti.** P22
  ([#373](https://github.com/Matleyx78/tecnigrafo/issues/373)) si è fermato su
  quattro buchi dopo la revisione delle interfacce (P25,
  [#464](https://github.com/Matleyx78/tecnigrafo/issues/464)); due vengono
  dalle decisioni stesse: B10 vale solo quando la catena cresce dalla fine, e
  la regola di B11 rivista può dare un arco con angoli uguali. L'arresto
  funziona; mancava un controllo sulla correzione. Il responsabile fissa il
  metodo, in sei passi
  ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B10a): smistare i buchi
  (scheda, contratto, non si sa), decidere nella issue un buco per commento,
  una sola pull request di correzione, rilettura con la lista, merge,
  ripresa del pacchetto. Le cose nuove sono due:
  - **ogni decisione ha il suo esempio**, con la risposta scelta, nella scheda
    ("Test richiesti"), e una regola si scrive per tutti i casi che tocca; se
    la risposta si vede con una domanda del catalogo, un pacchetto di
    riferimenti con la lettera la mette in un caso;
  - **la correzione si rivede prima del merge**: la pull request comincia con
    **Revisione della correzione: da fare**, e un agente diverso da chi l'ha
    scritta (di norma Codex) rilegge le sole regole nuove o corrette con la
    lista di CONTRATTI.md di solid-space, più due domande (vale anche al
    contrario? che cosa succede quando il calcolo arriva esattamente al
    limite?), con un **Verbale della revisione della correzione**. Il
    responsabile unisce solo dopo il verbale (A3).

  Vale da subito, anche per la correzione dei buchi della #373. Cambiano
  [AGENTS.md](../AGENTS.md) ("Le correzioni delle schede si rivedono"),
  [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md) (A3, tabella di B, B4, il
  nuovo B10a, C) e [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md) (sezione
  5). Nessun ADR cambia.
- **2026-11-02 — Chi rivede una pull request può commentarla.** La prima
  rilettura con il metodo di B10a (Codex sulla
  [#476](https://github.com/Matleyx78/tecnigrafo/pull/476)) si è fermata: il
  controllo automatico dell'agente ha letto «non modificarla» come divieto
  anche dell'etichetta e dei commenti, cioè di tutto ciò che la revisione deve
  scrivere. Le frasi per la rilettura dicono ora che cosa si può fare
  (prendere in carico, commentare, togliere l'etichetta) e che cosa no
  (commit, cambi a file, ramo, titolo o descrizione). Cambiano
  [AGENTS.md](../AGENTS.md) ("Le correzioni delle schede si rivedono") e
  [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md) (A3, frase per la revisione
  del contratto in solid-space; B10a, passo 4). Nessuna regola cambia nella
  sostanza.
- **2026-11-02 — Buchi della scheda P22: le decisioni.** P22
  ([#373](https://github.com/Matleyx78/tecnigrafo/issues/373#issuecomment-6576564686))
  si è fermato su quattro buchi. Il responsabile decide:
  1. **Verso quando la catena cresce dall'inizio: B.** Vince l'inizio della
     candidata, che diventa il vertice comune con la catena: dalla fine si
     aggiunge com'è, dall'inizio si gira. Con un solo estremo coincidente, il
     vertice comune è quello e la candidata si orienta allo stesso modo. B10
     resta, scritta per tutti e due i lati.
  2. **Polilinee chiuse: A.** Una polilinea chiusa diversa dalla sorgente non
     ha estremi liberi, non entra nella catena e si scarta.
  3. **Ordine degli estremi delle linee allineate: C.** L'ordine viene da
     solid-space, con una funzione nuova del contratto, come chiede l'ADR
     0035, punto 1. È un buco nel contratto:
     [Matleyx78/solid-space#158](https://github.com/Matleyx78/solid-space/issues/158),
     in cui il responsabile sceglie l'opzione 2 (dati i segmenti allineati,
     solid-space dà i due estremi del segmento unito, con i pareggi
     nell'ordine dei segmenti). P22 la cita in "Dipende da" e aspetta la pull
     request di solid-space che la applica.
  4. **Arco con angoli uguali: A.** Se `normalizeAngle(σ + il più lontano)`
     dà σ, gli archi formano un cerchio, con `formsCircle` vero, come se il
     più lontano fosse arrivato al giro intero.

  La rilettura di Codex (il
  [verbale](https://github.com/Matleyx78/tecnigrafo/pull/476#issuecomment-6577193793))
  ha trovato un buco nuovo nel punto 4,
  [B476-1](https://github.com/Matleyx78/tecnigrafo/pull/476#issuecomment-6577208775):
  il calcolo comprende la sorgente, quindi un arco sorgente quasi intero
  darebbe il cerchio anche da solo. Il responsabile sceglie **A**: serve almeno
  un altro arco che si unisca; con la sola sorgente il risultato è
  `std::nullopt`, la sorgente non cambia e non compare nessuna domanda.
  *cLose* resta l'eccezione.

  È la prima correzione con il metodo dello stesso giorno (B10a della guida):
  ogni decisione ha il suo esempio in "Test richiesti" della scheda, e la pull
  request si rivede con la lista prima del merge. Correggono
  l'[ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punto 17, con una
  riga **Corretto:**; cambiano la scheda [P22](tappe/0.7/P22-unione-core.md) e
  il piano della [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al piano".
  Il catalogo delle domande e i riferimenti di R22 non cambiano: nessun caso
  cade nei quattro punti.
- **2026-11-03 — Operazione snellimento, passo 3: Tecnigrafo rimanda a
  solid-space.** Durante il congelamento
  ([#478](https://github.com/Matleyx78/tecnigrafo/issues/478)), dopo l'[ADR 0013
  di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md)
  («Esatto dove serve») e i commenti delle sue intestazioni che lo seguono, il
  responsabile decide che i documenti di Tecnigrafo non ripetono più il
  contratto di solid-space: lo citano.
  - **Dove.** I 54 blocchi dell'[inventario di
    Codex](https://github.com/Matleyx78/tecnigrafo/issues/478#issuecomment-6579550558),
    negli ADR [0030](adr/0030-solid-space-kernel-di-tecnigrafo.md),
    [0035](adr/0035-taglio-offset-raccordi-unione.md) e
    [0036](adr/0036-tolleranze-di-tecnigrafo.md) e nelle schede
    [P15](tappe/0.7/P15-snap-intersezione.md)–[P22](tappe/0.7/P22-unione-core.md)
    e [P25](tappe/0.7/P25-revisione-interfacce.md), diventano un rimando
    all'intestazione (la funzione o il tipo), all'ADR di solid-space, a
    CONTRATTI.md o all'archivio dei casi. Le righe **Corretto:** dello 0035 che
    descrivevano solid-space diventano un rimando solo. Restano scritte per
    intero le regole di Tecnigrafo: comandi, messaggi, che cosa si tiene,
    l'ordine della catena di `JOIN`, la proiezione, i pixel, il confronto dei
    casi.
  - **Quale versione.** Un rimando vale nella versione dell'intestazione
    fissata dal sottomodulo, o in quella al cui commit la issue del pacchetto
    lo sposta (ADR 0030, punto 9); chi sposta il sottomodulo rilegge le
    intestazioni come dice AGENTS.md ("Quando fermarsi", punto 1).
  - **Le ripetizioni già diverse dal contratto** si correggono con il rimando:
    vale il contratto. Sono il lato di un arco orario (ADR 0035, punto 10,
    `side.hpp`), la grandezza M e il dominio degli errori (ADR 0036, contesto),
    il confronto dei casi (ADR 0036, punto 4: l'errore di un valore costruito
    e gli esiti liberi nelle fasce, dalla revisione della
    [#479](https://github.com/Matleyx78/tecnigrafo/pull/479)), i casi `failed`
    della curva parallela (P17) e l'attribuzione di B13 alla #152 (P22).
  - **Eccezione redazionale a [GOVERNANCE.md](../GOVERNANCE.md).** Il punto 1
    dello 0035, che P14 attua, e il punto 5 dello 0036 cambiano solo nella
    descrizione: il rimando la sostituisce, la regola resta identica. Il
    punto 4 dello 0036 invece si corregge (sopra), perché contraddiceva il
    contratto. I punti
    6–8 dello 0030, che P0 attua, non cambiano. Ogni ADR toccato ha una riga
    **Corretto:** con la data di oggi.
  - **La [PR #473](https://github.com/Matleyx78/tecnigrafo/pull/473) si chiude
    senza unirla.** Se ne tiene ciò che è di Tecnigrafo e serve ancora: il
    messaggio unico di `OutOfDomain` per `FILLET`, `CIRCLE` *Ttr* e `CHAMFER`,
    **`Result is beyond the drawing limits.`**, al posto di `Lines are nearly
    parallel.` (decisione del responsabile del 2026-11-02, nella chat di
    quella pull request), perché la diagnosi nasce da più costruzioni, non
    solo da rette quasi parallele; la dipendenza di P19 da
    [Matleyx78/solid-space#145](https://github.com/Matleyx78/solid-space/issues/145)
    (0.2-P20, dominio delle tangenze), al posto di #70, già unito; i due
    esempi nuovi dei test di P17 e P19 (un raggio costruito oltre 2^50, due
    parallele lontane). Si lasciano la riga **Corretto:** dello 0035, il
    commit `11108db` come punto di partenza delle schede, la dipendenza di
    P17 da #145 (dopo l'ADR 0013, 0.2-P20 non tocca più `offset.hpp`) e le
    ripetizioni del contratto corretto.
  - **Gli estremi degli archi in `JOIN`** (P22, B13): si prendono da
    `endpoints()` di `curves.hpp` di solid-space, sulla curva della
    traduzione di P14, come chiede il suo commento, e non da un coseno e un
    seno di Tecnigrafo. La scheda diceva «la stessa funzione usata altrove»;
    la fascia vicino al confine della tolleranza resta libera (ADR 0013 di
    solid-space, punto 4). Una diagnosi di `endpoints()`, e un estremo che
    `endpoints()` restituisce oltre `kMaxCoordinate` (possibile per dati
    validi, che `coincident` non accetta), seguono l'ADR 0035, punto 23: in
    `join` e in `joinToSource` un'entità diversa dalla sorgente non si unisce,
    non entra in `consumed` e conta tra le scartate; se è la sorgente, il
    risultato è `std::nullopt`. Il controllo vale per tutti e due gli estremi,
    per coordinate positive e negative e per ogni verso, prima di ogni
    chiamata a `coincident` (revisione della
    [#480](https://github.com/Matleyx78/tecnigrafo/pull/480), R480-1).
  - **P25** ([#464](https://github.com/Matleyx78/tecnigrafo/issues/464)) è
    chiusa dal 2026-11-01 e resta chiusa: la sua scheda rimanda ai punti 10 e
    11 di CONTRATTI.md invece di trascriverli. La rilettura di questa
    operazione la fa la revisione della correzione delle sue pull request,
    non una P25 riaperta.

  Cambiano gli ADR 0030, 0035 e 0036 (righe **Corretto:** del 2026-11-03), le
  schede P15–P22 e P25, il piano della [tappa 0.7](tappe/0.7-modifiche.md)
  ("Modifiche al piano" e la riga di P19), il catalogo delle domande
  ([verifica/domande.md](verifica/domande.md)) e la tabella dei messaggi di
  [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md). La issue #363 (P17) è
  bloccata da Matleyx78/solid-space#130 invece che da #63; la #367 (P19) da
  #145 invece che da #70.
- **2026-11-03 — Operazione snellimento, passo 4: AGENTS.md diventa un nucleo.**
  Durante il congelamento
  ([#478](https://github.com/Matleyx78/tecnigrafo/issues/478)), come in
  solid-space, il responsabile decide che [AGENTS.md](../AGENTS.md) contiene
  solo un nucleo che ogni agente legge sempre, sotto le 1000 parole (dove si
  scrive, presa in carico, regole che non si possono violare, quando fermarsi,
  prima di dichiarare finito, commit e pull request), più una tabella
  "Se ... leggi anche ..." con segni oggettivi: titolo della issue, etichetta,
  cartelle o file toccati, ramo. Il resto delle regole sta in
  [agenti/](agenti/), con gli stessi otto nomi di file nei due repository:
  `pacchetto.md`, `riferimenti.md`, `revisione.md`, `lotti.md`,
  `contratto.md`, `altro-repository.md`, `comandi.md`, `chiusura.md`. Nessuna
  regola si perde: le sezioni conservano i titoli di prima, così i rimandi
  «AGENTS.md, "<titolo>"» degli altri documenti restano validi. `GEMINI.md`
  rimanda al nucleo e alla tabella e tiene solo le note proprie di Gemini.
  Tre regole nuove:
  - **Niente compilazione locale per documenti, commenti e riferimenti.** Una
    pull request che tocca solo documenti, commenti o casi di riferimento non
    si compila e non si eseguono i casi in locale: la verifica la fa la CI;
    `scripts/controlla.sh --solo formato` resta permesso. Per i riferimenti cade la
    possibilità di eseguire i casi in locale; la loro validità la dicono i
    controlli anticipati
    ([ADR 0037](adr/0037-controllo-anticipato-dei-riferimenti.md)).
  - **La fascia di una soglia non blocca.** In revisione, un caso che sta entro
    un errore dichiarato dal confine di `kMaxCoordinate` o da un'altra soglia
    del contratto si chiude dichiarando la fascia
    ([ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
    punto 4), senza bloccare il merge e senza decisione del responsabile: chi
    rivede lo segnala come non bloccante e l'autore scrive la fascia nella
    scheda nella stessa pull request, senza un altro giro.
  - **Buchi visibili e no anche nelle schede.** La regola dell'ADR 0013 di
    solid-space, punto 3, vale anche per i buchi delle schede di Tecnigrafo:
    un buco visibile (esito o diagnosi, numero o ordine delle entità, una
    coordinata oltre la tolleranza della regola) lo decide il responsabile;
    uno non visibile non ferma, lo sceglie chi implementa (la scelta più
    semplice), lo scrive nella scheda o nel commento, lo fissa con un test e
    lo segnala nella pull request.

  Cambiano [AGENTS.md](../AGENTS.md), `GEMINI.md` e i nuovi file di
  [agenti/](agenti/). `CLAUDE.md` non cambia; nessun ADR cambia.
- **2026-11-03 — Operazione snellimento: revisioni, aggiornamenti, regole di
  metodo.** A fine congelamento
  ([#478](https://github.com/Matleyx78/tecnigrafo/issues/478)) il responsabile
  decide, nella chat di coordinamento dell'operazione, tre regole di metodo:
  - **Il secondo giro di una revisione guarda solo il commit di correzione.**
    Se la correzione rimette un testo di `main` o fa esattamente ciò che il
    revisore ha chiesto, basta il controllo del responsabile, senza un altro
    giro.
  - **Una pull request si aggiorna su `main` con un merge.** Sempre «Update
    with merge commit» (o `gh pr update-branch <numero>`), mai il rebase.
  - **Le regole di metodo cambiano solo alla chiusura della tappa**, salvo un
    vero blocco: un lavoro che non può andare avanti con le regole attuali, e
    lo stabilisce il responsabile. Le proposte si raccolgono durante la tappa
    in una issue per tappa, «[X.Y] Proposte di metodo», con l'etichetta
    `proposta`, e si decidono nella pull request di chiusura.

  Cambia [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md) (A3, punti 3 e 7;
  B10a, passo 4; B12), che nella stessa pull request si allinea anche alle
  decisioni dei passi 3 e 4. Nessun ADR cambia.

- **2026-11-03 — Revisione P15: angoli invertiti e identificativo dello snap.**
  Il responsabile decide A:a e B:a nella
  [#487](https://github.com/Matleyx78/tecnigrafo/pull/487#issuecomment-6609308810).
  In [P15](tappe/0.7/P15-snap-intersezione.md) l'inversione si tratta come
  l'uguaglianza di B1, con confronti esatti sugli angoli calcolati, senza
  tolleranza: ampiezza della parte maggiore dell'originale per un arco o un
  segmento curvo; inizio dopo la fine con `Direct`, prima con `AcrossStart`
  per un cerchio. `Empty` per il pezzo esatto corto, `NotRepresentable` per
  il lungo. In una parte di polilinea con più segmenti, il tratto curvo
  invertito prende bulge 0 conservando i punti ricevuti. In `extendTo` il
  nuovo angolo che oltrepassa l'altro estremo dà `NotRepresentable`
  (ampiezza diretta minore dell'originale), a inizio e fine, nei due versi
  dei segmenti curvi. I due esempi della revisione entrano nei test della
  scheda, insieme ai casi opposti. Per `findSnap`, il punto di una coppia
  porta il maggiore dei due identificativi; un'autointersezione porta
  quello della polilinea. Cambiano scheda P15, commenti delle interfacce,
  implementazione e test nella stessa PR; nessun ADR o riferimento cambia.

- **2026-11-03 — P15: correzione angolare minima, decisione C completa.**
  Il responsabile nella [#487](https://github.com/Matleyx78/tecnigrafo/pull/487#issuecomment-6611988842)
  sostituisce le regole caso per caso di **A:a e C:A**, compresa la parità,
  con una sola regola: quando gli angoli calcolati contraddicono `PartCourse`
  o il contenimento del pezzo esatto nell'arco originale, vince la correzione
  angolare minima fra collasso dei due angoli (B1) e riporto di uno o tutti e
  due gli angoli agli estremi originali, oppure al taglio 0/2π del cerchio
  dal lato richiesto da `PartCourse`. Si minimizza la somma degli spostamenti angolari circolari;
  confronti esatti sui valori calcolati, senza soglie; a parità prevale
  l'inizio originale, poi quello del pezzo. Vale per `Arc`, cerchio con
  `Direct` e `AcrossStart`, segmenti curvi nei due versi ed `extendTo`:
  per il prolungamento l'intervallo consentito è il complemento dell'arco,
  con scelta fra prolungamento minimo e chiusura non rappresentabile, sempre
  preferendo l'inizio originale in parità. I casi nuovi della stessa classe
  si risolvono con questa regola. Restano B1 per il collasso e B:a per lo
  snap. Cambiano nella stessa PR implementazione, commenti di `partOf` ed
  `extendTo`, [scheda P15](tappe/0.7/P15-snap-intersezione.md) con esempi e
  test; nessun ADR, riferimento o sottomodulo cambia.

- **2026-11-03 — P15, terzo verbale: fascia del cerchio al taglio 0/2π.**
  Il responsabile [sceglie a](https://github.com/Matleyx78/tecnigrafo/pull/487#issuecomment-6613739609)
  sul punto 1 del [terzo verbale](https://github.com/Matleyx78/tecnigrafo/pull/487#issuecomment-6613603556).
  Quando i due posti esatti di un cerchio stanno entro l'errore dichiarato
  dal taglio 0/2π, da lati opposti, l'esito della correzione angolare minima
  è libero e deterministico, e può essere il pezzo opposto a quello esatto,
  sia con `Direct` sia con `AcrossStart`: è una fascia ai sensi dell'[ADR
  0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
  punto 4. Su `Circle{{0,0},5}`, `Direct` da `(5,-1e-14)` a `(5,1e-14)` dà
  `Done`, arco 0→circa 2e-15 radianti; scambiando i punti, `AcrossStart` dà
  `Done`, arco circa 2e-15→0. Il test fissa questi esiti dentro la fascia,
  non quelli esatti. Il punto 2 allinea la formulazione della decisione C:
  si riportano **uno o tutti e due gli angoli**, come fa già `correctAngles`
  e come fissa `MinimumCorrectionKeepsAlmostTheWholeOriginalArc`.
  Cambiano commento di `partOf`, scheda P15, questa voce e test; algoritmo,
  metrica e spareggi (punto 3), ADR, riferimenti e sottomodulo restano invariati.

- **2026-11-04 — P22: `isStraight` sul segmento d'ingresso.** Codex si è
  fermato su P22 ([#373](https://github.com/Matleyx78/tecnigrafo/issues/373#issuecomment-6627971604))
  perché la scheda non diceva su quale corda chiedere `isStraight` quando un
  estremo del segmento si sposta sul vertice comune («nel punto dell'entità che
  segue»): la freccia, |bulge| · corda / 2, cambia con la corda. Il
  responsabile sceglie **A**: decide il segmento com'è nella sua entità, con
  gli estremi e il bulge con cui entra nella catena (girato se l'entità si
  gira), prima dello spostamento, anche alla chiusura; dalla fine e
  dall'inizio, in `join` e in `joinToSource`, nelle due unità. Nell'esempio di
  Codex, in millimetri, il segmento (0,0)→(1e-7,0) con bulge 10 ha freccia
  5e-7 e diventa dritto, anche se con il vertice comune (1.05e-6,0) la
  freccia sarebbe 5.25e-6. Cambiano la scheda [P22](tappe/0.7/P22-unione-core.md)
  e il piano della [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al
  piano"; nessun ADR cambia (l'ADR 0035, punto 17, non dice su quale corda si
  decide). La decisione cambia una risposta della domanda `join`, quindi si
  aggiunge R22b (Gemini) e P22 dipende anche da lei (guida, B10a, passo 5).

  Sulla revisione della correzione ([#492](https://github.com/Matleyx78/tecnigrafo/pull/492#issuecomment-6629266301)):
  - **R492-1:** un ARC nella catena si tratta come il suo segmento, con gli
    estremi di `endpoints()` e il bulge della sua ampiezza, e `isStraight`
    decide su quello, com'è nella sua entità, con la stessa regola dei
    segmenti di polilinea;
  - **R492-2:** non è una decisione nuova, vale l'ADR 0035, punto 23. Il
    pezzo costruito, con il vertice comune e il bulge deciso in ingresso, si
    controlla; se solid-space lo rifiuta, la candidata non si unisce e conta
    tra le scartate (con una diagnosi sulla sorgente, `std::nullopt`), come
    per R480-1.
  - **R492-3 e R492-4** ([decisione del responsabile](https://github.com/Matleyx78/tecnigrafo/pull/492#issuecomment-6630178016),
    applicazioni di regole che esistono): il pezzo che lo spostamento sul
    vertice comune riduce esattamente a un punto si toglie con i suoi dati, e
    gli altri conservano i loro (ADR 0036, punto 5; scelta C); un ARC che
    solid-space legge come giro intero è un cerchio, non ha estremi liberi e
    non si unisce, come un CIRCLE (scelta A);
  - **fascia del segmento di un ARC** (punto 10 del secondo verbale, non
    bloccante, senza decisione del responsabile; guida, «La fascia di una
    soglia»): quando costruzioni lecite degli estremi e del bulge danno frecce
    dai due lati di epsilon, l'esito di `isStraight` è libero e
    deterministico, e nessun riferimento lo impone.

- **2026-11-04 — Procedura «SITUAZIONE GENERALE».** Quando il responsabile
  scrive «SITUAZIONE GENERALE», l'agente segue la
  [procedura](agenti/situazione-generale.md) e consulta in sola lettura
  Tecnigrafo e solid-space. Il riepilogo indica lo stato dei lavori, che cosa
  li blocca e quali azioni del responsabile possono sbloccarli.

- **2026-11-04 — P19: parte conservata dal raccordo (B2 della
  [#367](https://github.com/Matleyx78/tecnigrafo/issues/367)).**
  Con *Trim* attivo solid-space indica, per ciascuna curva aperta, la parte
  da tenere o l'estremo da prolungare. La issue `contratto`
  [Matleyx78/solid-space#184](https://github.com/Matleyx78/solid-space/issues/184) è decisa e la
  [Matleyx78/solid-space#185](https://github.com/Matleyx78/solid-space/pull/185) la scrive nel contratto: le
  regole sono nel commento di `fillet` in `tangency.hpp` (regola 5, `KeptPart`,
  `Fillet::keepFirst` e `Fillet::keepSecond`) e qui non si ripetono. P19
  dipende da [Matleyx78/solid-space#190](https://github.com/Matleyx78/solid-space/issues/190)
  (0.2-P22), che le realizza, con i riferimenti Matleyx78/solid-space#189
  (0.2-R22). Cambiano la scheda [P19](tappe/0.7/P19-raccordi-core.md) e il
  piano della [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al piano".

  Sull'esito `radiusTooLarge` di `fillet`, deciso nella
  [Matleyx78/solid-space#187](https://github.com/Matleyx78/solid-space/issues/187)
  ([decisione del responsabile](https://github.com/Matleyx78/tecnigrafo/pull/494#issuecomment-6632305280)
  sul buco segnalato nella #494): in Tecnigrafo diventa un esito nuovo,
  `CornerOutcome::RadiusTooLarge`, in `fillet` e `filletSegments`; `FILLET`
  scrive `Radius is too large.` e non cambia il disegno; `filletPolyline`
  lascia il vertice com'è e lo conta in `tooShort`, come gli altri esiti senza
  raccordo. Esempio: L1 (2,0)→(5,0), L2 (0,0)→(0,30), scelte in (3,0) e
  (0,20), raggio 10. Cambiano le schede P19 e [P20](tappe/0.7/P20-fillet.md)
  e la tabella dei messaggi di [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md).

- **2026-11-04 — Che cosa blocca una revisione
  ([#489](https://github.com/Matleyx78/tecnigrafo/issues/489)).** Il revisore
  serve a portare il lavoro al merge, non a fermarlo: blocca solo un errore
  vero o una regola persa. Se una regola esistente risponde, la applica e va
  avanti; i dubbi di forma li scrive come non bloccanti. Prima di dichiarare un
  buco controlla se una regola esistente lo decide già (dati fuori dal dominio,
  [ADR 0035](adr/0035-taglio-offset-raccordi-unione.md), punto 23; pezzi
  degeneri, [ADR 0036](adr/0036-tolleranze-di-tecnigrafo.md), punto 5; fasce,
  ADR 0013 di solid-space, punto 4): se sì, lo scrive come applicazione di
  quella regola e non blocca il merge; con queste regole si trattano anche i
  dati privi di senso pratico (lunghezze sotto la tolleranza, ampiezze di un
  ulp). Una garanzia del contratto si verifica con i test, non con
  dimostrazioni matematiche: il revisore che teme un ramo non coperto chiede un
  test che lo raggiunga, non un lemma. La decisione toglie un blocco vero, che
  fermava P19 (#367); la stessa regola vale in solid-space. Cambia
  [agenti/revisione.md](agenti/revisione.md), "Che cosa blocca una revisione".

- **2026-11-04 — Prova di Copilot su una issue.** Il responsabile assegna a
  Copilot (cloud agent di GitHub Copilot Pro) una issue facile o media, per
  misurare quanti crediti IA consuma. È una prova, non cambia "Chi fa cosa" di
  [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md) né la decisione del
  2026-10-31: se Copilot entrerà nel piano, lo si decide alla chiusura della
  tappa. Il suo ambiente è in `.github/workflows/copilot-setup-steps.yml`; le
  differenze dal nucleo (presa in carico, CI, `Assisted-by`) sono in
  `.github/copilot-instructions.md`.

- **2026-11-05 — Esiti di `FILLET` e `CHAMFER` non riusciti
  ([#507](https://github.com/Matleyx78/tecnigrafo/pull/507),
  [#508](https://github.com/Matleyx78/tecnigrafo/pull/508)).** Tre decisioni
  del responsabile sui casi limite di R20 e R21:
  - dopo un raccordo o uno smusso non riuscito (nessun raccordo, infiniti,
    punto ambiguo, entità non ammessa, raggio o distanza troppo grande, oltre
    il dominio) il comando finisce, senza modifiche; con *Multiple* torna la
    prima domanda. Esempio: due parallele, `CHAMFER` con i clic sulle due,
    `Lines are parallel.` e `Command:`;
  - una distanza di smusso, o la lunghezza, più lunga della parte tenuta della
    sua linea dà il messaggio di AutoCAD, `Distance is too large.`, con un
    esito nuovo, `CornerOutcome::DistanceTooLarge`, in `chamfer` e
    `chamferSegments`; prima era `None`, con `Lines are parallel.`. Esempio:
    linee (200,0)→(210,0) e (200,0)→(200,10), clic in (205,0) e (200,5),
    distanze 15 e 15. Lo stesso con *Angle*, quando il secondo punto
    costruito cade dalla parte tenuta della seconda linea ma oltre la sua fine
    (decisione sulla [#508](https://github.com/Matleyx78/tecnigrafo/pull/508)):
    prima linea (0,0)→(10,0), seconda (0,0)→(0,1), clic in (8,0) e (0,0.5),
    lunghezza 5 e angolo 45°, secondo punto in (0,5);
  - in `CHAMFER` il valore predefinito della seconda distanza è la prima
    distanza appena scritta.

  Cambiano le schede [P19](tappe/0.7/P19-raccordi-core.md) (solo
  l'enumeratore), [P20](tappe/0.7/P20-fillet.md) e
  [P21](tappe/0.7/P21-chamfer.md), la tabella dei messaggi di
  [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md), il catalogo delle
  [domande](verifica/domande.md) (esito `distance-too-large`) e il piano della
  [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al piano".
