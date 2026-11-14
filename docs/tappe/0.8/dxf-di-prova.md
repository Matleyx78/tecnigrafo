# DXF di prova della tappa 0.8

Disegni preparati dal responsabile in AutoCAD LT 2026 e in AutoCAD 2021, da
zero, senza dati di lavoro e senza riferimenti al suo nome o alla sua licenza,
come per la 0.7 ([0.7/dxf-di-prova.md](../0.7/dxf-di-prova.md)). Sono nel
repository [Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file)
(CC0), in DXF 2018, in millimetri (`$INSUNITS` 4, `$MEASUREMENT` 1), partendo
dal modello descritto in «Modello» (non dal modello di lavoro personalizzato).
Accanto a ogni disegno lo script eseguito (`.scr`) e la **cronologia dei
comandi** (`.txt`), copiata da `F2` o dal registro dei comandi (`LOGFILEON`):
richieste e messaggi dei comandi di Tecnigrafo si prendono da lì. Le cronologie
sono in italiano, come il programma del responsabile.

Chi scrive i riferimenti legge il risultato di AutoCAD con l'oracolo ezdxf
(ADR 0020, esteso alle entità della tappa da PT3), anche per i campioni
dell'elettrico, che non vengono da AutoCAD; i valori attesi si copiano
dal log dell'oracolo, non si calcolano a mano e non si ricavano da Tecnigrafo.
Commit e hash SHA-256 dei file pubblicati sono in «File pubblicati».

| File | Che cosa contiene | Riferimenti |
|---|---|---|
| `autocad-solid.dxf` | `SOLID` da 3 e da 4 vertici, uno «a farfalla», uno su un layer con colore, uno specchiato | RT10 |
| `autocad-ellissi.dxf` | ellissi fatte con `ELLIPSE` in tutti i modi (asse e distanza, *Center*, *Rotation*), archi d'ellisse con angoli e con *Included angle*, un'ellisse ruotata di 30°, una specchiata, una scalata `(2, 1)` dentro un blocco ed esplosa | RT48, RT49, RT50 |
| `autocad-stili-testo.dxf` | stili `Standard`, uno con altezza fissa 5, uno con fattore di larghezza 0,8 e obliquo 15°; un `TEXT` per stile | RT11, RT12 |
| `autocad-testo-multiriga.dxf` | `MTEXT` con larghezza 0 e 60, i nove punti di allineamento, due paragrafi (`\P`), una parola più larga della colonna, interlinea 1,5, rotazione 30°; uno con formati (`\f`, `\H`, `\C`, `\S1/2;`) | RT13, RT14, RT15 |
| `autocad-blocchi.dxf` | un blocco semplice; uno con entità sul layer `0` e colore *DaBlocco*; uno annidato in un altro; inserimenti con scala 1, 2, −1 (specchiato), 2 e 1 (non uniforme) e rotazione 30°; un blocco con `ATTDEF` variabile, invisibile, costante e preimpostato, inserito con i valori predefiniti (`ATTREQ` 0); un inserimento specchiato con `MIRROR` e `MIRRTEXT` 0; un blocco ridefinito dopo l'inserimento; un blocco non esplodibile | RT16, RT17, RT18, RT20, RT21 |
| `autocad-esplodi.dxf` | copie degli inserimenti di `autocad-blocchi.dxf` esplose, compreso il blocco con attributi; un inserimento con scala `(2, 1)` di un blocco con un cerchio, un arco e un testo, esploso (dà ellissi); una quota esplosa; gli `MTEXT` a, b e d di `autocad-testo-multiriga.dxf`, esplosi | RT29 |
| `simbolo-contatto.dxf`, `simbolo-con-tipo-di-linea.dxf` | due «simboli» da inserire come file: `$INSBASE` diverso dall'origine, un `ATTDEF` costante e uno variabile, un layer e un tipo di linea che il disegno di destinazione ha con proprietà diverse | RT19, RT21 |
| `autocad-inserisci-file.dxf` | il disegno in cui i due simboli sono stati inseriti con `-INSERT`: uno con un nome nuovo; l'altro, il cui nome è già un blocco del disegno, una volta con il solo file e una volta con `nome=file` | RT19, RT21 |
| `autocad-quote.dxf` | con `ISO-25`: lineari orizzontale, verticale e ruotata, allineata, angolare fra due linee, radiale e diametrale su un cerchio e su un arco, le quote **strette** dei quattro casi dell'adattamento (testo e frecce dentro, solo il testo dentro, solo le frecce dentro, tutto fuori) sia lineari sia allineate, e radiali e diametrali in un cerchio piccolo; con uno stile `PROVA` (`DIMTAD` 0, `DIMDEC` 3, `DIMDSEP` `.`, `DIMTXT` 3,5); una con testo sostituito `<> mm`; una quota ruotata, una scalata e una specchiata dopo la creazione; una quota coordinata (tipo che Tecnigrafo conserva soltanto) | RT22, RT23, RT24, RT25, RT26, RT27, RT28 |
| `autocad-retini.dxf` | `ANSI31`, `NET` e `SOLID` su un rettangolo di quattro linee con un cerchio dentro (punto interno) e con isole annidate; un retino da *Select objects*; un motivo ruotato di 30° e scalato 2; gli stili *Normal*, *Outer* e *Ignore*; un retino con un motivo diverso (`ANSI37`) | RT30, RT31, RT42, RT43, RT44 |
| `autocad-pedit.dxf` | linee e archi convertiti in polilinee; *Join* con distanza 0; *Multiple* con distanza 2 e *Extend*, *Add* e *Both*, su linee che non si toccano, su linee che si sovrappongono e su un arco seguito da una linea; *Close*, *Open*, *Reverse*, *Ltype gen* | RT32, RT33 |
| `campioni-elettrico/` | otto simboli della libreria dell'elettrico (`TG_EL_CONTACT_NO`, `TG_EL_COIL_DELAY_OFF`, `TG_EL_BREAKER_3P`, `TG_EL_TRANSFORMER_1PH`, `TG_EL_MOTOR_3PH`, `TG_EL_TERMINAL`, `TG_EL_FOLIO_OUT`, `TG_EL_PUSHBUTTON_NO`) e lo schema `avviamento-diretto.dxf`, scritti dallo script della libreria (DXF R2000), non da AutoCAD; campioni di prova, non la libreria (decisione del 2026-11-11, punto 8); commit `764dfdb9420623d26636e1552769cc223e484231`, hash qui sotto | RT18, RT19, RT21 |

