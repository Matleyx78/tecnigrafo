# ADR 0055: Griglia, snap alla griglia e icona dell'UCS

- **Stato:** Proposto
- **Data:** 2026-11-21

## Contesto

Oggi la griglia è sempre accesa con un passo che segue lo zoom
(`render::gridSpacing`), non c'è lo snap alla griglia e gli assi sono due rette
per l'origine (idea [#664](https://github.com/Matleyx78/tecnigrafo/issues/664)).
Il responsabile ha deciso per la 0.8.1 ([DECISIONI.md](../DECISIONI.md),
2026-11-21, «Testo formattato, griglia, snap e attributi della 0.8.1», punti
2–5, ed «Esiti dei DXF di prova e della seduta della 0.8.1», punti 2, 3 e 5)
che lo stato sta nel disegno come in AutoCAD, nella voce `*ACTIVE` della
`VPORT`, con i valori di `acadiso.dwt` per un disegno nuovo. I DXF di prova
`autocad-griglia-*`, `autocad-ucsicon-*` e `autocad-snap-*`
([tappe/0.8.1/dxf-di-prova.md](../tappe/0.8.1/dxf-di-prova.md)) confermano i
gruppi della `VPORT`.

## Decisione

### 1. Stato nel disegno

`core::GridSettings`, un campo di `Document` con il suo setter:

| Campo | `VPORT` | Disegno nuovo |
|---|---|---|
| `gridOn` (`GRIDMODE`) | 76 | acceso |
| `gridUnit` (`GRIDUNIT`, X e Y; 0,0 = segue lo snap) | 15, 25 | 10, 10 |
| `snapOn` (`SNAPMODE`) | 75 | spento |
| `snapUnit` (`SNAPUNIT`, X e Y, > 0) | 14, 24 | 10, 10 |
| `snapBase` (`SNAPBASE`) | 13, 23 | 0, 0 |
| `gridDisplay` (`GRIDDISPLAY`, bit) | 60 | 3 |
| `gridMajor` (`GRIDMAJOR`, da 1 a 100) | 61 | 5 |
| `ucsIcon` (`UCSICON`, bit 1 acceso, bit 2 nell'origine) | 74 | 3 |

`SNAPANG` (50, gradi), `SNAPSTYL` (77) e `SNAPISOPAIR` (78) si leggono e si
riscrivono uguali, senza effetto: con un valore diverso da 0 griglia e snap
restano diritti, e il manuale lo dice. `SNAPTYPE` non sta nel disegno (è del
profilo): lo snap polare resta fuori. Di `GRIDDISPLAY` contano il bit 2
(adattiva) e il 4 (suddivisione); gli altri si conservano.

### 2. File

- **DXF:** si legge la voce `*ACTIVE` della `VPORT` (la prima con quel nome,
  senza distinguere maiuscole e minuscole) e si riscrivono solo i suoi gruppi del
  punto 1, nello stesso posto; tutti gli altri gruppi e le altre voci restano
  intatti (ADR 0006). Un gruppo che manca vale come in un disegno nuovo, e in
  scrittura si aggiunge solo se il valore è diverso; senza `*ACTIVE` lo stato è
  quello di un disegno nuovo e la voce si scrive come oggi.
- **Formato nativo:** gli stessi campi, con i nomi delle variabili.

### 3. Comandi e tasti

- `F7` e `F9` accendono e spengono griglia e snap del **disegno corrente**, con
  `<Grid on>`, `<Grid off>`, `<Snap on>`, `<Snap off>`; non entrano
  nell'annullamento e segnano il disegno modificato. Lo stesso per i comandi
  qui sotto, come le variabili di AutoCAD che stanno nel disegno.
- Variabili come comandi: `GRIDMODE`, `GRIDUNIT`, `SNAPMODE`, `SNAPUNIT`,
  `SNAPBASE`, `GRIDDISPLAY`, `GRIDMAJOR`, con `Enter new value for <NOME>
  <valore>:` come le altre.
- `GRID`: `Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect]
  <10.0000>:`; *Snap* mette `GRIDUNIT` 0,0, *Major* chiede `GRIDMAJOR`,
  *aDaptive* chiede i due bit, *Aspect* i due passi.
- `SNAP`: `Specify snap spacing or [ON/OFF/Aspect] <10.0000>:`.
- `UCSICON`: `Enter an option [ON/OFF/Noorigin/ORigin] <ON>:`.

Le parole delle richieste vengono dall'AutoCAD italiano tradotto: le
cronologie di `GRID`, `SNAP`, `UCSICON`, `F7` e `F9` sono **da registrare**
(nessun DXF di prova le ha).

### 4. Snap alla griglia

- Con lo snap acceso un punto **indicato** (clic, puntatore della
  preview) va sul nodo più vicino, asse per asse, con la regola di
  [REGOLE-NUMERICHE.md](../REGOLE-NUMERICHE.md), sezione 3, punto 6. Le
  coordinate scritte non si toccano.
- Uno snap a oggetto trovato vince sulla griglia.
- Con l'orto, il polare o il tracciamento il punto va **prima** sul nodo, poi
  le guide lo proiettano (REGOLE-NUMERICHE.md, sezione 4, «Con lo snap alla
  griglia»). **[D3]** Per il tracciamento la regola non c'è: questa è una
  proposta, da confermare insieme alla prova in AutoCAD LT di orto e polare.
- Con `gridUnit` 0,0 la griglia ha il passo dello snap.

### 5. Disegno della griglia

- Passo fisso `gridUnit` (o `snapUnit`), con una linea principale ogni
  `gridMajor`; nessun passo automatico.
- **Adattiva** (bit 2): quando le linee secondarie sarebbero più vicine di
  `SceneStyle::minGridSpacingPx`, si mostrano solo le principali; quando anche
  quelle lo sarebbero, il passo si moltiplica per `gridMajor` finché non lo è
  più. Senza il bit, sotto la soglia la griglia non si disegna.
- **Suddivisione** (bit 4, con il 2): ingrandendo, quando una maglia supera
  `gridMajor · minGridSpacingPx` si divide per `gridMajor`, mai sotto lo snap.
- La griglia si disegna su tutta la vista (non ci sono limiti).

### 6. Icona dell'UCS

- Per difetto, come AutoCAD: due frecce corte, X rossa e Y verde, con le
  lettere, di grandezza fissa in pixel (misura e forma dalla schermata di
  AutoCAD della #920); nell'origine se `ucsIcon` ha il bit 2 e l'icona sta
  intera nella vista, altrimenti nell'angolo in basso a sinistra; senza il bit 1
  non si disegna.
- `render` riceve un **sistema di riferimento** (`AxisFrame`: origine, due
  direzioni unitarie, due lettere): nel 2D quello del disegno (X, Y); lo schizzo
  del 3D darà il suo.
- Le due rette di oggi restano come **preferenza del programma**
  (Impostazioni), non del disegno: icona (predefinita), rette, niente.

### 7. Verifica

- Il rapporto ha il campo `grid snap:` **solo con lo snap acceso**
  (`grid snap: unit=10,10 base=0,0`): con lo snap spento nessun caso di oggi
  cambia, anche quelli che aprono un DXF con la griglia spenta o un altro
  passo. Il resto dello stato si legge negli script dall'eco delle variabili
  (`Enter new value for GRIDUNIT <5.0000,10.0000>:`) e nelle domande con
  `grid D`.
- Negli script lo snap si accende con `SNAPMODE` o `SNAP`; i tasti con `!key
  F7` e `!key F9` (livello 2). Nessuna direttiva nuova.

## Alternative considerate

- **Preferenza del programma invece del disegno:** scartata dal responsabile
  (decisione citata, punto 2).
- **Campo `grid:` nel rapporto quando lo stato è diverso da una sessione
  nuova:** cambierebbe i 67 casi che aprono un DXF, quasi tutti con la griglia
  spenta o a passo 0,5.
- **Passo automatico come opzione:** sostituito dalla griglia adattiva di
  AutoCAD (decisione citata, punto 3).

## Conseguenze

- Un disegno nuovo salvato in DXF ha la `VPORT` con i valori di `acadiso.dwt`.
- `render::gridSpacing` e il passo automatico spariscono.
