# ADR 0023: Layer e proprietà delle entità

- **Stato:** Accettato (2026-10-25)
- **Sostituisce in parte:** ADR 0011, punto 5, riga «Identificativi» (da
  quale numero partono le entità di una sessione nuova)
- **Data:** 2026-10-25
- **Corretto:** 2026-10-25, prima del codice che lo attua (GOVERNANCE.md):
  punto 1, gli handle degli oggetti di partenza di un disegno nuovo; prima li
  prendevano dal contatore delle entità

## Contesto

Fino alla tappa 0.5 le entità hanno solo la geometria. Layer, colore, tipo di
linea e spessore arrivano dai DXF come gruppi grezzi (ADR 0018, punto 4) e si
riscrivono uguali, ma il programma non li mostra né li cambia. La tappa 0.6 li
porta nel modello, come chiedono l'ADR 0006 (punto 1: tabelle e proprietà
seguono il DXF) e l'ADR 0018 (Conseguenze: ciò che non è ancora rappresentato
resta grezzo).

Le scelte di questo ADR fissano la struttura in `core`, le regole che i comandi
e i pannelli devono rispettare, e ciò che resta grezzo. Colori e tipi di linea
hanno i loro ADR (0024 e 0025).

## Decisione

### 1. Tabelle nel documento

- `core::Document` riceve la **tabella dei layer** e quella dei **tipi di
  linea**, nell'ordine del file; un record nuovo va in fondo.
