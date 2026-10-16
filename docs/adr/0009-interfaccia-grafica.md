# ADR 0009: Impianto dell'interfaccia grafica

- **Stato:** Proposto
- **Data:** 2026-10-16

## Contesto

Fino alla tappa 0.3 l'interfaccia è cresciuta un pezzo per volta: riga di comando,
menu *File*, *Edit*, *View*, *Tools* e *Help*, pulsanti `OSNAP` e `ORTHO` nella
barra di stato. Ogni azione è un `QAction` creato a mano in `MainWindow`.

Dalla tappa 0.6 arrivano i primi veri pannelli (gestore dei layer, proprietà della
selezione) e i comandi diventano decine. Se l'impianto della finestra si decide
dopo, pannelli e menu vanno rifatti. Inoltre il modellatore 3D (tappe 2.x) avrà
ambienti diversi (schizzo, parte, assieme, tavola), ciascuno con i suoi strumenti,
come in Inventor: l'interfaccia deve poterli accogliere senza essere riscritta.

Chi viene da AutoCAD conosce la ribbon, ma molti preferiscono ancora menu e barre
degli strumenti classici; AutoCAD offre entrambi.

L'ADR 0002 ha già scelto Qt Widgets. Qt non ha una ribbon propria.

## Decisione

### 1. Registro unico delle azioni

- Ogni azione dell'interfaccia (comandi di disegno e modifica, annulla, zoom,
  apertura di pannelli, interruttori come snap e orto) si dichiara **una sola
  volta** in un registro del modulo `app`, con: identificatore stabile in inglese
  (per esempio `draw.line`), testo, descrizione, icona, scorciatoia predefinita,
  eventuale comando di `cmd` che la esegue, e gli **ambienti** in cui vale.
- Ribbon, menu, barre degli strumenti, barra di stato e finestra delle
  scorciatoie si costruiscono leggendo il registro. Nessun `QAction` si crea fuori
  dal registro.
- L'unico ambiente della serie 0.x è `drawing2d`. Gli ambienti del 3D si
  aggiungeranno con le tappe 2.x, senza cambiare il registro.

### 2. Ribbon e interfaccia classica, a scelta dell'utente

- Due interfacce, costruite dallo stesso registro:
  - **Ribbon** (predefinita): schede e gruppi come in AutoCAD.
  - **Classica**: barra dei menu e barre degli strumenti.
- L'utente sceglie dalle impostazioni e con un pulsante nella barra di stato; il
  cambio vale subito, senza riavviare. Il menu dell'applicazione resta sempre
  raggiungibile anche con la ribbon.
- La riga di comando c'è in entrambe e non si può togliere.
- La ribbon si realizza con la libreria **SARibbon**
  (<https://github.com/czyt1988/SARibbon>, licenza MIT, compatibile con la
  GPLv3 per l'ADR 0003). È l'unica nuova dipendenza ammessa da questo ADR, e solo
  nel modulo `app`. La versione si fissa (tag o commit) e il modo di includerla si
  decide nel piano della tappa 0.6. Prima di usarla, un pacchetto di prova verifica
  che si compili senza avvisi con la versione minima di Qt dell'ADR 0002 su
  Windows, Linux e macOS; se non è possibile, serve un ADR che cambi la versione
  minima o la libreria, prima di procedere.

### 3. Pannelli agganciabili

- Ogni pannello (layer, proprietà, e quelli futuri) è un `QDockWidget`: si
  aggancia ai bordi, si raggruppa a schede, si stacca come finestra.
- La disposizione dei pannelli e la scelta tra ribbon e classica si salvano
  all'uscita e si ripristinano all'avvio.
- Nessuna libreria di aggancio esterna per ora. Se `QDockWidget` si rivelerà
  insufficiente, si valuterà con un nuovo ADR.

### 4. Icone

- Tutte le icone sono **nostre**, disegnate per Tecnigrafo in un unico stile, in
  SVG, rilasciate con la licenza del progetto. Nessuna icona copiata da altri
  programmi o raccolte.
- Sono monocromatiche: il colore lo dà il tema, così una sola icona vale per il
  chiaro e lo scuro. Un secondo colore d'accento è ammesso solo dove aiuta a
  capire (per esempio il punto agganciato di uno snap).
- Le regole di disegno (griglia, spessori, dimensioni) si scrivono in
  `docs/ICONE.md` insieme alle prime icone.

### 5. Tema chiaro o scuro

- Due temi, **chiaro** (predefinito) e **scuro**, scelti dall'utente nelle
  impostazioni; il cambio vale subito.
- Si usa lo stile *Fusion* di Qt con due tavolozze nostre, uguale sui tre
  sistemi. Anche lo sfondo del disegno ha un colore per tema.
- Seguire il tema del sistema operativo richiede Qt 6.5: si aggiungerà quando la
  versione minima di Qt lo permetterà.

### 6. Scorciatoie personalizzabili

- Ogni azione del registro ha una scorciatoia predefinita che l'utente può
  cambiare o togliere da una finestra dedicata. Si salvano solo le differenze
  dai valori predefiniti.
- La finestra segnala i conflitti (stessa scorciatoia su due azioni dello stesso
  ambiente) e non li permette.
- Le scorciatoie si possono esportare e importare in un file.
- Le parole della riga di comando (`L`, `LINE`, `U`…) non sono scorciatoie e
  restano quelle dell'ADR 0005.

### 7. Aree di lavoro: rimandate

Un'area di lavoro (quali schede, barre e pannelli mostrare, e dove) si deciderà
con un ADR dedicato, al più tardi con la prima tappa 2.x, insieme agli ambienti del
3D. Fino ad allora le uniche "aree" sono la scelta tra ribbon e classica e la
disposizione salvata dei pannelli. Il registro del punto 1 è pensato perché le
aree di lavoro si possano aggiungere senza cambiarlo.

## Alternative considerate

- **Solo menu e barre degli strumenti**: più semplice, ma lontano da ciò che chi
  viene da AutoCAD e Inventor si aspetta oggi.
- **Solo ribbon**: toglie la scelta a chi preferisce l'interfaccia classica, e
  lega tutta l'interfaccia a una libreria esterna.
- **Ribbon scritta da noi**: nessuna dipendenza, ma molto lavoro su un componente
  che non è il cuore del CAD.
- **Librerie ribbon commerciali** (per esempio QtitanRibbon): a pagamento e non
  ridistribuibili liberamente, quindi inadatte a un progetto libero.
- **Qt Advanced Docking System** per i pannelli (LGPL): più ricco di
  `QDockWidget`, ma oggi non serve; resta un'opzione per il futuro.
- **Icone da raccolte libere esistenti**: più rapido, ma stile non uniforme e
  licenze da verificare una per una.

## Conseguenze

- Aggiungere un comando all'interfaccia vuol dire aggiungere una voce al registro:
  ribbon, menu e scorciatoie lo vedono senza altro lavoro.
- I `QAction` creati a mano in `MainWindow` nelle tappe 0.2 e 0.3 vanno portati
  nel registro (tappa 0.6).
- Ogni nuovo comando deve avere la sua icona: disegnare le icone diventa un lavoro
  continuo, da prevedere nei pacchetti delle tappe.
- SARibbon va seguita come ogni dipendenza; se un giorno venisse abbandonata,
  l'interfaccia classica continua a funzionare e la ribbon si può sostituire
  senza toccare il registro.
- I testi del registro passano per il sistema di traduzione della tappa 0.9.
