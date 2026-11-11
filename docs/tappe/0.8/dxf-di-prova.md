# DXF di prova della tappa 0.8

Disegni da preparare dal responsabile in AutoCAD LT, da zero, senza dati di
lavoro e senza riferimenti al suo nome o alla sua licenza, come per la 0.7
([0.7/dxf-di-prova.md](../0.7/dxf-di-prova.md)). Vanno nel repository
[Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file) (CC0), in
DXF 2018, in millimetri (`$INSUNITS` 4, `$MEASUREMENT` 1), partendo da
`acadiso.dwt` (non dal modello di lavoro personalizzato). Accanto a ogni disegno
lo script eseguito (`.scr`) e la **cronologia dei comandi** copiata da `F2`:
richieste e messaggi dei comandi di Tecnigrafo si prendono da lì.

Chi scrive i riferimenti legge il risultato di AutoCAD con l'oracolo ezdxf
(ADR 0020, esteso alle entità della tappa da PT3), anche per i campioni
dell'elettrico, che non vengono da AutoCAD; i valori attesi si copiano
dal log dell'oracolo, non si calcolano a mano e non si ricavano da Tecnigrafo.
Le hash SHA-256 si scrivono qui quando i file sono pubblicati.

| File | Che cosa contiene | Riferimenti |
|---|---|---|
| `autocad-solid.dxf` | `SOLID` da 3 e da 4 vertici, uno «a farfalla», uno su un layer con colore, uno specchiato | RT10 |
| `autocad-ellissi.dxf` | ellissi fatte con `ELLIPSE` in tutti i modi (asse e distanza, *Center*, *Rotation*), archi d'ellisse con angoli e con *Included angle*, un'ellisse ruotata di 30°, una specchiata, una scalata `(2, 1)` dentro un blocco ed esplosa | RT48, RT49, RT50 |
| `autocad-stili-testo.dxf` | stili `Standard`, uno con altezza fissa 5, uno con fattore di larghezza 0,8 e obliquo 15°; un `TEXT` per stile | RT11, RT12 |
| `autocad-testo-multiriga.dxf` | `MTEXT` con larghezza 0 e 60, i nove punti di allineamento, due paragrafi (`\P`), una parola più larga della colonna, interlinea 1,5, rotazione 30°; uno con formati (`\f`, `\H`, `\C`, `\S1/2;`) | RT13, RT14, RT15 |
| `autocad-blocchi.dxf` | un blocco semplice; uno con entità sul layer `0` e colore *DaBlocco*; uno annidato in un altro; inserimenti con scala 1, 2, −1 (specchiato), 2 e 1 (non uniforme) e rotazione 30°; un blocco con `ATTDEF` variabile, invisibile, costante e preimpostato, inserito con i valori predefiniti (`ATTREQ` 0); un inserimento specchiato con `MIRROR` e `MIRRTEXT` 0; un blocco ridefinito dopo l'inserimento; un blocco non esplodibile | RT16, RT17, RT18, RT20, RT21 |
| `autocad-esplodi.dxf` | copie degli inserimenti di `autocad-blocchi.dxf` esplose, compreso il blocco con attributi; un inserimento con scala `(2, 1)` di un blocco con un cerchio, un arco e un testo, esploso (dà ellissi); una quota esplosa | RT29 |
| `simbolo-contatto.dxf`, `simbolo-con-tipo-di-linea.dxf` | due «simboli» da inserire come file: `$INSBASE` diverso dall'origine, un `ATTDEF` costante e uno variabile, un layer e un tipo di linea che il disegno di destinazione ha con proprietà diverse | RT19, RT21 |
| `autocad-inserisci-file.dxf` | il disegno in cui i due simboli sono stati inseriti con `-INSERT`, una volta con il nome già esistente e *Redefine* = *Yes* | RT19, RT21 |
| `autocad-quote.dxf` | con `ISO-25`: lineari orizzontale, verticale e ruotata, allineata, angolare fra due linee, radiale e diametrale su un cerchio e su un arco, le quote **strette** dei quattro casi dell'adattamento (testo e frecce dentro, solo il testo dentro, solo le frecce dentro, tutto fuori) sia lineari sia allineate, e radiali e diametrali in un cerchio piccolo; con uno stile `PROVA` (`DIMTAD` 0, `DIMDEC` 3, `DIMDSEP` `.`, `DIMTXT` 3,5); una con testo sostituito `<> mm`; una quota ruotata, una scalata e una specchiata dopo la creazione; una quota coordinata (tipo che Tecnigrafo conserva soltanto) | RT22, RT23, RT24, RT25, RT26, RT27, RT28 |
| `autocad-retini.dxf` | `ANSI31`, `NET` e `SOLID` su un rettangolo di quattro linee con un cerchio dentro (punto interno) e con isole annidate; un retino da *Select objects*; un motivo ruotato di 30° e scalato 2; gli stili *Normal*, *Outer* e *Ignore*; un retino con un motivo diverso (`ANSI37`) | RT30, RT31, RT42, RT43, RT44 |
| `autocad-pedit.dxf` | linee e archi convertiti in polilinee; *Join* con distanza 0; *Multiple* con distanza 2 e *Extend*, *Add* e *Both*, su linee che non si toccano, su linee che si sovrappongono e su un arco seguito da una linea; *Close*, *Open*, *Reverse*, *Ltype gen* | RT32, RT33 |
| `campioni-elettrico/` | otto simboli della libreria dell'elettrico (`TG_EL_CONTACT_NO`, `TG_EL_COIL_DELAY_OFF`, `TG_EL_BREAKER_3P`, `TG_EL_TRANSFORMER_1PH`, `TG_EL_MOTOR_3PH`, `TG_EL_TERMINAL`, `TG_EL_FOLIO_OUT`, `TG_EL_PUSHBUTTON_NO`) e lo schema `avviamento-diretto.dxf`, scritti dallo script della libreria (DXF R2000), non da AutoCAD; campioni di prova, non la libreria (decisione del 2026-11-11, punto 8); commit `764dfdb9420623d26636e1552769cc223e484231`, hash qui sotto | RT18, RT19, RT21 |