- Un record ha l'handle del DXF come identificativo (`core::LayerId`,
  `core::LinetypeId`, come `EntityId` per le entità, ADR 0006 punto 1), e
  conserva come gruppi grezzi ciò che il modello non rappresenta (descrizione,
  dizionari d'estensione, XDATA, gruppi sconosciuti).
- **Layer:** nome, colore (indice o RGB, mai `ByLayer` né `ByBlock`, ADR 0024),
  tipo di linea (mai `ByLayer` né `ByBlock`), spessore (mai `ByLayer`,
  `ByBlock` né `Default`, ADR 0025), stampabile, e tre stati: **acceso o
  spento**, **congelato**, **bloccato**.
- Il layer `0` esiste sempre: non si rinomina e non si cancella. Gli altri nomi
  non hanno regole speciali (per `Defpoints` si deciderà con le quote, tappa
  0.8).
- I **nomi** di layer e tipi di linea conservano maiuscole e minuscole ma si
  confrontano senza distinguerle, come in AutoCAD. Non possono essere vuoti,
  superare 255 caratteri, né contenere `< > / \ " : ; ? * | , = ` `` ` ``, né
  iniziare o finire con uno spazio.

#### Handle di un disegno nuovo

Nel DXF tabelle, record, blocchi, dizionari ed entità condividono **un solo
spazio di handle**, e `$HANDSEED` è il primo libero. AutoCAD crea gli oggetti
del suo modello di partenza prima di ogni entità: nel suo disegno vuoto
(`vuoto-mm.dxf` della raccolta `Matleyx78/dxf-test-file`, R2018) sono 148
oggetti, il layer `0` ha l'handle 10 esadecimale e `$HANDSEED` vale 268.

- Tecnigrafo **riserva gli handle da 1 a 256** (da 0x1 a 0x100) agli
  **oggetti di partenza** di un disegno nuovo: record delle tabelle, tabelle,
  blocchi, dizionari, layout.
- Ogni oggetto di partenza ha un handle **fisso**, scritto una volta per
  sempre in una mappa nel codice (`core/include/tg/core/start_handles.hpp`).
  Gli handle si assegnano scendendo da 256: un oggetto aggiunto in futuro
  prende il primo numero libero sotto quelli già dati e non ne sposta
  nessuno. Un controllo in compilazione impedisce che la mappa scenda sotto 1.

  | Handle | Esadecimale | Oggetto |
  |---|---|---|
  | 256 | 100 | tipo di linea `ByBlock` |
  | 255 | FF | tipo di linea `ByLayer` |
  | 254 | FE | tipo di linea `Continuous` |
  | 253 | FD | layer `0` |
  | 252 e sotto | FC e sotto | gli oggetti dello scheletro DXF (tabelle, blocchi `*Model_Space` e `*Paper_Space`, dizionari, layout), assegnati dal pacchetto 0.6-P10 |

- La **prima entità** di un disegno nuovo ha l'handle **257** (0x101); il
  contatore cresce da lì come prima. Gli identificativi non si riusano mai, e
  `$HANDSEED`, l'ultimo handle dato e la scelta dell'ultima entità continuano
  a funzionare.
- La mappa vale solo per un **disegno nuovo**: all'avvio, dopo `NEW` e nelle
  domande al modello. Un **file letto** porta i suoi handle così come sono: il
  lettore parte da un documento vuoto, senza oggetti di partenza, e i numeri da
  1 a 256 restano agli oggetti del file. Un file salvato da un disegno nuovo e
  poi riaperto ha di nuovo la mappa, perché la porta il file stesso.
- Nella modalità di controllo gli identificativi del rapporto sono gli handle
  in decimale (ADR 0011, punto 5): in una sessione nuova la prima entità è
  **257**, non più 1. Il resto della riga «Identificativi» dell'ADR 0011 resta
  valido (nessun identificativo da numeri casuali, elenchi ordinati per
  identificativo).

### 2. Proprietà delle entità

- Ogni `EntityRecord` riceve `core::EntityProperties`:

  | Proprietà | Gruppo DXF | Valore predefinito |
  |---|---|---|
  | layer | 8 | `0` |
  | colore | 62, 420 | `ByLayer` |
  | tipo di linea | 6 | `ByLayer` |
  | scala del tipo di linea | 48 | 1 |
  | spessore | 370 | `ByLayer` |

- Le proprietà fanno parte del record, come la geometria e gli extra: si
  tolgono e si rimettono con l'entità, quindi l'annullamento le conserva.
- Un'entità riferisce layer e tipo di linea per identificativo, non per nome:
  rinominare un layer non tocca le entità. Il nome si scrive solo nel DXF.
- I gruppi 8, 6, 48, 62, 370 e 420 escono da `EntityExtras::common` ed entrano
  nel modello. Restano grezzi e si riscrivono uguali: 430 (colore da un
  catalogo), 440 (trasparenza, tappa 0.9), 60 (visibilità), 284, 347, 390
  (stile e ombre di stampa) e ogni altro gruppo comune. Se l'utente cambia il
  colore di un'entità che ha il gruppo 430, il 430 si toglie: non
  corrisponderebbe più al colore.

### 3. Valori correnti

Le variabili `$CLAYER`, `$CECOLOR`, `$CELTYPE`, `$CELWEIGHT`, `$CELTSCALE` e
`$LTSCALE` escono dalla sezione `HEADER` grezza ed entrano nel documento. Ogni
entità nuova creata da un comando riceve i valori correnti
(`edit::Editor::add`). Cambiarli è un'operazione annullabile, come in
AutoCAD.

### 4. Effetti degli stati

| Stato | Disegnato | Scelto con clic o finestra | Snap | Modificabile |
|---|---|---|---|---|
| acceso | sì | sì | sì | sì |
| spento | no | no | no | no |
| congelato | no | no | no | no |
| bloccato | sì | trovato dalle domande, escluso dalla selezione dei comandi | sì | no |

- Spento e congelato hanno lo stesso effetto finché non ci sono blocchi
  (tappa 0.8), dove si distingueranno come in AutoCAD.
- Il layer corrente si può spegnere (i comandi chiedono conferma) ma non
  congelare.
- Sul layer corrente bloccato si può disegnare, come in AutoCAD.
- La selezione dei comandi scarta le entità dei layer bloccati e lo dice:
  `1 was on a locked layer.` / `<n> were on a locked layer.`
- Nella 0.6 i layer bloccati non si disegnano sbiaditi: arriva con la
  trasparenza, nella tappa 0.9.
- L'estensione del disegno (`Document::extents`, zoom estensione) resta quella
  di tutte le entità.

### 5. Creare, rinominare, cancellare

- **Creare:** nome valido e non usato; valori del layer `0` appena creato
  (colore 7, `Continuous`, spessore predefinito, acceso).
- **Rinominare:** il nome nuovo segue le regole del punto 1. Tecnigrafo
  aggiorna anche il gruppo 8 delle entità non gestite e delle entità nei
  blocchi grezzi. Se il vecchio nome compare in altri dati non gestiti (stati e
  filtri dei layer, XDATA), lo dice **prima** di rinominare e procede: quei
  dati conservano il vecchio nome (ADR 0006, punto 2).
- **Cancellare:** solo un layer diverso da `0` e dal corrente, che nessuna
  entità usa, gestita o no, compresi i blocchi grezzi. Altrimenti un messaggio
  dice perché, e nulla cambia.
- Creare, rinominare, cancellare un layer e cambiarne le proprietà sono
  operazioni annullabili di `edit::Editor`, come le entità.

### 6. Lettura di file incoerenti

- Un'entità che nomina un **layer assente** dalla tabella: il lettore crea il
  layer con i valori del punto 5 e il messaggio d'apertura lo conta
  (`1 missing layer added.`).
- Un'entità che nomina un **tipo di linea assente**: il lettore crea un tipo
  senza tratti (si disegna continuo) con quel nome e lo conta allo stesso modo.
- Due record con lo stesso nome, a meno delle maiuscole: si tiene il primo;
  il secondo resta grezzo e lo si conta.

### 7. Modalità di controllo

Il rapporto e le domande al modello rappresentano layer e proprietà
(`docs/verifica/rapporto.md`, `docs/verifica/domande.md`, pacchetto 0.6-P5):
le proprietà di un'entità si scrivono solo se diverse dal valore predefinito,
la tabella dei layer e i valori correnti solo se diversi da una sessione
nuova, così i rapporti già scritti restano validi. Le domande sul documento
(`pick`, `window`, `crossing`, `snap`) seguono la tabella del punto 4.

## Alternative considerate

- **Layer riferito per nome nell'entità:** è ciò che fa il DXF, ma rinominare
  un layer diventerebbe un'operazione su tutte le entità, e un errore di
  maiuscole creerebbe layer doppi.
- **Proprietà nella variante `Entity`, dentro ogni geometria:** ripeterebbe gli
  stessi campi in cinque tipi oggi e molti di più domani.
- **Cancellare un layer usato spostando le sue entità sul layer `0`:** è
  `LAYDEL` di AutoCAD, un comando a parte; con dati non gestiti che lo nominano
  sarebbe una perdita.
- **Sbiadire i layer bloccati da subito:** serve la trasparenza del render;
  si aggiunge nella tappa 0.9 senza cambiare il modello.
- **Oggetti di partenza presi dal contatore delle entità** (la prima versione
  di questo ADR): ogni oggetto di partenza aggiunto in una tappa sposterebbe
  gli identificativi di tutte le entità, e con loro i rapporti di tutti i casi
  di verifica.
- **Oggetti di partenza dopo le entità** (lo scheletro dello scrittore fino
  alla tappa 0.5): il loro handle dipenderebbe da quante entità si sono
  disegnate prima di salvare.
- **Numerare le entità del rapporto per posizione invece che per handle:**
  cambierebbe il formato del rapporto e delle domande, e il rapporto perderebbe
  il legame diretto con il DXF.

## Conseguenze

- `core::Document`, `edit::Editor`, il lettore e lo scrittore DXF, il rapporto
  e le domande crescono; i comandi esistenti non cambiano, perché le entità
  nuove ricevono i valori correnti dall'editor.
- La tappa 0.8 aggiunge i blocchi: lì spento e congelato si distinguono, e i
  blocchi grezzi diventano modello con le stesse regole.
- Trasparenza, stili di stampa e layer bloccati sbiaditi restano per la
  tappa 0.9.
- Gli handle di partenza cambiano una volta sola gli identificativi delle
  entità create in una sessione nuova: i casi di verifica si riscrivono con
  il pacchetto di riferimenti 0.6-R4, i test unitari con 0.6-P4.
