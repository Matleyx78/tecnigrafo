# ADR 0039: Decisione visiva dell'interfaccia

- **Stato:** Accettato
- **Data:** 2026-11-07

## Contesto

L'impianto dell'interfaccia della 0.6 funziona (ribbon SARibbon, pannelli
agganciabili, registro unico delle azioni, temi, scorciatoie:
[ADR 0009](0009-interfaccia-grafica.md)), ma a vederlo Tecnigrafo resta lontano
da AutoCAD e da Open CAD Studio. Dal confronto della
[#483](https://github.com/Matleyx78/tecnigrafo/issues/483) la distanza non è
tecnologica: manca lo strato visivo sopra Qt. Il tema è solo una `QPalette` su
Fusion, con una sola superficie per tutto; le icone sono monocrome; il ribbon
è largo, con una fascia vuota sopra le schede e la scheda *File* tagliata; i
pannelli sono moduli a tutta larghezza.

Il responsabile ha deciso (2026-11-07) che alla fine della tappa 0.7.5
Tecnigrafo si presenta praticamente come le schermate di riferimento della
#483 ed è pronto per gli ambienti. Il riferimento è il mockup «prima versione»
del 2 novembre 2026, che sta nel raccoglitore privato di idee grafiche del
responsabile: un documento pubblico non lo collega e niente di lì entra così
com'è. Questo ADR lo descrive per intero, e le immagini di riferimento entrano
nel repository rifatte come file propri (sotto, punto 7).

Il riferimento visivo non è una funzione: quello che mostra e che il programma
non sa ancora fare arriva con le tappe dei comandi (punto 6).

## Decisione

### 1. Il riferimento visivo, area per area

Dall'alto in basso, nei due temi:

1. **Barra del titolo**, propria (finestra senza cornice, punto 4): a sinistra
   l'icona del programma e la **barra di accesso rapido** (nuovo, apri, salva,
   annulla, ripeti, icone piccole senza testo); al centro `<documento> —
   Tecnigrafo`; a destra i tre pulsanti della finestra.
2. **Schede del ribbon**, sulla stessa fascia del titolo o subito sotto, senza
   spazio vuoto: la scheda **File** a sinistra, piena, nel colore d'accento con
   il testo chiaro; poi le schede del ribbon, la corrente con un bordo superiore
   nel colore d'accento e il fondo del ribbon.
3. **Ribbon compatto, a gruppi**, nella scheda *Home*: *Draw*, *Modify*,
   *Annotation*, *Layers*, *Properties*, *Utilities*. In un gruppo:
   - pulsanti grandi (icona 32 px, testo sotto, freccia se hanno varianti);
   - pile di tre pulsanti piccoli (icona 16 px e testo a destra, o solo icona);
   - menu a tendina per le varianti di un comando, con l'icona di ogni
     variante (per `CIRCLE`: *Center, Radius*; *Center, Diameter*; *2-Point*;
     *3-Point*; *Tan, Tan, Radius*);
   - il nome del gruppo in una fascia bassa sotto il gruppo, nel testo
     secondario, con la freccia del gruppo esteso dove serve.
   Nel gruppo *Layers* la combo dei layer con le icone di stato (acceso,
   scongelato, sbloccato) e il campione di colore; nel gruppo *Properties* tre
   combo (colore, tipo di linea, spessore) con il loro campione e un'icona
   piccola a sinistra.
4. **Riga delle schede dei documenti**, sotto il ribbon: a sinistra il pulsante
   del menu dei documenti aperti, poi la scheda *Start* e una scheda per
   documento, con il nome, l'indicatore di modifica e la crocetta.
5. **Area di disegno** su tutta la larghezza rimasta, con griglia leggera,
   assi dell'origine in basso a sinistra e la **barra di navigazione** in alto
   a destra dentro l'area (sposta, zoom, zoom estensioni), verticale, su un
   fondo arrotondato.
6. **Riga di comando mobile**, sopra il fondo dell'area di disegno, con le
   ultime righe della cronologia in alto e la riga di immissione sotto:
   crocetta, simbolo del prompt, nome del comando, richiesta, opzioni tra
   parentesi quadre nel colore d'accento. Testo a passo fisso.
7. **Immissione dinamica** vicino al cursore
   ([ADR 0042](0042-immissione-dinamica.md)): un riquadro con la richiesta e
   due campi (lunghezza e angolo), quello attivo nel colore d'accento.
