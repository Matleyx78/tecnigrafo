# ADR 0041: Documenti, ambienti e confini dei moduli

- **Stato:** Accettato
- **Data:** 2026-11-07
- **Completa:** [ADR 0009](0009-interfaccia-grafica.md), punto 1 (gli ambienti
  del registro delle azioni), e [ADR 0030](0030-solid-space-kernel-di-tecnigrafo.md),
  punti 4 e 5 (chi usa quali componenti di solid-space)

## Contesto

Tecnigrafo sarà un programma con più ambienti: disegno, parti, assiemi,
elettrico ([#522](https://github.com/Matleyx78/tecnigrafo/issues/522)). Oggi
c'è solo il disegno, e il codice lo dà per scontato in alcuni punti (letti sul
`main` del 2026-11-06 e verificati da Codex nella #522, il 2026-11-07):

- `MainWindow` tiene un solo `core::Document`, un solo `edit::Editor` e un solo
  `cmd::CommandRunner` per valore; `CadView`, i pannelli *Layers* e
  *Properties* e l'unico listener di `Editor` sono legati a quel documento;
- `edit::Editor` fa quattro lavori: cronologia e annulla, operazioni sulle
  entità, operazioni sulle tabelle (layer, tipi di linea, unità, scala), ciclo
  di vita del file (`fileName`, `hasFile`, `markSaved`, `isModified`,
  `replaceDocument`);
- `cmd::CommandContext` tiene `edit::Editor&` e i valori predefiniti del
  disegno; il runner gestisce selezione, snap, grip e coordinate 2D; il registro
  dei comandi non conosce gli ambienti, quindi un comando digitato vale sempre;
- la nozione di ambiente esiste solo in `ActionSpec::environments` di `app`;
- il server MCP di sviluppo ha una sessione sua (`Level1Session`).

Il responsabile ha deciso (2026-11-07) che alla fine della 0.7.5 l'interfaccia è
pronta per gli ambienti: documenti in schede, ambiente attivo che guida ribbon,
menu e comandi, e un ambiente che il documento attivo non può ospitare apre o
crea un documento del tipo giusto in una scheda nuova (strada A). Codex
consigliava di rinviare registri e documenti multipli al secondo ambiente; vale
la decisione del responsabile, con la cautela che Codex chiedeva: ogni
separazione ha un consumatore concreto in questa tappa o nella 0.8, e i casi di
verifica restano invariati.

## Decisione

### 1. Tipi di documento e ambienti sono due cose

- Il **tipo di documento** dice che cosa c'è nel file (disegno; più avanti
  parte, assieme, progetto elettrico). Ogni tipo ha il suo tipo di file, con
  estensione, contenuto e versione propri, e un ADR suo quando arriva.
- L'**ambiente** dice quali comandi, quale ribbon e quali menu valgono mentre
  lo si modifica. L'ambiente attivo è funzione del **documento attivo** e del
  suo **modo di modifica** (in una parte: la parte o uno schizzo), non una
  proprietà del file.
- Un tipo dichiara gli ambienti che può ospitare e quello in cui si apre. Il
  disegno ospita `drawing2d`, e più avanti l'elettrico (stesso file, strumenti
  diversi).

### 2. Una sessione per documento aperto

- Ogni documento aperto ha una **sessione**: il documento, la sua cronologia,
  il suo runner dei comandi, lo stato della vista (centro, scala), il suo file.
  Le sessioni le possiede l'interfaccia, in un elenco con una sessione attiva.
- **Contratto di attivazione:** vista, riga di comando, pannelli, barra di
  stato, azioni di annulla e ripeti e il listener dei cambiamenti seguono la
  sessione attiva. Cambiare scheda non annulla e non chiude niente: un comando
  in corso resta nel runner della sua sessione e riprende quando la scheda torna
  attiva.
- **Script e MCP** agiscono sulla sessione attiva, con il suo runner e il suo
  annulla, mai in parallelo con l'utente. Il canale MCP dell'utente (tappa 0.8)
  si collega così; il server di sviluppo della 0.6 tiene la sua sessione
  privata, che è una sessione come le altre.

### 3. Ciclo di vita del file fuori da `Editor`