## Modello

AutoCAD LT non ha `acadiso.dwt` (il suo è `acadltiso.dwt`, e quello del
responsabile è personalizzato). Tutti i disegni partono dal modello
`acadlt-Tecnigrafo.dwt`, fatto così:

1. un disegno **senza modello, metrico**: `MEASUREINIT` 1, `FILEDIA` 0, `NEW` →
   `.` (nessun modello), `FILEDIA` 1;
2. lo script `modello-tecnigrafo.scr` (unità, sistema globale, proprietà
   correnti, stile di testo `Standard` 2,5, stile di quota `ISO-25` corrente);
3. salvato come modello (`.dwt`).

Un disegno metrico senza modello ha solo lo stile di quota `ISO-25`, senza
`Standard`. Gli script impostano in testa anche le variabili che AutoCAD tiene
nel profilo e non nel disegno (`OSMODE`, `DYNMODE`, `OSNAPCOORD`, `ATTREQ`…).

`simbolo-con-tipo-di-linea.dxf` e `autocad-inserisci-file.dxf` caricano il
tipo di linea `TRATTO` da due file nostri, `tratto-simbolo.lin` (5, −2) e
`tratto-destinazione.lin` (10, −5): stesso nome, definizioni diverse, senza
dipendere dai nomi tradotti dei file `.lin` di AutoCAD.

## File pubblicati