## Che cosa provare a mano in AutoCAD LT

Oltre ai disegni, da scrivere nella pull request che li pubblica:

1. `-DIMSTYLE` → *STatus* di `Standard` e `ISO-25` in un disegno nuovo da
   `acadiso.dwt` (ADR 0047, punto 2).
2. Tracciamento: in `LINE`, acquisiti due punti, dopo aver indicato il primo
   punto i segni dei punti acquisiti restano o spariscono? (ADR 0049, punto 2.)

## Campioni dell'elettrico

Commit `764dfdb9420623d26636e1552769cc223e484231` di
[Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file/tree/764dfdb9420623d26636e1552769cc223e484231/campioni-elettrico),
cartella `campioni-elettrico/`, CC0. Copiati da `tec-ss-grafica` al commit
`cec2edd`; scritti dallo script della libreria (DXF R2000, fine riga CRLF).

| File | SHA-256 |
|---|---|
| `TG_EL_BREAKER_3P.dxf` | `cfde6e3f8597cbface5b47df3ffaf306eeb80a1d455f7faebae879ac4c755c05` |
| `TG_EL_COIL_DELAY_OFF.dxf` | `fdba66ca1135c7cf32ee730cf37a21db6aebd05e8e7854042bcd4475d94f972d` |
| `TG_EL_CONTACT_NO.dxf` | `f576ba399064f16520794ef217214e5dbcdec1612c9886898ed3e510d2a89d21` |
| `TG_EL_FOLIO_OUT.dxf` | `0ef1d6e6ed516f785df6eb3256b02bac91481e527ca38a2cef15a0569b1f0a9d` |
| `TG_EL_MOTOR_3PH.dxf` | `15d778e0b43c9acb755b23d995db984f51e089f4679f99dc3adaccc04635c833` |
| `TG_EL_PUSHBUTTON_NO.dxf` | `82b394687bda6d262ac42aad530e04cc9a392c8d4a813c74dab9629382d668a1` |
| `TG_EL_TERMINAL.dxf` | `7edaa904c0e2846d10e4285faa248ea6a81b7be12eff13a067b96fe52d2149fe` |
| `TG_EL_TRANSFORMER_1PH.dxf` | `9ee89223b747a6e5ef115d33e17c8f231b37b55f2af5c5794223d29f3ad0f6ba` |
| `avviamento-diretto.dxf` | `efeef9c563c4142f3bf9a2fedaa7d2cd6ecf80069703b04a6df2854bdab620b5` |
