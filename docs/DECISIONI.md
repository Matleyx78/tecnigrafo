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
    sua linea dà il messaggio di AutoCAD, `Chamfer distance too large.` (il
    testo della schermata di AutoCAD trovata nella revisione della #508), con un
    esito nuovo, `CornerOutcome::DistanceTooLarge`, in `chamfer` e
    `chamferSegments`; prima era `None`, con `Lines are parallel.`. Esempio:
    linee (200,0)→(210,0) e (200,0)→(200,10), clic in (205,0) e (200,5),
    distanze 15 e 15. Lo stesso con *Angle*, quando il secondo punto
    costruito cade dalla parte tenuta della seconda linea ma oltre la sua fine
    (decisione sulla [#508](https://github.com/Matleyx78/tecnigrafo/pull/508)):
    prima linea (0,0)→(10,0), seconda (0,0)→(0,1), clic in (8,0) e (0,0.5),
    lunghezza 5 e angolo 45°, secondo punto in (0,5). Con una distanza zero e
    l'altra troppo lunga prevale invece B8, `None`, in tutti e due gli ordini
    (decisione sulla #508): le stesse linee con distanze 0 e 15;
  - in `CHAMFER` il valore predefinito della seconda distanza è la prima
    distanza appena scritta.

  Cambiano le schede [P19](tappe/0.7/P19-raccordi-core.md) (solo
  l'enumeratore), [P20](tappe/0.7/P20-fillet.md) e
  [P21](tappe/0.7/P21-chamfer.md), la tabella dei messaggi di
  [dxf-di-prova.md](tappe/0.7/dxf-di-prova.md), il catalogo delle
  [domande](verifica/domande.md) (esito `distance-too-large`) e il piano della
  [tappa 0.7](tappe/0.7-modifiche.md) in "Modifiche al piano".
- **2026-11-06 — P21, precedenza della prima lunghezza in Angle.** Nella
  [#525](https://github.com/Matleyx78/tecnigrafo/pull/525#issuecomment-6683170915)
  il responsabile conferma che una lunghezza maggiore della parte tenuta della
  prima linea dà `DistanceTooLarge` prima di cercare il secondo punto. Vale
  anche quando la seconda retta è parallela alla retta dello smusso o la
  interseca nello spigolo o dalla parte opposta. La
  [scheda P21](tappe/0.7/P21-chamfer.md) riporta la regola e l'esempio di test.

- **2026-11-06 — P23, opzione `cLose` di `JOIN` negli script.**
  Nella [#375](https://github.com/Matleyx78/tecnigrafo/issues/375#issuecomment-6684760832)
  il responsabile conferma che *cLose* converte l'arco senza domanda di
  conferma e resta disponibile dopo la scelta di altri archi: si ripete
  `Select arcs to join to source or [cLose]:` fino a Invio o alla scelta
  dell'opzione. Il messaggio è `Arc converted to circle.` se nessun altro
  arco è assorbito. Nella [#528](https://github.com/Matleyx78/tecnigrafo/pull/528#issuecomment-6685063558)
  il responsabile decide `<n> arcs joined and converted to circle` se
  *cLose* assorbe altri archi, contando la sorgente e gli archi assorbiti.
  In entrambi i casi si aggiunge il suffisso delle scartate quando serve,
  omettendo l'eventuale punto prima della virgola. Cambiano la [scheda P23](tappe/0.7/P23-join.md), la tabella dei
  messaggi delle [prove DXF](tappe/0.7/dxf-di-prova.md) e il
  [piano della tappa 0.7](tappe/0.7-modifiche.md).
- **2026-11-06 — P24, messaggi della tabella e fine dei comandi negli script.**
  Nella [#530](https://github.com/Matleyx78/tecnigrafo/pull/530#issuecomment-6689130590)
  il responsabile conferma ciò che fa il codice in due punti in cui i
  documenti erano rimasti indietro. `OFFSET` con distanza nulla o negativa
  scrive `Value must be positive and nonzero.`, come il raggio di `CIRCLE`
  *Ttr*, e *Ttr* senza cerchio possibile scrive `Circle does not exist.`: la
  tabella dei messaggi delle [prove DXF](tappe/0.7/dxf-di-prova.md) è corretta.
  Negli script di AutoCAD `FILLET` finisce dopo *Radius* e dopo *Trim*,
  `CHAMFER` dopo *Distance*, *Angle*, *Trim* e *mEthod*, come già dicono le
  schede [P20](tappe/0.7/P20-fillet.md) e [P21](tappe/0.7/P21-chamfer.md): la
  decisione completa la tabella del punto 8
  dell'[ADR 0032](adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md), che
  nomina solo *Radius*, *Distance* e *Angle*; è corretta anche la tabella
  delle sequenze in [verifica/script.md](verifica/script.md). Il [manuale dei
  comandi](manuale/comandi.md) descrive tutti e due i punti.
- **2026-11-07 — La pausa tra le tappe e le decisioni della pausa dopo la 0.7.**
  Nella chat del 2026-11-07, dopo l'analisi delle issue aperte dei due
  repository, il responsabile decide:
  - **Immagini della CI.** I due progetti si fissano a `windows-2025-vs2026` e
    `macos-26`, con Linux `ubuntu-24.04`. È annullata la scelta della #526 del
    2026-11-06 (`windows-2025` e `macos-15`), che non considerava la decisione
    del 2026-11-05 di solid-space né la prova della sua #199 (sulle immagini
    precedenti il codice di solid-space non compila); in solid-space resta la
    decisione del 2026-11-05 sulle immagini più recenti. Lo applica il primo
    pacchetto della [tappa 0.7.5](tappe/0.7.5-correzione-di-rotta.md).
  - **La pausa tra le tappe.** La decisione del 2026-11-03 («le regole di
    metodo cambiano solo alla chiusura della tappa […] e si decidono nella pull
    request di chiusura») era scritta male. Si intende: chiuse le tappe, con
    tutto fermo, il responsabile decide le idee aperte, le proposte di metodo e
    le correzioni dei documenti; una pull request di soli documenti per
    repository le applica; solo dopo si apre la tappa successiva. La pull
    request di chiusura non decide più le proposte
    ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), nuovo B13 e B12;
    [ROADMAP.md](ROADMAP.md), "Chiudere una tappa";
    [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), sezione 8). Durante la
    tappa le regole non cambiano, salvo un vero blocco, come prima.
  - **Idee.** Entrano nella tappa 0.7.5, di cui c'è ora la
    [scheda](tappe/0.7.5-correzione-di-rotta.md): l'allineamento della CI
    ([#521](https://github.com/Matleyx78/tecnigrafo/issues/521)), l'analisi dei
    pacchetti medi ([#442](https://github.com/Matleyx78/tecnigrafo/issues/442))
    e l'**interfaccia** ([#483](https://github.com/Matleyx78/tecnigrafo/issues/483)):
    compilato alla fine della 0.7.5, Tecnigrafo deve presentarsi praticamente
    come le schermate di riferimento della #483 ed essere pronto per gli
    ambienti (disegno, parti, assiemi), con documenti in schede e ambiente
    attivo che guida ribbon, menu e comandi: passando da un disegno a un file
    di parti cambiano ribbon, icone e menu, perché i comandi sono altri. Se si
    chiede un ambiente che il documento attivo non può ospitare, il documento
    resta aperto nella sua scheda e se ne apre o crea uno del tipo giusto in una
    scheda nuova, come in Inventor (strada A, scelta fra due); la richiesta di
    salvare compare solo quando un documento modificato si chiude davvero.
    Entra nella 0.7.5 anche l'immissione dinamica vicino al cursore che si vede
    nel riferimento visivo; restano fuori i testi in italiano (0.9), i comandi
    che non esistono ancora e le schede *Modello*/*Layout* (dopo la 1.0). Icone e voci si
    rifanno per tutti i comandi già esistenti, compresi quelli di modifica della
    0.7; le icone dei comandi nuovi le disegna la tappa che li porta, con le
    regole fissate. Il riferimento visivo è il mockup del raccoglitore privato
    del responsabile, che l'ADR della decisione visiva descrive per intero. Il
    tempo che serve non è un vincolo: cade la durata indicativa di circa 10
    giorni del 2026-11-06. I lavori della 0.7.5 che toccano solid-space si
    fanno fuori tappa, come la #202 di solid-space.
    L'autohide dei pannelli
    ([#534](https://github.com/Matleyx78/tecnigrafo/issues/534)) si analizza e
    si decide in quel lavoro. `PEDIT`
    ([#535](https://github.com/Matleyx78/tecnigrafo/issues/535)) entra nella
    tappa 0.8, nella sua [scheda](tappe/0.8-quote-retini-blocchi.md), perché
    nella 0.7.5 non c'è posto. La
    [#522](https://github.com/Matleyx78/tecnigrafo/issues/522) aspetta il parere
    di Codex, poi segue la scheda della 0.7.5. La
    [#62](https://github.com/Matleyx78/tecnigrafo/issues/62) resta fuori, per la
    0.9. La [#61](https://github.com/Matleyx78/tecnigrafo/issues/61) è fatta
    nella 0.7 (P12, #354) e si chiude.
  - **Proposte di metodo**
    ([#486](https://github.com/Matleyx78/tecnigrafo/issues/486) e
    Matleyx78/solid-space#169): si applicano tutte. Le voci comuni hanno lo
    stesso testo nei due repository:
    - una dipendenza da una pull request si scrive come la issue che quella
      pull request chiude ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
      sezione 1; [tappe/0000-modello.md](tappe/0000-modello.md));
    - prefissi `PT`/`RT`/`CT` per Tecnigrafo e `PS`/`RS`/`CS` per solid-space,
      dalla tappa 0.8, senza rinominare l'esistente; servono prima gli script e
      i controlli, adeguati da un pacchetto della 0.7.5, che quindi usa ancora
      `P`/`R`/`C` (LAVORO-IN-PARALLELO.md, "Come leggere i riferimenti");
    - prima di fermarsi per un dubbio anche chi realizza cerca le regole
      esistenti, come chi rivede ([AGENTS.md](../AGENTS.md), "Quando
      fermarsi");
    - chi rivede usa un modello diverso dall'autore, e chi scrive i riferimenti
      un modello diverso da chi realizza; pull request e verbali dicono il
      modello (AGENTS.md, "Commit e pull request"; LAVORO-IN-PARALLELO.md, "Chi
      fa cosa" e sezione 6; [agenti/revisione.md](agenti/revisione.md));
    - i riferimenti delicati (curve parallele, raccordi, intersezioni) a un
      agente di fascia forte, di norma Codex; a Gemini 3.8 Flash solo i
      riferimenti ordinari e dei comandi (LAVORO-IN-PARALLELO.md, "Modello e
      impegno"). Completa la regola del 2026-10-31;
    - chi trova riferimenti che gli sembrano sbagliati li guarda tutti e si
      ferma una volta sola, con un solo commento
      ([agenti/pacchetto.md](agenti/pacchetto.md)).

    Il responsabile aggiunge una regola solo di Tecnigrafo: una tappa che
    aggiunge comandi li porta anche nell'interfaccia (icona, voci di menu,
    ribbon e barre, scorciatoia), con un pacchetto di `app` e un criterio di
    chiusura che la PR di chiusura controlla, perché nella 0.7 i comandi di
    modifica sono rimasti fuori da menu e ribbon (LAVORO-IN-PARALLELO.md,
    "Comandi nell'interfaccia" e sezione 8; tappe/0000-modello.md; scheda della
    0.8).

    Le proposte che sono codice o CI diventano lavori della 0.7.5: correggere
    `scripts/ci-attendi.sh`; il commit del sottomodulo automatico e sicuro
    (scelta del 2026-11-06, che vale anche per «gestire il commit del
    sottomodulo» di solid-space); gli estremi degli archi con `endpoints()`
    di solid-space, con il commento dello snap alle intersezioni. In
    solid-space la promessa «Symmetric» di `intersection.hpp` si tiene.
  - **Documenti da sistemare.** Le issue
    [#500](https://github.com/Matleyx78/tecnigrafo/issues/500) e
    Matleyx78/solid-space#192 sono vuote: si chiudono senza correzioni.
  - **Una chat su due repository.** Per questa pausa il responsabile ha
    autorizzato la chat di Tecnigrafo a scrivere anche in solid-space
    (commenti, chiusura di issue, una pull request), in deroga ad
    [AGENTS.md](../AGENTS.md), "Dove puoi scrivere", che non cambia.

  La [#526](https://github.com/Matleyx78/tecnigrafo/issues/526) si chiude: la
  sostituisce la scheda della tappa 0.7.5.
- **2026-11-07 — Decisioni del piano della tappa 0.7.5.** Nella chat del piano
  ([#537](https://github.com/Matleyx78/tecnigrafo/pull/537)) il responsabile
  decide:
  - **F5 fuori tappa**: gli strumenti accettano le tappe con tre numeri prima
    che la 0.7.5 si apra ([fuori-tappa/F5](tappe/fuori-tappa/F5-tappe-con-tre-numeri.md)).
  - **Attributi dei blocchi nella 0.8**, al livello (b) della
    [#522](https://github.com/Matleyx78/tecnigrafo/issues/522): `ATTDEF` e
    `ATTRIB` letti, rappresentati nel modello e riscritti, senza comandi per
    crearli o modificarli; le operazioni sugli inserimenti li tengono coerenti
    (spostare, copiare, cancellare ed esplodere un `INSERT` porta con sé i suoi
    `ATTRIB`, con handle nuovi sulle copie e un solo passo di annulla); le
    varianti non gestite si conservano e le operazioni che non le terrebbero
    coerenti si rifiutano. Chi prepara il piano della 0.8 legge anche la
    [#538](https://github.com/Matleyx78/tecnigrafo/issues/538), la prima
    analisi dei blocchi dell'elettrico. Cambia la
    [scheda della 0.8](tappe/0.8-quote-retini-blocchi.md).
  - **Richieste dei retini**: non si aprono ora, ma con il piano della 0.8,
    dopo la chiusura della 0.7.5 e la pausa, perché le valuti e le realizzi la
    tappa di solid-space che corrisponde alla 0.8; i loro testi sono in un commento della
    [#576](https://github.com/Matleyx78/tecnigrafo/issues/576).
  - **Revisione delle interfacce delle schede P24–P31** (0.7.5-P23), anche se
    non toccano `core`.
  - **Icone contate a coppie**: nei pacchetti delle icone un'icona a 16 e 32 px
    conta come un file per la misura del pacchetto.

  - **Modello della quota nella 0.8**: la quota della 0.8 è solo
    un'annotazione, ma il suo modello tiene separate tre cose, perché possa
    diventare associativa e, negli schizzi, un vincolo che comanda senza rifarla:
    come si mostra (stile, testo, frecce, posizione), che cosa misura (oggi
    punti di definizione, domani riferimenti a entità) e chi decide il valore
    (oggi la misura, domani un vincolo). I dati DXF standard restano nel DXF,
    quelli propri in `extensions.json`; rigenerare la geometria della quota non
    cancella i dati sconosciuti (parere di Codex nella #522, punto 5). Lo scrive
    l'ADR delle entità composte della 0.8.
  - **`NEW` e `OPEN` con i documenti in schede** (strada B, come AutoCAD): aprono
    sempre un documento nuovo, che diventa attivo, e non chiedono di salvare
    quello di prima, che resta aperto, anche se è un disegno appena creato e mai
    toccato (provato dal responsabile in AutoCAD LT il 2026-11-07: dopo *Nuovo*,
    *Apri* apre il file in un'altra scheda). Vale nel programma e negli script dei
    due livelli; il rapporto ha il campo `documents:` con più di un documento
    ([verifica/rapporto.md](verifica/rapporto.md)). I casi di verifica che
    cambiano, cioè tutti quelli che usano `NEW` o `OPEN`, si aggiornano con un
    pacchetto di riferimenti (0.7.5-R28b), prima del lavoro
    (0.7.5-P28b).
  - **Avvio sulla pagina *Start***, come AutoCAD: il programma parte senza
    documenti, e l'utente crea quello che gli serve fra i tipi registrati (oggi
    il disegno). Gli script partono come oggi da un disegno vuoto (0.7.5-P29).
  - **Pannelli che si nascondono da soli**
    ([#534](https://github.com/Matleyx78/tecnigrafo/issues/534)): sì, costruiti
    con Qt, senza librerie esterne (ADR 0009, punto 3): una puntina nella barra
    del titolo, la linguetta sul bordo, il riquadro sovrapposto dentro la
    finestra (0.7.5-P18). Il pacchetto di analisi 0.7.5-P17 si toglie.
- **2026-11-07 — Il piano della 0.8 dopo la chiusura della 0.7.5.** Il
  responsabile toglie dalla tappa 0.7.5 il pacchetto del piano della 0.8
  (0.7.5-P34, #576): il piano di una tappa si prepara dopo la chiusura della
  precedente e dopo la pausa (GUIDA-RESPONSABILE.md, B13 e B1), e lo scrive lui
  con Claude in chat. La 0.7.5 ne prepara le basi
  ([tappe/0.7.5-correzione-di-rotta.md](tappe/0.7.5-correzione-di-rotta.md),
  «Modifiche al piano»).
- **2026-11-07 — Qt 6.12.** Con le immagini nuove della CI (0.7.5-P1,
  [#579](https://github.com/Matleyx78/tecnigrafo/pull/579)) Qt 6.8.3 non
  collega su `macos-26`: l'SDK di macOS 26 non ha più il framework AGL, che la
  configurazione di Qt 6.8.3 aggiunge (QTBUG-137687). Il responsabile sceglie di
  passare alla versione stabile più recente, come per le immagini: **Qt 6.12.0
  su tutti e tre i sistemi, minima 6.12**. Su Windows aqtinstall 3.3.0 non
  installa Qt 6.11 e successive: finché non esce una versione pubblicata che lo
  fa, la CI usa aqtinstall da un commit fissato del suo ramo principale, con
  `py7zr` fissato. Scartate Qt 6.10.3 (serie chiusa, senza correzioni),
  l'aggiramento di AGL in CMake, un Xcode più vecchio e `macos-15`. Qt 6.12 si
  installa a parte anche su Debian 13. Lo scrive
  l'[ADR 0043](adr/0043-qt-6-12.md), che sostituisce in parte l'ADR 0028; lo
  applica 0.7.5-P1, la cui [scheda](tappe/0.7.5/P1-immagini-ci.md) è aggiornata.
- **2026-11-07 — Buchi delle schede P24–P31: le decisioni.** Il responsabile
  decide i buchi B1–B9 del verbale della revisione delle interfacce (0.7.5-P23,
  [#563](https://github.com/Matleyx78/tecnigrafo/issues/563)), una decisione per
  commento, letto anche con la revisione del verbale. Tutti restano in
  Tecnigrafo.
  - [B1](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711336484), P27: `U`, `UNDO` e `REDO` valgono in tutti gli ambienti, e
    `Invio` li ripete con la stessa regola; per questi nomi `environmentsOf`
    dà la lista vuota, che vuol dire «tutti gli ambienti».
  - [B2](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711336608), P28: il testo scritto e non ancora consegnato al runner (riga di
    comando e campi dell'immissione dinamica, con il campo attivo) è della
    sessione: si conserva lasciandola, torna tornandoci, anche da *Start*, e
    non arriva mai a un'altra sessione.
  - [B3](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711336738), P28b: un file è aperto al più in un documento; `SAVEAS`, `SAVE` e
    `QSAVE` rifiutano un nome aperto in un altro documento, prima della
    domanda di sostituzione, con `Cannot save <name>: it is open in another
    document.`
  - [B4](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711336858), P29: chiusa la scheda attiva diventa attiva quella a destra, se
    no quella a sinistra, se no *Start*; chiudere una scheda non attiva non
    cambia l'attiva.
  - [B5](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711336976), P29: chiudere la finestra chiude i documenti uno per uno,
    nell'ordine delle schede, come la crocetta di ciascuno; un *Cancel* o un
    salvataggio non riuscito ferma la chiusura, e i documenti già chiusi
    restano chiusi (ADR 0041, punto 3, come letto nella revisione del verbale).
  - [B6](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711337104), P30: si corregge la scheda secondo l'ADR 0041, punto 7, che
    resta com'è: il tipo di documento crea, apre e salva, e la cornice passa
    dal tipo. Per la misura del pacchetto il lavoro va in un pacchetto nuovo,
    [P30b](tappe/0.7.5/P30b-aprire-e-salvare-dal-tipo.md).
  - [B7](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711337203), P29, P30: un'azione ha gli ambienti in cui compare, con la lista
    vuota per «tutti» (le azioni globali), e un flag separato «richiede un
    documento»; niente «ambiente Start».
  - [B8](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711337347), P30, P31: l'ambiente scelto resta della sessione (ADR 0041,
    punto 1); che cosa succede con un comando in corso non si vede in questa
    tappa e lo sceglie chi implementa P31.
  - [B9](https://github.com/Matleyx78/tecnigrafo/issues/563#issuecomment-6711337477), P30, P31: l'indicatore dell'ambiente resta anche con *Start*,
    senza ambiente spuntato, e il suo menu funziona.

  Le schede si correggono anche con R1 (P24, P28b: il file e i recenti dopo
  `OPEN`) e con le note N1–N4 del verbale e della revisione, che non chiedono
  decisioni. Cambiano le schede [P24](tappe/0.7.5/P24-ciclo-di-vita-del-file.md),
  [P25](tappe/0.7.5/P25-tabelle.md),
  [P26](tappe/0.7.5/P26-protocollo-dei-comandi.md),
  [P27](tappe/0.7.5/P27-ambienti-nei-comandi.md),
  [P28](tappe/0.7.5/P28-sessione-per-documento.md),
  [P28b](tappe/0.7.5/P28b-new-e-open.md),
  [P29](tappe/0.7.5/P29-schede-e-pagina-iniziale.md),
  [P30](tappe/0.7.5/P30-ambiente-attivo-e-tipi.md) e
  [P31](tappe/0.7.5/P31-ambiente-non-ospitato.md), nasce P30b, e il piano della
  [tappa 0.7.5](tappe/0.7.5-correzione-di-rotta.md) lo scrive in "Modifiche al
  piano". Nessun ADR cambia. P23 (#563) si chiude.
- **2026-11-08 — Test e CI più rapidi: P35 e P36.** Il responsabile aggiunge alla
  [tappa 0.7.5](tappe/0.7.5-correzione-di-rotta.md) due pacchetti:
  [P35](tappe/0.7.5/P35-icone-calcolate-una-volta.md), le icone colorate
  calcolate una volta per tema, con il limite di `app_widgets` riportato a un
  valore che ferma solo un blocco vero; e
  [P36](tappe/0.7.5/P36-tidy-e-ccache.md), il confronto giusto per scegliere
  il tidy nelle pull request e ccache nei job Linux. Decide:
  - **ccache ammesso nella CI**, nei job Linux: è uno strumento della CI, non
    una dipendenza del programma;
  - **un cambio di `.github/workflows/ci.yml` fa ancora il tidy completo**
    nella pull request, come oggi;
  - dividere `app_widgets` ed eseguire i test in parallelo si decide dopo, con
    i tempi misurati da P35 e P36.
- **2026-11-08 — Su Windows la CI usa le Qt ottimizzate: P37.** Il responsabile
  aggiunge alla [tappa 0.7.5](tappe/0.7.5-correzione-di-rotta.md)
  [P37](tappe/0.7.5/P37-windows-qt-ottimizzate.md): nel job Windows la build
  Debug resta Debug per il nostro codice, asserzioni comprese, ma si collega al
  runtime normale di MSVC e alle Qt ottimizzate, come già succede su Linux
  (diagnostica della [#599](https://github.com/Matleyx78/tecnigrafo/pull/599):
  `app_widgets` da 93,6 s a 8,17 s). Accetta di perdere su Windows i controlli
  del runtime di debug di MSVC e le asserzioni interne delle Qt: gli errori di
  memoria li cerca il job ASan su Linux. Dividere `app_widgets` non serve più;
  eseguire i test in parallelo si decide dopo P36, con i tempi del job ASan.
- **2026-11-08 — Test in parallelo nella CI: P38.** Con i tempi misurati dopo
  P36 e P37 il responsabile aggiunge alla
  [tappa 0.7.5](tappe/0.7.5-correzione-di-rotta.md)
  [P38](tappe/0.7.5/P38-test-in-parallelo.md). `ctest` si esegue in parallelo,
  con tanti processi quanti i processori della macchina, nella CI e in
  `scripts/controlla.sh`. I test non condividono più cartelle di lavoro fisse.
  Restano un processo per caso (`gtest_discover_tests`) e i `TIMEOUT` di oggi.
- **2026-11-08 — Confronto tra sistemi: eccezioni fino alle funzioni
  deterministiche.** Il primo giro del confronto tra sistemi
  ([0.7.5-P5](tappe/0.7.5/P5-confronto-e-controllo-incrociato.md),
  [ADR 0038](adr/0038-ci-dei-due-progetti.md), punto 5) trova 17 casi con le
  ultime cifre di alcuni numeri diverse tra Linux, Windows e macOS
  ([#546](https://github.com/Matleyx78/tecnigrafo/issues/546#issuecomment-6725181081)).
  Il responsabile sceglie di renderli identici, non di confrontarli con una
  tolleranza: solid-space fornirà funzioni trigonometriche deterministiche,
  uguali bit per bit su tutti i sistemi, e `core` le userà. Il lavoro si fa
  all'inizio della tappa 0.8 ([#620](https://github.com/Matleyx78/tecnigrafo/issues/620)).
  Fino ad allora i 17 casi sono eccezioni di
  [verifica/sistemi.md](verifica/sistemi.md), ognuna con il rimando alla #620;
  il confronto dà errore per un'eccezione il cui caso è già uguale su tutti i
  sistemi, così il pacchetto della 0.8 deve togliere le righe.
- **2026-11-08 — Analisi notturna con SonarQube Cloud
  ([#625](https://github.com/Matleyx78/tecnigrafo/issues/625)).** Il
  responsabile aggiunge a Tecnigrafo l'analisi di SonarQube Cloud già attiva in
  solid-space (sua decisione del 2026-11-03), fuori dai pacchetti della tappa.
  Si analizza solo il ramo `main`, ogni notte verso le 02:30 italiane (cron
  `47 1 * * *`, in UTC, mezz'ora dopo solid-space) e a mano con
  `workflow_dispatch`, nel workflow separato `.github/workflows/sonar.yml`, che
  non tocca `ci.yml` e non blocca le pull request. Nessuna analisi sulle pull
  request né sui push, nessuna copertura dei test per ora. L'azione
  `SonarSource/sonarqube-scan-action` è fissata a uno SHA preciso; il C++ si
  analizza dal `compile_commands.json` del preset `release`
  (`build/release/compile_commands.json`), senza Build Wrapper, con le Qt
  installate come nel job `linux-release`, così entra anche `app`. La
  configurazione sta in `sonar-project.properties`; solid-space
  (`third_party/**`) è escluso, perché ha la sua analisi. L'analisi automatica
  di Sonar è spenta e il token sta nel secret `SONAR_TOKEN`.
- **2026-11-10 — Pacchetti medi: le condizioni dell'analisi della #442 e le due
  misure.** Letta l'analisi di
  [0.7.5-P8](tappe/0.7.5/analisi-pacchetti-medi.md) (#549), il responsabile
  approva la raccomandazione «sì, con condizioni»: le condizioni 1–7 della
  sezione 5 e il modello di lavoro della sezione 3.9, «la fascia forte decide e
  controlla, la media realizza». In fascia forte restano i riferimenti con casi
  delicati, la revisione delle interfacce, i pacchetti che progettano
  un'interfaccia nuova e la revisione mirata delle pull request in fascia media
  che toccano parti delicate; va in fascia media il codice che traduce decisioni
  già scritte (nella scheda, nel contratto di solid-space o in un ADR) e coperte
  da riferimenti di fascia forte. Delle due varianti proposte per «Modello e
  impegno» vale la B.
  - **Quando valgono.** Dalla tappa 0.8. Le regole di metodo non cambiano a
    tappa aperta (2026-11-07, «La pausa tra le tappe»): il testo della sezione 6
    dell'analisi lo applica la pull request di documenti della pausa dopo la
    0.7.5, prima del piano della 0.8, che nasce già con le regole nuove.
  - **Riferimenti divisi per natura: ogni `RN` al suo `PN`.** La forma
    proposta dall'analisi (un solo pacchetto di riferimenti per tutte le parti
    di un pacchetto diviso, con un file di attesa per ogni pacchetto di lavoro)
    non si adotta: l'[ADR 0011](adr/0011-modalita-di-controllo.md), punto 7, e i
    nomi dei file che `tests/verifica/pending_list.cmake` accetta restano come
    sono. Ogni parte di un pacchetto diviso ha i suoi riferimenti; dove
    servono casi delicati e ordinari, si dividono in blocchi con la lettera
    (per esempio `RT14a` di fascia forte con i casi delicati e `RT14b` leggero
    con gli ordinari), come le regole di oggi già permettono. La pausa applica
    il punto «Riferimenti della stessa misura» in questa forma, non in quella
    della sezione 6 dell'analisi.
  - **Impegno registrato.** Da subito ogni pull request dichiara, nella sezione
    sull'uso dell'IA, strumento, modello e impegno: senza, la misura non si può
    fare (condizione 7). Il responsabile autorizza la riga nel modello di pull
    request in `.github/` ([AGENTS.md](../AGENTS.md), «Commit e pull request»;
    [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), A2). È un dato in più, non
    un cambio di metodo.
  - **Due misure.** Alla fine della 0.7.5 il pacchetto nuovo
    [0.7.5-P39](tappe/0.7.5/P39-misura-pacchetti.md) rende ripetibili gli
    script della sezione 7 dell'analisi e misura la tappa, quasi tutta di
    pacchetti medi. Alla fine della 0.8 l'ultimo pacchetto del suo piano rifà
    la misura con lo stesso script e propone la decisione definitiva: è un
    criterio di chiusura della [scheda della 0.8](tappe/0.8-quote-retini-blocchi.md).
    La qualità del codice impegnativo realizzato in fascia media è un'ipotesi
    finché la seconda misura non la conferma.
- **2026-11-10 — Icone a tratto da 1 px e correzioni della prova manuale
  (nuovo 0.7.5-P40).** Nella prova manuale della chiusura della 0.7.5
  ([#647](https://github.com/Matleyx78/tecnigrafo/pull/647)), su Debian sotto
  Wayland a scala 100%, i comportamenti passano e l'aspetto no: barra di
  accesso rapido a destra con la cornice del sistema, schede del ribbon più
  basse di *File*, riquadri dei pannelli a scomparsa che non si allargano,
  icone spesse e sfocate. Il responsabile decide:
  - **Tratto da 1 px.** Nelle due griglie, 16 e 32 px, il tratto è 1 px, con
    le linee orizzontali e verticali sulla mezzeria di un pixel
    (coordinata `n + 0,5`), così che coprano una riga o una colonna intera
    di pixel. Sostituisce il tratto dell'[ADR 0040](adr/0040-icone-a-ruoli.md),
    punto 3 (1,75 a 32 e 1,25 a 16), che resta scritto così com'è: fanno fede
    questa voce e [ICONE.md](ICONE.md). Il resto dell'ADR 0040 non cambia.
    Punti, tratteggi e margini seguono, come dice la scheda di P40.
  - **Un pacchetto solo, subito.** Le quattro correzioni e le icone
    ridisegnate sono un pacchetto unico,
    [0.7.5-P40](tappe/0.7.5/P40-correzioni-della-prova-manuale.md) (Claude,
    forte), anche se va oltre la quindicina di file
    ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), «Preparare il piano»):
    le icone contano come un gruppo. La chiusura della 0.7.5 riprende dopo
    P40, con i punti 2, 4 e 7 della prova manuale ripetuti.
- **2026-11-11 — La pausa dopo la 0.7.5.** Nella chat del 2026-11-11, dopo
  l'analisi delle issue aperte dei due repository, il responsabile decide:
  - **Proposte di metodo**
    ([#600](https://github.com/Matleyx78/tecnigrafo/issues/600)), che si chiude:
    - *Galleria approvata*: non si adotta. L'interfaccia è migliorata molto e
      resta com'è; la grafica non deve frenare lo sviluppo, e la forma
      definitiva la darà una tappa dedicata prima degli installatori, con un
      metodo da trovare ([#655](https://github.com/Matleyx78/tecnigrafo/issues/655)).
    - *Regole della #442*: si applicano come deciso il 2026-11-10, anche in
      solid-space, con lo stesso testo per le voci comuni
      ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), «Pacchetti leggeri» e
      «Modello e impegno»; [tappe/0000-modello.md](tappe/0000-modello.md),
      sezione 6; [tappe/0000-pacchetto.md](tappe/0000-pacchetto.md);
      [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B1).
    - *Agenti*: dal 19 novembre Gemini (3.8 Flash e 4 Argon) e Copilot non ci
      sono più, e gli agenti sono Claude e Codex. Ogni pacchetto fa un giro a
      tre: un agente scrive i riferimenti, l'altro realizza il pacchetto, e
      chi ha scritto i riferimenti rivede la pull request (Codex, Claude, Codex
      oppure Claude, Codex, Claude), perché riferimenti fatti bene si sono
      dimostrati ciò che fa avanzare il lavoro. Riferimenti e revisioni si
      possono fare a lotti. La pull request di chiusura della tappa passa a
      Codex; i pacchetti facili seguono lo stesso giro. Le etichette
      `agente:gemini`, `agente:gemini-argon` e `agente:copilot` restano, ma non
      si assegnano; le istruzioni di Antigravity in
      [AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md) restano, segnate come non
      usate. I file di Copilot (`.github/copilot-instructions.md` e il workflow
      `copilot-setup-steps.yml`) si tolgono; l'[ADR 0043](adr/0043-qt-6-12.md),
      accettato, ne parla ancora, e fa fede questa voce.
    - *Codex che chiama `@claude` a fine pacchetto*: rimandata a quando anche
      Codex potrà prendere comandi dai commenti
      ([#656](https://github.com/Matleyx78/tecnigrafo/issues/656)).
  - **Idee per la 0.8** (nella [scheda](tappe/0.8-quote-retini-blocchi.md)):
    - *Tracciamento degli snap a oggetto*
      ([#653](https://github.com/Matleyx78/tecnigrafo/issues/653)): dentro
      acquisizione, guide orizzontali e verticali, incrocio di due guide e
      distanza scritta; fuori le guide polari dai punti acquisiti
      (`POLARMODE`), `TRACKPATH`, *Prolungamento* e *Parallelo*. Spento in un
      disegno nuovo; negli script il punto si acquisisce con una direttiva
      nuova, così `!move` non cambia significato.
    - *Funzioni trigonometriche deterministiche*
      ([#620](https://github.com/Matleyx78/tecnigrafo/issues/620)): sono
      quelle di `solid_space::robust`, che ha già `sinCos` e `atan2`, scritte
      senza librerie esterne. Un pacchetto all'inizio della 0.8 le usa al posto
      di `<cmath>` in `core`, `cmd` e `render`, e svuota le eccezioni; `tan`,
      `acos` e `hypot` si chiedono a `robust` con il piano, allo stesso modo.
    - *Blocchi, pensando all'elettrico*
      ([#538](https://github.com/Matleyx78/tecnigrafo/issues/538)): l'entità
      `SOLID` (diversa dal motivo SOLID dei retini) si legge, si disegna e si
      riscrive, insieme ai retini; `INSERT` inserisce un file DXF come blocco,
      come AutoCAD, perché le librerie di simboli sono un file per simbolo
      (anche in AutoCAD Electrical); un blocco con `ATTDEF` crea i suoi
      `ATTRIB` con i valori predefiniti, senza domande, come AutoCAD con
      `ATTREQ` = 0. La #538 resta aperta per l'elettrico dopo la 1.0.
    - *Errori e crash*
      ([#62](https://github.com/Matleyx78/tecnigrafo/issues/62)): nella 0.8 la
      traccia dello stack su Windows e l'avviso al riavvio dopo un crash; il
      resto nella 0.9.
  - **La 0.8 non si divide.** È densa, ma il piano cerca di far avanzare in
    parallelo il più possibile i flussi indipendenti; ed è la prima tappa con
    le regole nuove dei pacchetti, che una tappa densa mette meglio alla prova.
  - **Idee per la 0.9** (nella [scheda](tappe/0.9-stampa-e-distribuzione.md)):
    l'icona del programma
    ([#654](https://github.com/Matleyx78/tecnigrafo/issues/654)), dal disegno
    SVG del raccoglitore grafico, senza versione, con licenza CC0 e versioni
    semplificate per le misure piccole (l'immagine JPG non si usa); la parte
    generale della segnalazione di errori e crash (#62, ADR 0010).
  - **Altre idee.** La
    [#522](https://github.com/Matleyx78/tecnigrafo/issues/522) è esaurita
    (ADR 0041, decisioni per la 0.8, separazioni P24–P27, traccia 2.x rivista
    da P32) e si chiude; ciò che rimanda è nell'ADR 0041, punto 12, e nella
    [#634](https://github.com/Matleyx78/tecnigrafo/issues/634), che riguarda il
    dopo-1.0 e resta com'è.
  - **Una chat su due repository.** Per questa pausa il responsabile autorizza
    la chat di Tecnigrafo a scrivere anche in solid-space (una pull request,
    commenti), in deroga ad [AGENTS.md](../AGENTS.md), "Dove puoi scrivere",
    che non cambia.
- **2026-11-11 — L'aspetto dell'interfaccia si corregge fuori tappa, in locale
  (F6).** Nella chat del 2026-11-11, dopo l'analisi di fattibilità, il
  responsabile decide:
  - **È una correzione.** Portare l'aspetto dell'interfaccia al riferimento
    dell'[ADR 0039](adr/0039-decisione-visiva-dell-interfaccia.md) è una
    correzione della 0.7.5 ([ROADMAP.md](ROADMAP.md), "Cosa si può fare fuori
    dalla tappa aperta"), e si fa con il pacchetto fuori tappa
    [F6](tappe/fuori-tappa/F6-aspetto-dell-interfaccia.md)
    ([#658](https://github.com/Matleyx78/tecnigrafo/issues/658)), **in
    parallelo** alla tappa 0.8: la 0.8 si apre e avanza senza aspettarlo. È il
    metodo che la voce «La pausa dopo la 0.7.5» lasciava da trovare
    ([#655](https://github.com/Matleyx78/tecnigrafo/issues/655)); se dopo F6
    serva ancora una tappa dedicata, e la galleria approvata che la CI
    confronta, lo decide il responsabile alla chiusura di F6, e fino ad allora
    la #655 resta aperta.
  - **In locale, con il responsabile.** Si lavora sul suo PC Debian, in una
    cartella dedicata (`~/progetti/tecnigrafo-grafica`), con Claude Code:
    l'agente compila, lancia la galleria e guarda le immagini, il responsabile
    guarda la finestra, e si ripete finché l'area è a posto. Durante il lavoro
    non ci sono pull request né CI.
  - **Un ramo solo, che tocca solo la grafica.** Il ramo `f6-grafica` tocca
    `app/`, le icone e i loro test, mai `core`, `cmd`, `io` e gli altri
    moduli; il comportamento non cambia. SARibbon resta copiato senza
    modifiche. Dopo ogni pull request unita in `main` il ramo ne assorbe le
    novità con un merge commit.
  - **Un'unione sola, alla fine.** Dal ramo si unisce una volta, a lavoro
    approvato, con una sola pull request, anche se è grande da rivedere per
    Codex: niente unioni a pezzi, per non rischiare rotture nel resto di
    Tecnigrafo.
  - **Deroghe.** Per il solo F6, la scheda deroga ad [AGENTS.md](../AGENTS.md)
    ("Quando fermarsi", l'etichetta `AA-Presa in carico` tenuta per tutto il
    pacchetto) e alla misura dei pacchetti
    ([LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), «Preparare il piano»),
    che non cambiano.
- **2026-11-11 — Piano della 0.8: le otto decisioni.** Nella chat del piano
  ([#661](https://github.com/Matleyx78/tecnigrafo/pull/661)) il responsabile
  decide, punto per punto della sezione 5 del
  [piano](tappe/0.8-quote-retini-blocchi.md):
  1. **`PEDIT` con *Multiple*.** In AutoCAD distanza di tolleranza e tipo di
     unione esistono solo dentro *Multiple*: senza, la decisione del
     2026-11-07 non avrebbe la sequenza di AutoCAD
     ([ADR 0050](adr/0050-pedit.md), punto 1).
  2. **Incrocio fra una guida del tracciamento e la guida polare** dal punto
     precedente, con l'intersezione di solid-space già nel contratto
     ([ADR 0049](adr/0049-tracciamento-degli-snap-a-oggetto.md), punto 4).
     Le guide polari dai punti acquisiti (`POLARMODE`) restano fuori.
  3. **`!hover x,y`** è la direttiva che acquisisce: descrive il gesto, come
     `!move` e `!click` (ADR 0049, punto 7).
  4. **Canale MCP spento a ogni avvio**, senza «accendi all'avvio» per ora; il
     **ponte `tecnigrafo --mcp` resta vivo** e si collega al programma a ogni
     richiesta, così l'assistente funziona appena il canale si accende, senza
     riavviarlo ([ADR 0051](adr/0051-canale-mcp-dell-assistente.md), punti 1
     e 3).
  5. **`MTEXT` e quote solo dalla riga di comando** nella 0.8. L'editor nella
     finestra, l'opzione *Mtext* delle quote e `TEXTEDIT` sono l'idea
     [#662](https://github.com/Matleyx78/tecnigrafo/issues/662), da pianificare
     dopo F6.
  6. **Testo e frecce delle quote strette come AutoCAD con `ISO-25`**
     (`DIMATFIT` 3, `DIMTMOVE` 0): le soglie e le posizioni si ricavano dai
     disegni di AutoCAD LT del responsabile; con altri valori di quelle
     variabili la quota non è rigenerabile
     ([ADR 0047](adr/0047-quote-e-stili-di-quota.md), punti 2 e 4). Sostituisce
     la proposta «testo sempre in mezzo».
  7. **L'ellisse entra nella 0.8**, «anche se è la strada più in salita»:
     entità `ELLIPSE`, comando `ELLIPSE`, inserimenti con scala diversa in X e
     in Y che generano ed esplodono in ellissi come AutoCAD, e una quarta
     richiesta a solid-space per selezione e intersezioni
     ([ADR 0052](adr/0052-ellisse.md)). `TRIM`, `EXTEND`, `OFFSET` e `FILLET`
     sulle ellissi restano fuori: idea
     [#663](https://github.com/Matleyx78/tecnigrafo/issues/663).
  8. **Otto simboli della libreria dell'elettrico e lo schema d'esempio come
     campioni di prova**, copiati in `Matleyx78/dxf-test-file`,
     `campioni-elettrico/` (CC0), dichiarati come campioni e non come la
     libreria, che resta in lavorazione in `tec-ss-grafica` fino a lavoro
     finito.
- **2026-11-11 — ADR della 0.8 accettati e richieste approvate.** Il
  responsabile accetta gli ADR [0044](adr/0044-entita-composte-e-geometria-generata.md)–[0052](adr/0052-ellisse.md)
  e approva i testi delle quattro richieste a solid-space del piano della 0.8
  (contorni e isole; parti di una curva in una regione; tangente, arcocoseno e
  `hypot`; l'ellisse in `geom2d`), scritti nei commenti di
  [#661](https://github.com/Matleyx78/tecnigrafo/pull/661), e chiede a Claude
  di aprirle: sono [solid-space#214](https://github.com/Matleyx78/solid-space/issues/214),
  [#215](https://github.com/Matleyx78/solid-space/issues/215),
  [#216](https://github.com/Matleyx78/solid-space/issues/216) e
  [#217](https://github.com/Matleyx78/solid-space/issues/217), tappa `ss-0.3`.
- **2026-11-12 — Schede della 0.8 dopo il contratto: tre decisioni.** Nella
  chat delle schede scritte dopo la revisione del contratto di solid-space
  (`[0.3-CS]`, Matleyx78/solid-space#222), il responsabile decide
  ([piano della 0.8](tappe/0.8-quote-retini-blocchi.md), «Modifiche al piano»):
  1. **Snap *Vicino* fuori dalla 0.8.** Tecnigrafo non lo ha per nessuna
     entità: l'ellisse non lo porta da sola (l'[ADR 0052](adr/0052-ellisse.md),
     punto 4, lo nominava). Il punto più vicino di solid-space serve nella 0.8
     alla scelta con un clic e alla domanda `closest`
     ([0.8-PT50](tappe/0.8/PT50-ellisse-selezione-snap.md)); lo snap, per tutte
     le entità insieme, è l'idea
     [#665](https://github.com/Matleyx78/tecnigrafo/issues/665).
  2. **Curve candidate del contorno rilevato:** le entità linea, arco, cerchio
     e polilinea del disegno, sui layer accesi e non congelati come la
     selezione. Non contano le forme generate di inserimenti e quote né le
     ellissi: un contorno fatto da un blocco non si rileva, una differenza da
     AutoCAD ([0.8-PT43](tappe/0.8/PT43-contorni-dei-retini.md); ADR 0048,
     punto 5).
  3. **Le righe del motivo lungo un lato del contorno si disegnano:** i tratti
     di bordo di `clipToRegion` fanno parte del retino come quelli interni
     ([0.8-PT42](tappe/0.8/PT42-righe-dei-motivi.md); ADR 0048, punto 3).
- **2026-11-12 — Direzioni cardinali esatte delle righe dei motivi.** Nella
  rilettura della correzione di PT40
  ([#751](https://github.com/Matleyx78/tecnigrafo/pull/751)) Codex trova un
  buco visibile: la direzione di una famiglia viene da `sinCos` dell'angolo in
  radianti, e a 90° il coseno del `double` di π/2 è circa `6e-17`, non 0. Una
  riga verticale di `NET` sul lato verticale di un contorno lo tocca solo
  nello spigolo e il tratto di bordo sparisce, o cade appena dentro, contro la
  decisione 3 del 2026-11-12 qui sopra. Il responsabile sceglie l'uscita **A**:
  la direzione di una famiglia (e di un retino) è quella di `sinCos`, ma una
  componente sotto `2^-48` in modulo diventa esattamente 0 e l'altra
  esattamente ±1, col suo segno. Vale anche dopo `ROTATE` e `MIRROR` e per lo
  scostamento calcolato dai `.pat`: la funzione è `cardinalSinCos` di
  [0.8-PT1](tappe/0.8/PT1-trigonometria-1.md) (voce successiva), usata da
  [0.8-PT30](tappe/0.8/PT30-retini-core.md) e
  [0.8-PT42](tappe/0.8/PT42-righe-dei-motivi.md). Scartata l'uscita B (angoli
  presi alla lettera, con il tratto di bordo verticale che a volte c'è e a
  volte no).
- **2026-11-12 — Quarti di giro esatti.** Nel secondo giro di rilettura di
  [#751](https://github.com/Matleyx78/tecnigrafo/pull/751) Codex trova che la
  voce precedente non basta: con `ROTATE` di 90° del retino e del suo
  contorno, `Similarity::matrix` usa ancora il coseno del `double` di π/2, il
  lato che era sull'asse y si inclina di `6e-16` mentre la riga resta esatta,
  e il tratto di bordo sparisce. Il responsabile sceglie l'uscita **A1**: la
  funzione con la soglia diventa `cardinalSinCos` in
  `tg/core/math.hpp` ([0.8-PT1](tappe/0.8/PT1-trigonometria-1.md)), e la usa
  anche `Similarity::matrix`, per tutte le entità: `ROTATE` e `MIRROR` di un
  multiplo di 90° danno coordinate esatte, e riga e lato restano
  coincidenti anche quando il contorno è fatto di entità separate. Scartate
  A2 (quarti esatti solo nella trasformazione del retino: due modi di
  ruotare) e B (direzione presa dalla matrice effettiva: il retino
  cambierebbe dopo salvataggio e rilettura del DXF, dove la direzione si
  ricalcola dall'angolo). Nel terzo giro Codex trova che la soglia proposta
  da Claude, `2^-40`, era troppo larga: raddrizzava anche angoli soltanto
  vicini a un quarto di giro, e a 16 km dall'origine gli spostamenti
  superavano la tolleranza (matrice e angolo della similitudine non
  concordavano più; `MIRROR` su un asse quasi diagonale spostava i punti
  dell'asse). Il responsabile approva la soglia **`2^-48`**: sta sopra il
  residuo di un quarto di giro scritto come `double` (circa `2^-50`) e sposta
  i punti al più di circa `2^-46` volte la grandezza dei dati, sotto la
  tolleranza relativa `2^-42`. Quei casi diventano una fascia dichiarata nel
  commento di `cardinalSinCos` (ADR 0013 di solid-space, punto 4).
- **2026-11-12 — Angoli in gradi ridotti a un giro.** Nel quarto giro di
  rilettura di [#751](https://github.com/Matleyx78/tecnigrafo/pull/751) Codex
  trova che un angolo del retino di molti giri (3600° in *Properties* di
  `-HATCH`) arriva a `cardinalSinCos` come circa 64 radianti, con l'errore
  della conversione da gradi sopra la soglia `2^-48`: il quarto di giro non
  si riconosce e il tratto di bordo sparisce. Il responsabile sceglie di
  ridurre l'angolo **in gradi**, con `std::fmod(gradi, 360)` (esatto), nel
  punto in cui entra, prima di convertirlo: la funzione è
  `reducedDegreesToRadians` di [0.8-PT1](tappe/0.8/PT1-trigonometria-1.md),
  usata dalla lettura dei gruppi 52 e 53
  ([0.8-PT31](tappe/0.8/PT31-retini-dxf.md); il DXF si riscrive con
  l'angolo ridotto, come già gli archi, e l'ADR 0018, punto 6, confronta i
  valori del modello) e dall'angolo digitato in `-HATCH`
  ([0.8-PT44](tappe/0.8/PT44-comando-hatch.md)). Ridurre dopo, in radianti,
  non basta: l'errore nasce nella conversione. Scartata l'altra uscita
  (angolo in radianti senza riduzione, con il quarto di giro riconosciuto
  solo fino a qualche decina di radianti).
- **2026-11-13 — Inserimento di file, punto base e interlinea come AutoCAD.**
  Pubblicando i DXF di prova della 0.8
  ([tappe/0.8/dxf-di-prova.md](tappe/0.8/dxf-di-prova.md)) le cronologie di
  AutoCAD mostrano tre comportamenti diversi dalle schede; il responsabile
  sceglie per tutti e tre quello di AutoCAD. (1) `-INSERT` con il percorso di
  un file il cui nome è già un blocco del disegno inserisce la definizione che
  c'è, senza domande; `nome=percorso` ridefinisce il blocco senza domande,
  come AutoCAD da script, così gli script di AutoCAD funzionano uguali
  ([0.8-PT21](tappe/0.8/PT21-comando-insert.md)). (2) Il blocco preso da un
  file ha punto base (0,0) e le entità spostate di −`$INSBASE`, come le
  scrive AutoCAD ([0.8-PT19](tappe/0.8/PT19-file-come-blocco.md)). (1) e (2)
  correggono l'[ADR 0045](adr/0045-blocchi-e-attributi.md), punto 7.
  (3) L'interlinea di `MTEXT` resta per i testi successivi, come `TSPACETYPE`
  e `TSPACEFAC` ([0.8-PT15](tappe/0.8/PT15-comando-mtext.md)). Scartate le
  alternative: chiedere sempre se ridefinire (gli script di AutoCAD con
  `nome=file` si romperebbero alla domanda in più), tenere `$INSBASE` come
  punto base (dati diversi da quelli di AutoCAD per lo stesso disegno),
  interlinea a 1 per ogni testo (differenza da dichiarare).
- **2026-11-13 — Buchi della revisione della #756.** Il verbale di Codex
  sulla [#756](https://github.com/Matleyx78/tecnigrafo/pull/756) apre sei
  buchi visibili; il responsabile decide. (1) Un file con `$INSBASE` non
  nullo e un'entità che non si sa spostare (non gestita, conservata com'è):
  la definizione tiene come punto base `$INSBASE` e tutte le entità intatte,
  come prima; il disegno è lo stesso e niente si perde
  ([0.8-PT19](tappe/0.8/PT19-file-come-blocco.md)); scartato il rifiuto
  dell'inserimento. (2) In `-INSERT` il primo `=` separa sempre il nome del
  blocco dal percorso, come AutoCAD; un file con `=` nel nome non si inserisce
  così; scartata la precedenza al file letterale. (3) Quando il percorso di un
  file riusa una definizione che c'è, e per ogni blocco preso per nome, la
  scala proposta converte le unità della definizione in quelle del disegno,
  come AutoCAD; scartata la scala 1. (4) Con `nome=percorso`, `Esc` al punto
  d'inserimento conserva la ridefinizione e le tabelle importate, in un passo
  `Insert`, come AutoCAD (è il suo modo di ridefinire senza inserire);
  scartato l'annullamento di tutto il comando (2, 3 e 4 in
  [0.8-PT21](tappe/0.8/PT21-comando-insert.md)). (5) L'interlinea di `MTEXT`
  la ricorda ogni disegno, fra i valori dei comandi, e a ogni avvio riparte da
  *At least* 1, come `POLARANG` e `TRIMEXTENDMODE`; scartato il
  comportamento di AutoCAD (tipo nel profilo, comune ai disegni e conservato
  dopo il riavvio), che avrebbe chiesto il primo valore condiviso fra le
  schede e salvato nelle impostazioni. (6) L'interlinea scelta diventa
  corrente appena confermata, anche se poi `Esc` non crea il testo; scartato
  l'aggiornamento solo alla creazione del testo (5 e 6 in
  [0.8-PT15](tappe/0.8/PT15-comando-mtext.md)).
- **2026-11-13 — Buchi del secondo giro della #756.** Il secondo verbale di
  Codex apre quattro buchi visibili; il responsabile decide. (1) L'eccezione
  di PT19 (punto base `$INSBASE` con le entità intatte) vale solo per le
  entità dello **spazio modello** che non si sanno spostare: lo spazio carta
  non entra nel blocco, e un blocco annidato tiene le sue coordinate
  ([0.8-PT19](tappe/0.8/PT19-file-come-blocco.md)). (2) In `-INSERT` un
  secondo `=` è un errore, `Invalid file name.`: nessun nome di file con `=`
  si inserisce, e il primo resta il separatore di `nome=percorso`; scartato
  il percorso letterale dopo il primo `=`. (3) Con `nome=percorso` e un nome
  nuovo, `Esc` al punto conserva la definizione e le tabelle importate, in un
  passo `Insert`, come per la ridefinizione e come AutoCAD; scartato
  l'annullamento dell'importazione. (4) Un codice di unità del blocco fuori
  dalla tabella di `$INSUNITS` vale come unità sconosciuta (fattore 1) e si
  conserva; scartato il rifiuto dell'inserimento (2, 3 e 4 in
  [0.8-PT21](tappe/0.8/PT21-comando-insert.md)).