Commit `1ad04b0b8a568fb9dd4960719a8f17289129f788` di
[Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file/tree/1ad04b0b8a568fb9dd4960719a8f17289129f788),
fine riga CRLF. I DXF sono ripuliti come gli altri del repository
(`$LASTSAVEDBY` vuoto, GUID fittizi, stampante `none_device`, codice di
prodotto vuoto, percorsi ridotti al nome del file).

| File | Programma | SHA-256 |
|---|---|---|
| `autocad-solid.dxf` | AutoCAD 2021 | `44779ab861c04e97cd82c5c67482d3e1273ae943e4c773c1edc69fc8d33552cf` |
| `autocad-solid.scr` | AutoCAD 2021 | `79973ec6f3aff2e5f70638999553ff307ae426bf0a7cdc3dc1fd44c093ad988f` |
| `autocad-solid.txt` | AutoCAD 2021 | `319badc722191892709d5d23a04ef76193f3d2947e42f2f8b02e8c6896849fa2` |
| `autocad-ellissi.dxf` | AutoCAD 2021 | `eacd35327f5a5c9833a6de0f304428e058d50385363673ee3107b647fd38ad6e` |
| `autocad-ellissi.scr` | AutoCAD 2021 | `6165d11e70b9c24ce487d877b3bdb31dfde1d505e9b824d5400ef1dbce69f5fe` |
| `autocad-ellissi.txt` | AutoCAD 2021 | `d5f87cebfad93305880d93a45f6d60801c07cd6f6368484f6b388aaee7e50dd0` |
| `autocad-stili-testo.dxf` | AutoCAD 2021 | `bfefbb6223919d7b9c65a513282aeed1f65eae93c0d217aef290d48f2ded6066` |
| `autocad-stili-testo.scr` | AutoCAD 2021 | `0f383a34685984eb6f29ce8727bb31bdcf4470a912e1c5dc553610a7fa9b6bc9` |
| `autocad-stili-testo.txt` | AutoCAD 2021 | `4cb1b438121e500c626d869da8e9f22a300b1f46d2cdc632a5e6f0eee8a4bde6` |
| `autocad-testo-multiriga.dxf` | AutoCAD 2021 | `9cecfc2bb45052792d16b6ec2b7af3240d9c7be920c540454f0bce33499e1bcd` |
| `autocad-testo-multiriga.scr` | AutoCAD 2021 | `df8219703540be5e66714576ceb4c834e15675acaf062765995d582f71865646` |
| `autocad-testo-multiriga.txt` | AutoCAD 2021 | `b54ed714b9edd5516912ed8b07358921879ccafa9471721144e54af9a3fc26d7` |
| `autocad-blocchi.dxf` | AutoCAD 2021 | `347efd4ee75e05d33ba7c4dc0c2257a8d658e6fa825ec265a87c746a0af17f64` |
| `autocad-blocchi.scr` | AutoCAD 2021 | `88a8515a3e7959b431fa2e3b41560a8207d50ff5d5dff75989bbffd0d0dfffbb` |
| `autocad-blocchi.txt` | AutoCAD 2021 | `67d3c850f15ddb91762e0670f83c34043eb26581a35ef1a0cbb94f97a0f5cabb` |
| `autocad-esplodi.dxf` | AutoCAD 2021 | `7a0b712e05b656477176ded4cb79a58033bf7b9c396172a9a67921a94949a11f` |
| `autocad-esplodi.scr` | AutoCAD 2021 | `b632dae77ae2f9b1542889c7f4f6578bde240e79feba48c19d4d12422762c192` |
| `autocad-esplodi.txt` | AutoCAD 2021 | `78a3b9e4a7692785c573bcd0aa33632960b251a1a8cb7fc4e31b856f8da59b35` |
| `simbolo-contatto.dxf` | AutoCAD 2021 | `f8dd190a0d9b129717f95410c8311b07e81c60a1cdfee1d1e2a40fe6a13b29cc` |
| `simbolo-contatto.scr` | AutoCAD 2021 | `6572d66e588e489387cb80cd61bccb98f2eb5f3b71f985a7fc7671cc144ef88d` |
| `simbolo-contatto.txt` | AutoCAD 2021 | `90d99182c8b49d360b0caf09abad71459fbd011c14b94ab0b6b6f86ff112e5d4` |
| `simbolo-con-tipo-di-linea.dxf` | AutoCAD 2021 | `602c9610ad025cd4675a4c65608d1405664b25c2a1ae4d8ce84625791b678d26` |
| `simbolo-con-tipo-di-linea.scr` | AutoCAD 2021 | `38611f018a03418c23346d5540d43f7060ff00e3cab66c4179f8891771e6b676` |
| `simbolo-con-tipo-di-linea.txt` | AutoCAD 2021 | `7ab2d1447ec2afbdadcf23b221041333a3970e794f3f4a431dfa8f2be0b61ccc` |
| `autocad-inserisci-file.dxf` | AutoCAD 2021 | `7876b8a2bdabf0f782986bc075e27ce842fcdee2b034336ec698fa759b9bfdd2` |
| `autocad-inserisci-file.scr` | AutoCAD 2021 | `0b3eedcfe6fa71f42a0c23eafb1686cbedc974b826f38ecce7bf54ec23f46e64` |
| `autocad-inserisci-file.txt` | AutoCAD 2021 | `969bf443af2f833c1c23f77bd48811abe05a644c84911c45063590bfbe89d92e` |
| `autocad-quote.dxf` | AutoCAD LT 2026 | `63a0cf5a870a2518e24d4afded207680da20d04c28a7299f5d15d100875f7ec1` |
| `autocad-quote.scr` | AutoCAD LT 2026 | `fb5933d007618e7f085cea5f5bbde93a2ba913e93ecb12a5cbb27c9ed5a3d106` |
| `autocad-quote.txt` | AutoCAD LT 2026 | `3a34f4e798be80489f9e1ae811de974ce730a7d71e173b0148817ff850157483` |
| `autocad-retini.dxf` | AutoCAD LT 2026 | `53003a6acb7fee57be087cc0a1e7a9733e95181ff557c874752f06e57373e926` |
| `autocad-retini.scr` | AutoCAD LT 2026 | `0fa9bd76d108ebc7887904911e8a1896888c4ffe9668951f872aab5d1dc3df48` |
| `autocad-retini.txt` | AutoCAD LT 2026 | `bdf812076096712179a80b99343ec7d1bed793c9a4928338317eaa8230b17550` |
| `autocad-pedit.dxf` | AutoCAD LT 2026 | `396f68a56cf58e5361aedea5cb594ff08acf2ac2ced0a1125dde0a4482cd33be` |
| `autocad-pedit.scr` | AutoCAD LT 2026 | `a730c6379d9134caf46fd20f60efa453b27ed174496b743d909aa680b3c4774a` |
| `autocad-pedit.txt` | AutoCAD LT 2026 | `d998919262b229582eb06bc265fd2c6909058ad76f96e0749db42c8ec0df88fa` |
| `modello-tecnigrafo.scr` | — | `8f9acad473a1b283e54acfe680ff3e1aaa4a1b888d2210cedeae8d617a99bbab` |
| `tratto-simbolo.lin` | — | `0e513a06dc39bc1644d421e75cb2bbfcf346b44742fe1da08c39f9ee936a7cb9` |
| `tratto-destinazione.lin` | — | `8bfb737c139e2a464462c31754df7bc4ca062c5be5148be8eac98097a52dc7de` |