Nome, percorso, presenza di un file e posizione salvata escono da `Editor` in
un componente della sessione. «Modificato» resta **calcolato** dalla cronologia
(posizione salvata, ramo di ripeti, `LWDISPLAY`), non diventa un valore
memorizzato. La richiesta di salvare compare solo quando un documento modificato
si chiude davvero.

### 4. Operazioni sulle tabelle separate da quelle sulle entità

Le operazioni sulle tabelle del documento (layer, tipi di linea, unità, scala;
nella 0.8 stili e definizioni dei blocchi) stanno in un componente separato da
quelle sulle entità, ma nella **stessa transazione** e nella **stessa
cronologia**: separare le classi non separa l'annulla.

### 5. Protocollo dei comandi separato dal contesto di disegno

- Il meccanismo di un comando (registrazione, richieste, parole chiave,
  ingressi, stato, `Esc`) si separa da ciò che è del disegno 2D (l'editor, i
  valori predefiniti, i file). Un comando del disegno riceve il contesto del
  disegno; un ambiente futuro avrà il suo, sopra lo stesso protocollo.
- Selezione, snap, grip, anteprima di entità e coordinate restano dichiarati
  **2D** e restano nel runner del disegno. Non si inventa ora un ingresso o
  un'anteprima universale per il 3D.
- Nessun comando esistente cambia comportamento: i casi di verifica restano
  invariati e fanno da rete di sicurezza.

### 6. Ambienti nei comandi e nelle azioni

- Ogni comando dichiara, nella sua descrizione
  ([ADR 0026](0026-descrizione-dei-comandi.md)), gli ambienti in cui vale; il
  valore predefinito è `drawing2d`.
- Il runner conosce l'ambiente attivo e rifiuta un comando che non vi
  appartiene con un messaggio; oggi l'ambiente è sempre `drawing2d`, quindi
  nessun comando esistente cambia.
- Ribbon, menu, barre e scorciatoie mostrano le azioni dell'ambiente attivo
  (ADR 0009, punto 1: `ActionSpec::environments`). Un ambiente può aggiungere
  schede contestuali al ribbon ([ADR 0039](0039-decisione-visiva-dell-interfaccia.md),
  punto 5).
- Un **ambiente di prova**, registrato solo nei test, dimostra che un secondo
  ambiente cambia ribbon, menu e comandi digitati senza toccare il codice di
  `app` che gestisce schede e sessioni.

### 7. Registro dei tipi di documento

- Un registro piccolo, in `app`: per ogni tipo un identificatore, il nome, le
  estensioni, come crearne uno nuovo, aprirlo e salvarlo, gli ambienti che
  ospita e quello in cui si apre. Lo riempie l'avvio del programma, senza
  plug-in dinamici. Oggi contiene solo il disegno (`.cadx`, `.dxf`).
- La gestione delle sessioni e la cornice dell'interfaccia passano dal
  registro; solo il montaggio iniziale conosce i tipi concreti.
- Un tipo di prova, solo nei test, ospita l'ambiente di prova.

### 8. Un ambiente che il documento attivo non ospita

Se l'utente chiede un ambiente che il documento attivo non può ospitare (per
esempio l'ambiente parti con un disegno aperto), il documento **resta aperto**
nella sua scheda, e il programma apre o crea un documento del tipo giusto in
una **scheda nuova**, come in Inventor. Non si chiude niente, quindi non si
chiede di salvare (strada A, decisione del 2026-11-07). Se c'è un solo tipo che
ospita l'ambiente, se ne crea uno nuovo; se ce ne sono più, l'utente sceglie.

### 9. Pagina iniziale

La scheda *Start* è sempre la prima: nuovo documento (uno per tipo registrato),
apri, file recenti. Non è un documento e non ha una sessione.

### 10. Dipendenze

```
core    → solid_space::robust, geom2d
render  → core
edit    → core
io      → core
cmd     → edit, io
script  → cmd, io
mcp     → script, cmd, io
app     → cmd, render, script
modeling (futuro) → core, solid_space::kernel, features, naming, solver
```

- **Mai in senso inverso:** `core`, `edit`, `io` e `render` non includono né
  collegano `modeling`. Il CAD 2D si compila senza OCCT (ADR 0030, punto 5).
  Quando `modeling` esiste, una prova della CI compila `app` senza di lui.