8. **Pannello *Properties***, agganciato a destra: in testa una combo con il
   tipo e il numero degli oggetti selezionati e due pulsanti (filtro, selezione
   rapida); sotto una **griglia** a due colonne, a gruppi che si chiudono
   (*General*, *Geometry*), con righe da 20–22 px; nella geometria di una
   polilinea lo scorrimento dei vertici (*Current Vertex* con le frecce,
   *Vertex X*, *Vertex Y*), poi lunghezza e area in sola lettura.
9. **Pannello *Layers***, agganciato a destra sotto *Properties*: barra di
   icone (nuovo, elimina, rendi corrente), campo di ricerca, tabella con
   colonne a icone (acceso, congelato, bloccato) e colonne di testo (nome,
   colore con il campione, tipo di linea); il layer corrente con la spunta e il
   fondo di selezione. Nessuna barra di scorrimento orizzontale finché le
   colonne entrano.
10. **Barra di stato**: a sinistra lo spazio per le schede *Model* e *Layout*
    (vuoto fino a dopo la 1.0, punto 6) e le coordinate del puntatore; a destra
    gli **interruttori a icone** (griglia, snap alla griglia, orto, polare, snap
    a oggetto, spessori, immissione dinamica), quelli accesi con il fondo di
    selezione, poi la scala e il pulsante delle impostazioni; lo spazio per
    l'**indicatore dell'ambiente** (punto 5).

### 2. Gettoni del tema

I colori dell'interfaccia sono **gettoni** con un nome, definiti una volta per
tema in `app/src/theme.cpp` e usati dal foglio di stile, dal `QProxyStyle` e
dalle icone; nessun altro file scrive un colore dell'interfaccia. I valori
iniziali sono quelli del riferimento:

| Gettone | Chiaro | Scuro | Dove |
|---|---|---|---|
| `surface.chrome` | `#dde1e6` | `#1e2228` | titolo, schede del ribbon, barra di stato |
| `surface.ribbon` | `#f4f5f7` | `#2b3038` | ribbon, intestazioni dei pannelli |
| `surface.group` | `#e9ecef` | `#262a31` | fascia dei nomi dei gruppi, intestazioni dei gruppi nelle griglie |
| `surface.tabs` | `#e4e7eb` | `#23272d` | riga delle schede dei documenti |
| `surface.panel` | `#f7f8fa` | `#2b3038` | fondo dei pannelli e della riga di comando |
| `surface.field` | `#ffffff` | `#20242a` | campi, combo, riga di immissione |
| `surface.canvas` | `#ffffff` | `#1b1f24` | area di disegno |
| `border` | `#cdd3da` | `#3b424d` | bordi e separatori |
| `text` | `#1f2328` | `#e4e7eb` | testo |
| `text.secondary` | `#68707a` | `#9098a3` | nomi dei gruppi, etichette nelle griglie |
| `accent` | `#1f6fbf` | `#2a72b5` | scheda *File*, campo attivo, opzioni della riga di comando |
| `selection` | `#dce8f4` | `#304357` | righe e voci selezionate, interruttori accesi |
| `icon.geo`, `icon.pt`, `icon.cost`, `icon.esist` | | | colori dei ruoli delle icone ([ADR 0040](0040-icone-a-ruoli.md)) |

Un solo colore d'accento. I colori dell'area di disegno (entità, griglia,
anteprima, evidenziazione) restano quelli di `render::SceneStyle`; cambiano
solo per allineare lo sfondo a `surface.canvas`. I valori si possono ritoccare
nei pacchetti, tenendo i nomi; la galleria (punto 7) mostra l'effetto.

### 3. Densità

- Righe di 20–22 px nei pannelli e nelle griglie.
- Nel ribbon pulsanti grandi con icona da 32 px e pulsanti piccoli con icona da
  16 px, in pile da tre.
- Nessun bordo intorno ai pulsanti a riposo: il bordo e il fondo di selezione
  compaiono al passaggio del puntatore e quando sono premuti.

### 4. Come si realizza

- Fusion resta lo stile di base (ADR 0009, punto 5); sopra, un **foglio di
  stile** generato dai gettoni e un **`QProxyStyle`** per ciò che il foglio di
  stile non controlla bene (barre di scorrimento, schede, combo, intestazioni
  dei pannelli).