## Prove a mano

1. **Stili di quota.** `-DIMSTYLE` → *STatus* in un disegno nuovo dalla copia di
   fabbrica di `acadltiso.dwt` (AutoCAD LT 2026). `ISO-25` ha i valori di
   `ISO-25` del modello qui sopra, salvo `DIMJOGANG`: **45** nel modello di
   fabbrica, 90 nel disegno senza modello (e quindi nei disegni di prova, che
   non hanno quote di raggio ridotte). Il disegno nuovo di Tecnigrafo prende 45
   (ADR 0047, punto 2). `Standard` ha i valori in pollici anche nel modello
   metrico: `DIMASZ` 0,18, `DIMCEN` 0,09, `DIMDEC` 4, `DIMDLI` 0,38, `DIMDSEP`
   `.`, `DIMEXE` 0,18, `DIMEXO` 0,0625, `DIMGAP` 0,09, `DIMTAD` 0, `DIMTDEC` 4,
   `DIMTIH` On, `DIMTOH` On, `DIMTOFL` Off, `DIMTOLJ` 1, `DIMTXT` 0,18,
   `DIMTZIN` 0, `DIMZIN` 0, `DIMALTD` 2, `DIMALTF` 25,4, `DIMALTTD` 2,
   `DIMJOGANG` 45; le altre come `ISO-25`.