- **`modeling` → `core`**, non il contrario; `modeling` non collega Qt,
  `render`, `cmd` né `io`.
- I **vincoli 2D**, che useranno gli schizzi e un giorno il disegno, stanno in
  un modulo condiviso che dipende da `core` e dal `solver` di solid-space,
  senza OCCT; non dentro `modeling`. L'ADR della tappa che li introduce lo
  autorizza (ADR 0030, punto 4).
- Lettura e scrittura di parti e assiemi stanno fuori da `io`, che resta 2D.
- Il **contenitore** del `.cadx` (ZIP, `manifest.json`, versione, limiti) si
  estrae da `io` quando arriva il secondo tipo di file, non ora. Allora la
  versione dello schema di un tipo e quella del contenitore restano distinte, e
  i limiti (oggi 256 parti) si rivedono per quel tipo.

### 11. Schizzi

Principi per l'ADR della tappa degli schizzi, che li dettaglia:

- uno schizzo **possiede** un documento geometrico 2D (`core::Document`, che
  resta il disegno DXF senza concetti 3D), più il piano, i vincoli e i dati
  propri; il piano sta nello schizzo, non nel documento;
- i comandi 2D sono gli stessi del disegno; quelli che non possono valere in
  uno schizzo si escludono con gli ambienti (punto 6), non si duplicano;
- annulla: una sessione di modifica dello schizzo, con annulla locale dentro e un
  solo passo della parte quando si termina (strada (a) della #522);
- vincoli e soluzione sono un'unica modifica annullabile; i vincoli puntano a
  chiavi «handle più ruolo» qualificate dallo schizzo, con regole di
  rimappatura, rifiuto o segnalazione per le modifiche che le cambiano;
- unità imposte dalla parte; importazione di un disegno **per copia**, chiedendo
  piano e origine e mostrando l'elenco di ciò che si scarta, che l'utente
  accetta o rifiuta in blocco.

### 12. Che cosa non si fa ora

- `modeling`, i file di parti e assiemi, l'estrazione del contenitore;
- le aree di lavoro (ADR 0009, punto 7, al più tardi con la prima tappa 2.x);
- i vincoli nel disegno CAD: si costruiscono con gli schizzi e poi si
  riportano, con una tappa propria;
- l'elettrico come prodotto o ambiente: nessuna scelta di questo ADR lo
  esclude;
- riferimenti tra file (assiemi, progetti elettrici, tavole): percorso
  relativo più identità del documento, con ricerca e ricollegamento, si decidono
  con il primo tipo che li usa.

## Alternative considerate

- **Rinviare registri e documenti multipli al secondo ambiente** (parere di
  Codex): meno lavoro ora, ma l'interfaccia della 0.7.5 non sarebbe pronta per
  gli ambienti, e il codice della 0.8 crescerebbe sull'unico documento.
- **Programmi separati** per parti e assiemi: più installatori, più CI,
  interfacce da tenere coerenti; contro la direzione di un solo programma
  (ADR 0009).
- **Parti e assiemi come entità del `Document`:** incompatibile con il modello
  DXF ([ADR 0006](0006-modello-dati-dwg-e-formato-cadx.md)).
- **Un solo formato di file per tutti i tipi:** un file cambierebbe significato
  secondo il contenuto, senza versioni e controlli propri.
- **Vincoli 2D dentro `modeling`:** renderebbe il CAD 2D dipendente da
  `modeling` e, per suo tramite, da OCCT.
- **Ambiente come proprietà del file:** non regge lo schizzo dentro una parte
  né l'elettrico sullo stesso file del disegno.

## Conseguenze

- `MainWindow` smette di possedere il documento: lo possiede la sessione, e la
  finestra ne tiene molte.
- Le separazioni di `edit` e `cmd` cambiano molti file senza cambiare il
  comportamento; i casi di verifica e i test esistenti lo provano.
- La 0.8 aggiunge stili, blocchi e quote sopra tabelle separate e sopra il
  contesto del disegno, e il canale MCP sopra la sessione attiva.
- La traccia 2.x si riscrive con questi confini (0.7.5-P32).
- Il registro dei tipi oggi ha un tipo solo: costa poco, e il primo tipo nuovo
  lo trova pronto.