- Ribbon: lo **stile compatto** di SARibbon e la sua **finestra senza
  cornice**, con la barra di accesso rapido nella fascia del titolo. Prima di
  fissarli, il pacchetto della cornice (0.7.5-P12) li prova sulla versione in uso
  (2.9.5) con Windows, X11 e Wayland. Dove la finestra senza cornice non regge
  (per esempio sotto Wayland), si tiene la cornice del sistema, con la barra di
  accesso rapido sotto il titolo: la differenza si scrive in questo ADR quando
  il pacchetto l'ha provata.
- Nessuna libreria nuova per lo stile.

### 5. Posto per ambienti e documenti

Le schermate lasciano posto, anche se nella 0.7.5 c'è solo il disegno:

- all'**indicatore dell'ambiente** nella barra di stato, prima degli
  interruttori, con l'icona e il nome dell'ambiente attivo;
- alle **schede contestuali** del ribbon, che compaiono a destra delle schede
  fisse in un colore proprio quando un ambiente le chiede;
- a **schede di documenti di tipo diverso**: ogni scheda porta l'icona del tipo
  di documento.

Le regole di ambienti e documenti sono nell'[ADR 0041](0041-documenti-e-ambienti.md).

### 6. Che cosa del riferimento non entra nella 0.7.5

Sono funzioni, non grafica, e arrivano con la loro tappa:

- i testi in italiano: le traduzioni sono della tappa 0.9, e fino ad allora
  l'interfaccia è in inglese (per questo i nomi del punto 1 sono in inglese);
- i comandi che non esistono ancora (per esempio *Stretch*, *Array*,
  *Dimension*, *Table*, *Measure*): gruppi e schede si riempiono con le tappe
  dei comandi, e nella 0.7.5 il ribbon mostra solo i comandi che esistono;
- le schede *Model* e *Layout* della barra di stato: lo spazio carta è dopo la
  1.0;
- ViewCube e barra di navigazione del 3D, che appartengono all'ambiente parte.

### 7. Verifica

- **Immagini di riferimento** nel repository, in `docs/adr/0039/`, rifatte
  come file propri (0.7.5-P9): finestra intera nei due temi, più un dettaglio
  per ribbon, pannello *Properties*, pannello *Layers*, barra di stato e
  immissione dinamica: [finestra, tema chiaro](0039/finestra-chiaro.svg),
  [finestra, tema scuro](0039/finestra-scuro.svg), [ribbon](0039/ribbon.svg),
  [*Properties*](0039/proprieta.svg), [*Layers*](0039/layer.svg),
  [barra di stato](0039/stato.svg),
  [immissione dinamica](0039/immissione-dinamica.svg); che cosa mostra ognuna
  è nel [README](0039/README.md).
- **Galleria di schermate** generata in CI per ogni pull request che tocca
  `app/` (piattaforma `offscreen` su Linux, due temi), da confrontare con le
  immagini di riferimento senza compilare (0.7.5-P3).
- **Prova manuale** della tappa, su Windows e Linux.

## Alternative considerate

- **Solo un foglio di stile, senza ADR:** più rapido, ma ogni agente
  interpreterebbe «bello» a modo suo, e senza un riferimento fissato non si può
  rivedere.
- **Passare a Qt Quick/QML:** più libertà grafica, ma vorrebbe dire riscrivere
  tutto `app/` e il ribbon; il limite non è Qt Widgets.
- **Uno stile Qt di terze parti:** una dipendenza in più (ADR 0002) per una cosa
  che foglio di stile e `QProxyStyle` fanno.
- **Collegare il mockup del raccoglitore:** chi legge il repository non può
  aprirlo, e il riferimento deve stare con il codice che lo realizza.

## Conseguenze

- I colori dell'interfaccia stanno in un posto solo; un tema nuovo è una
  tabella di gettoni.
- I pacchetti di `app` della 0.7.5 hanno un riferimento con cui confrontarsi, e
  il responsabile lo controlla dalla galleria.
- Le tappe successive aggiungono gruppi e schede del ribbon nello stesso stile,
  e le loro icone con le regole dell'ADR 0040.
- Il punto 4 ha una parte da completare dopo la prova di SARibbon con Wayland.