2. **Tracciamento.** In `LINE`, con due punti acquisiti, il clic sul primo
   punto toglie i segni dei punti acquisiti e le loro guide, come dice l'ADR
   0049, punto 2. Le guide si vedono solo con il puntatore vicino alla loro
   linea; i segni restano finché la richiesta non riceve un punto.

## Che cosa si è visto nei disegni

Da sapere per chi scrive i riferimenti (i valori si leggono comunque con
l'oracolo):

- **`SOLID`** specchiato: vertici riflessi nello stesso ordine, estrusione
  (0,0,1). Con l'Invio al quarto punto `SOLID` crea il triangolo e chiede di
  nuovo il terzo punto.
- **`ELLIPSE`**: specchiata con `MIRROR`, estrusione (0,0,−1); un'ellisse intera
  esplosa da un inserimento con scala `(2, 1)` ha parametri da 0,137 a 6,420
  (differiscono di 2π ma non partono da 0).
- **Stili di testo**: l'obliquo (gruppo 50) è in **gradi**; all'indietro e
  capovolto danno il gruppo 71 = 6 nello stile e nel `TEXT`; un `TEXT` con lo
  stile `Standard` non scrive il gruppo 7.
- **`MTEXT`**: la rotazione è nella direzione 11/21, non nel gruppo 50; AutoCAD
  non scrive i gruppi 42 e 43. Dove va a capo si vede in `autocad-esplodi.dxf`,
  dove gli `MTEXT` a, b e d sono esplosi (un `TEXT` per riga). L'interlinea
  resta per i `-MTEXT` successivi (decisione del 2026-11-13, PT15).
- **Blocchi**: l'`ATTDEF` costante non diventa `ATTRIB`; esplodendo un
  inserimento gli attributi tornano `ATTDEF`, costante compresa; il blocco non
  esplodibile ha il gruppo 280 = 0 nel `BLOCK_RECORD` e `EXPLODE` risponde
  «Impossibile esplodere 1.». `EXPLODE` da script prende un solo oggetto
  («Selezionare oggetto», senza Invio). Una quota esplosa dà `LINE`, due
  `SOLID` per le frecce e un `MTEXT`.
- **Inserimento di file**: a parità di nome vincono layer e tipo di linea del
  disegno; il comportamento con un blocco che esiste e il punto base sono
  nella decisione del 2026-11-13 (PT19, PT21).
- **Quote**: con i punti scritti, `Creata quota non associativa.`; le radiali e
  diametrali con la posizione data hanno il bit 128 nel gruppo 70; la quota
  coordinata Y di un punto in y = −250 misura 250; nello stile `PROVA`
  `DIMTAD` 0 e `DIMDSEP` `.` non sono scritti, perché sono i valori
  predefiniti del gruppo.
- **Retini**: con gli stili *Esterno* e *Ignora* AutoCAD scrive comunque tutti
  i contorni (lo stile decide che cosa si riempie); l'angolo del motivo
  (gruppo 52) è in **gradi**.
- **`PEDIT`** *Multiple*, distanza 2, segmenti aggiunti: *Extend* 1 (linee che
  non si toccano), 0 (linee sovrapposte: non le unisce), 1 (arco e linea);
  *Add* 2, 2, 2; *Both* 1, 2, 1. L'arco convertito parte dall'estremo da cui
  AutoCAD lo percorre in senso antiorario.

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
