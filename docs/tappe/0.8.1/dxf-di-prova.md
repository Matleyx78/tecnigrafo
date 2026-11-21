# DXF di prova della tappa 0.8.1

Disegni preparati dal responsabile in AutoCAD 2021 (completo, in italiano), da
zero, senza dati di lavoro e senza riferimenti al suo nome o alla sua licenza,
con le stesse regole della 0.8 ([0.8/dxf-di-prova.md](../0.8/dxf-di-prova.md)).
Sono l'elenco di lavoro della
[#920](https://github.com/Matleyx78/tecnigrafo/issues/920) per la scheda
[0.8.1](../0.8.1-completamenti.md) (sezione «Da preparare prima
dell'apertura»). Stanno nel repository
[Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file) (CC0), in
DXF 2018, in millimetri (`$INSUNITS` 4, `$MEASUREMENT` 1), partendo dal modello
`acadlt-Tecnigrafo.dwt` della 0.8 (vedi «Modello» in
[0.8/dxf-di-prova.md](../0.8/dxf-di-prova.md)). Accanto a ogni disegno lo
script eseguito (`.scr`) e la **cronologia dei comandi** (`.txt`, dal registro
`LOGFILEON` o da `F2`), in italiano: richieste e messaggi dei comandi di
Tecnigrafo si prendono da lì.

Chi scrive i riferimenti legge il risultato di AutoCAD con l'oracolo ezdxf
(ADR 0020); i valori attesi si copiano dal log dell'oracolo, non si calcolano a
mano e non si ricavano da Tecnigrafo. I casi di riferimento li assegna il piano
della tappa. Commit e hash SHA-256 dei file pubblicati sono in «File
pubblicati»; le prove fatte a mano sono in «Seduta in AutoCAD».

| File | Che cosa contiene |
|---|---|
| `autocad-mtext-formati.dxf` | un `MTEXT` per formato, con larghezza 0: grassetto, corsivo, grassetto corsivo (`\f…\|b1\|i1`), `\L`, `\O`, `\K`, `\H2x;`, `\H3.5;`, `\C1;`, un colore vero (`\c16744448;`), `\W0.8;`, `\Q15;`, `\T1.5;`, `\S1/2;`, `\S1#2;`, `\S1^2;`, un `\f` con un font che non c'è e uno con Arial; un paragrafo largo 60 con più formati e una frazione, che va a capo; una quota lineare con testo sostitutivo formattato (`{\C1;<>} {\fArial\|b1…;mm}`). Ogni `MTEXT` ha una copia a x = 150 esplosa con `EXPLODE`: i `TEXT` dicono dove AutoCAD mette ogni pezzo e dove va a capo il paragrafo. Stile di testo `ARIAL` (`arial.ttf`), che ha le misure di Liberation Sans |
| `autocad-attributi.dxf` | blocco `COMPONENTE` con sei `ATTDEF`, uno per modo (nessuno, *Invisible*, *Constant*, *Verify* con lo stile `ARIAL`, *Preset*, *Lock position* giustificato `MC` e ruotato di 10°), creati con `AFLAGS` 0 e il modo acceso dalla domanda di `-ATTDEF`; tre `-INSERT` con `ATTREQ` 1, due con i valori scritti e uno con l'Invio a ogni domanda; `-ATTEDIT` uno per uno (valore sostituito, posizione, altezza, angolo, stile, layer, colore su `CODICE`; valore cambiato in parte su `SIGLA`) e globale (su tutti, anche gli invisibili; sui soli visibili con selezione e filtri `*`, `CODICE`, `K*`) |
| `autocad-attdisp-on.dxf`, `autocad-attdisp-off.dxf` | lo stesso blocco `VISTA` (un attributo normale e uno invisibile) inserito una volta, con `ATTDISP` *ON* e *OFF*: `$ATTMODE` 2 e 0; i due file differiscono solo per `$ATTMODE` (oltre a date e GUID) |
| `autocad-attsync.dxf` | `SINCRO_A` e `SINCRO_B`, uguali, con `PRIMO`, `SECONDO`, `TERZO`; `SINCRO_A` inserito due volte con valori diversi, `SINCRO_B` una volta; tutti e due ridefiniti con `-BLOCK` (un cerchio in più, `SECONDO` tolto, `TERZO` spostato e alto 3,5, `QUARTO` aggiunto); `ATTSYNC` del solo `SINCRO_A`. `ATTSYNC` aggiorna tutti gli inserimenti del blocco scelto: il «prima» è `SINCRO_B` |
| `autocad-battman.dxf` | blocco `BATT` con `UNO`, `DUE`, `TRE` (nella definizione `TRE`, `DUE`, `UNO`), inserito (I1); in `BATTMAN`, a mano, ordine `UNO`, `DUE`, `TRE` e *Sincronizza*; I2 e I3 inseriti con le domande nel nuovo ordine; poi, a mano, messaggio di `DUE` cambiato in «Due cambiato» e *Sincronizza*. I passaggi a mano sono nel `.scr` come commenti e nel `.txt` come righe «[a mano nella finestra]» |
| `autocad-hatchedit.dxf` | sette rettangoli di quattro linee con un cerchio dentro, retinati uguali (`ANSI31`, scala 1, angolo 0, *Normal*), ognuno con una sola modifica di `-HATCHEDIT`: motivo `ANSI37`; scala 2; angolo 30; origine nuova 245,5 non memorizzata; stile *Outer*; tutto insieme (`ANSI37`, 2, 30, origine 405,5, *Ignore*); nessuna modifica |
| `autocad-griglia-spenta.dxf`, `-passi`, `-snapbase`, `-display0`, `-display2`, `-display6`, `-major` | una linea e la `VPORT` `*Active` con uno stato di partenza uguale per tutti (griglia accesa, passi 10, snap spento, `SNAPBASE` 0,0, `SNAPANG` 0, `SNAPSTYL` 0, `SNAPTYPE` 0, `GRIDDISPLAY` 2, `GRIDMAJOR` 5, `UCSICON` 3) e una sola variante: griglia e snap spenti; `GRIDUNIT` 5,10 e `SNAPUNIT` 2.5,5 con lo snap acceso; `SNAPBASE` 3,4; `GRIDDISPLAY` 0, 2, 6; `GRIDMAJOR` 10 |
| `autocad-ucsicon-off.dxf`, `-noorigin`, `-origin` | come sopra, con `UCSICON` 0, 1 e 3 |
| `autocad-snap-ang30.dxf`, `autocad-snap-iso.dxf`, `autocad-snap-polare.dxf` | come sopra, per la conservazione: `SNAPANG` 30, `SNAPSTYL` 1, `SNAPTYPE` 1 |
| `autocad-rectang.dxf` | dieci `RECTANG` 40×20: semplice, *Chamfer* 3 e 5, *Fillet* 4, *Width* 1, *Area* 600 con lunghezza 30, *Dimensions*, *Elevation* 5, *Thickness* 2, uno dato dal vertice in alto a destra, *Rotation* 30 con *Dimensions*; ogni caso rimette a 0 le opzioni che `RECTANG` ricorda |
| `autocad-polygon.dxf` | `POLYGON` da 3, 6, 1024 e 5 lati *Inscribed* con il raggio scritto; 6 lati *Circumscribed*; 6 lati con il raggio dato da un punto (`@20<15`), *Inscribed* e *Circumscribed*; *Edge* in un verso e nell'altro |
| `autocad-selezione.dxf` | `ERASE` *Last* due volte; `SELECT` a finestra e `MOVE` *Previous*; `MOVE`, `U` e `SELECT` *Previous*; `LINE`, `U` e `SELECT` *Last*; per ultimo `ERASE` e `SELECT` *Previous*. Resta una linea; i conteggi sono nella cronologia |
| `autocad-vicino.dxf` | una linea, un arco, un cerchio, una polilinea con un lato ad arco, un'ellisse e un `MTEXT`; per ciascuno una `LINE` che parte con `_nea` da un punto scritto 0,3 mm fuori dall'oggetto. Sul `MTEXT` AutoCAD non trova niente: lo script si ferma lì e la cronologia finisce con l'Esc |

## Come sono fatti gli script

- In testa le variabili del profilo e correnti, come nella 0.8, più `ATTMODE`;
  `ATTREQ` vale 1 solo nei file degli attributi che lo chiedono.
- Le finestre di selezione vogliono gli oggetti a schermo: dove servono c'è uno
  `ZOOM` *Finestra* prima.
- La domanda «Selezionare attributi» di `-ATTEDIT` prende una sola selezione e
  si chiude da sola, senza Invio (come `EXPLODE`).
- `autocad-hatchedit.scr` sceglie il retino con `(entlast)` di AutoLISP: va in
  AutoCAD completo, non in LT; la cronologia mostra l'indirizzo di sessione
  (`<Nome entità: …>`).
- `autocad-vicino.scr` fissa `APERTURE` 10 e uno zoom, perché la casella dello
  snap è in pixel.
- In `-INSERT` le domande degli attributi arrivano nell'ordine della
  definizione, che con `-BLOCK` e `_ALL` è l'**inverso** della creazione; gli
  script scrivono i valori in quell'ordine.
- Le cronologie dal registro sono ripulite: niente intestazione né percorsi con
  il nome dell'utente, niente righe vuote, lettere accentate ripristinate (il
  registro di AutoCAD è in codifica Windows), UTF-8 con fine riga CRLF.

## File pubblicati

Commit `2aafbb1fa17abc67fdf2ff7de26a34b70e79b736` di
[Matleyx78/dxf-test-file](https://github.com/Matleyx78/dxf-test-file/tree/2aafbb1fa17abc67fdf2ff7de26a34b70e79b736),
fine riga CRLF. I DXF sono ripuliti come gli altri del repository
(`$LASTSAVEDBY` vuoto, GUID sostituiti, stampante `none_device`, codice di
prodotto vuoto).

| File | Programma | SHA-256 |
|---|---|---|
| `autocad-mtext-formati.dxf` | AutoCAD 2021 | `5b69493f63208ac85f9c3c97557e1aa537fe61bedc30f4e40047784fde85ea4b` |
| `autocad-mtext-formati.scr` | AutoCAD 2021 | `94328eecc5c99d3ad3211931bf782ed490bfdf3fb5ddbc76e81336698c66e72f` |
| `autocad-mtext-formati.txt` | AutoCAD 2021 | `2d3ecf287dc780f181cf0fa4dac047fabe81d76c1c07344f6b955ad12fb215a3` |
| `autocad-attributi.dxf` | AutoCAD 2021 | `180f81f8c9af3de95eaa13ca9f938ec5eca0a23bb95fcadc78cbf85ee612685e` |
| `autocad-attributi.scr` | AutoCAD 2021 | `14473306dfbf2a629c61bc64651d0d285589c44708cf71517753543dee40b3a8` |
| `autocad-attributi.txt` | AutoCAD 2021 | `a6e4c3d3c4c1637f398be5efca319feacdd487b88631c7c1c77b62604538dfe8` |
| `autocad-attdisp-on.dxf` | AutoCAD 2021 | `e6117fe73e8439034ab9336933ac28012d3e4ba8fe65841dcd8f76fd5060128a` |
| `autocad-attdisp-on.scr` | AutoCAD 2021 | `53e2c4a669931f62e9d9e17e7b2853c23cf07e0ac5fdd13452b428f174cfb971` |
| `autocad-attdisp-on.txt` | AutoCAD 2021 | `808c9a8baabb2a4bd47b9f76f241ec27b3fdd04177526bd87774874eac04b555` |
| `autocad-attdisp-off.dxf` | AutoCAD 2021 | `19cb4dee9f7b7d14f9ddd556722aa6de0b8d2a558c66e7fc123c0347c7c1a00f` |
| `autocad-attdisp-off.scr` | AutoCAD 2021 | `5e4efe6088c7cee81180dff8299ce07506f72931cf6e760a8398db367ca1aafa` |
| `autocad-attdisp-off.txt` | AutoCAD 2021 | `caa220409c723a14bf7cd0f3eec530edc79019f2839c7d0826b99f402a0b1a9d` |
| `autocad-attsync.dxf` | AutoCAD 2021 | `eabf40ba218053553f0e561f65ebf9b1f89cce336514ab35071e102d51231715` |
| `autocad-attsync.scr` | AutoCAD 2021 | `24e2bee9411097db9c017e5a374770c375f0c140b5cef446709a31a23581acce` |
| `autocad-attsync.txt` | AutoCAD 2021 | `4b3fc807d81da3ede7fd66df1338a9f4531289df54d72ae6f2fb0c108e48612a` |
| `autocad-battman.dxf` | AutoCAD 2021 | `65ba51f9fad7543300ed7e767e15fce7a60ce3d12c7a2b64bdfa0d3f69ee858c` |
| `autocad-battman.scr` | AutoCAD 2021 | `298b5efe0f92b9b41daa6cdbf36cc6cd1e0242d7cdf536596580b6f5ff2d7fc2` |
| `autocad-battman.txt` | AutoCAD 2021 | `aeff15dc8935ec2065e6749aeeeac9d0b14040a219cf1d5b2e354fbbcb90347a` |
| `autocad-hatchedit.dxf` | AutoCAD 2021 | `5b11e0d6eb2c863deff2c529f33c5ff7c3d8b932dc9f54719dfb2758221017eb` |
| `autocad-hatchedit.scr` | AutoCAD 2021 | `c005bd6afd5fdb668d2099dea589bfc3ac9b7b756bcab6fa91690f5d632b7f85` |
| `autocad-hatchedit.txt` | AutoCAD 2021 | `d926350ac65e37edd9836dce6d4829aac8155c1312101c54bcf96119d3916867` |
| `autocad-griglia-spenta.dxf` | AutoCAD 2021 | `646153d8b2a29dfbda511b52cc2dcd9617c6ddf3b2e5aebd4490d9c6731b9258` |
| `autocad-griglia-spenta.scr` | AutoCAD 2021 | `93dbe17684e67427e6608fecf3b8469a5d27ece80ad036e573e98eb0ae350919` |
| `autocad-griglia-spenta.txt` | AutoCAD 2021 | `77ac99b8ee802f2e3ad7ac501fe92286267fc580d6243a391c1077a5a4716265` |
| `autocad-griglia-passi.dxf` | AutoCAD 2021 | `09ed440e60e775b7a39f767149f6fb518336224ac470bc2175ac8fa7b7b9ef83` |
| `autocad-griglia-passi.scr` | AutoCAD 2021 | `c286026ef460adcb779b0c2d0daaac3a8bb4853e97c3460a40fdcaac004e694b` |
| `autocad-griglia-passi.txt` | AutoCAD 2021 | `b70928aa27bd51bb1d7b9c0071204ce1678f0caf0b5c292683d7ab4d94f99071` |
| `autocad-griglia-snapbase.dxf` | AutoCAD 2021 | `4d7c6412d81ccbd0f77a6c7564d662c878280e3fa097698793162c41b8bc0ff1` |
| `autocad-griglia-snapbase.scr` | AutoCAD 2021 | `2c46891f19244d69b43ddc9299763b105432c9cbf1ad56a3e3788f7f7030d592` |
| `autocad-griglia-snapbase.txt` | AutoCAD 2021 | `a2c30afbd8affe8cf603e9fa0b7fd3986cdf4cb288e5a47ef3b6583606ba7527` |
| `autocad-griglia-display0.dxf` | AutoCAD 2021 | `8ce775da654bf848e45fbb1e2efddcfc133a3f2805c7fb37dc8aa47338e83e25` |
| `autocad-griglia-display0.scr` | AutoCAD 2021 | `cccbddccc40932621d7637f4a927e563a2c94ad37267f6a7c7f2cb8392bf8268` |
| `autocad-griglia-display0.txt` | AutoCAD 2021 | `98113ba5872fa3b16d64a573c6420693fb220deab2905c516c1d18de08dec726` |
| `autocad-griglia-display2.dxf` | AutoCAD 2021 | `8fc9f41ea8cbc95d8efc55e06716ba8a817b487badf5b32f3f4d168a376a9947` |
| `autocad-griglia-display2.scr` | AutoCAD 2021 | `384d123e466d44231f67e6e73dbbad62e54df61a509efde4f4eda46d75a6cd3a` |
| `autocad-griglia-display2.txt` | AutoCAD 2021 | `a420de5d40ffae6f46033884331eed8c48d312ab5ed23903fb8592052592123d` |
| `autocad-griglia-display6.dxf` | AutoCAD 2021 | `6a8cfe117181fdce32b39770e3baf925f65f61d2d7fbc5644f8a2902516f5517` |
| `autocad-griglia-display6.scr` | AutoCAD 2021 | `de9f0fc8a6e43c08a76af71f85bfae216dac65638e202b7d9750868629834e29` |
| `autocad-griglia-display6.txt` | AutoCAD 2021 | `545d29ea8968b6ea765f5075e21a5b9229f273d1df462603f8f52afccdd37f23` |
| `autocad-griglia-major.dxf` | AutoCAD 2021 | `f200845a62a07572256972b0c423f147363cfd35d92523246ab7b2d04e3ea8ff` |
| `autocad-griglia-major.scr` | AutoCAD 2021 | `8de3d713fddcbcd33a10c4d3a830d9d1f851cee3a0bca6e4f36e1be85501bfb8` |
| `autocad-griglia-major.txt` | AutoCAD 2021 | `d3ced5537bc633e8ae519f1796d566ab3263a6a7a9e054941c06b9134109ac5e` |
| `autocad-ucsicon-off.dxf` | AutoCAD 2021 | `97e247408935a3415477f0293c7d905c5f901d46e165ad80e8503e76786b04a2` |
| `autocad-ucsicon-off.scr` | AutoCAD 2021 | `ca6fa05081810e388d389467129d7db278232c73121901d9ee9545735f629945` |
| `autocad-ucsicon-off.txt` | AutoCAD 2021 | `6a3416b7be16480c9a6751a3b19d348655cec8f03d1b91a59ae6cd4d4ac8a1cd` |
| `autocad-ucsicon-noorigin.dxf` | AutoCAD 2021 | `76eb93b937d535ee0c022f823a299915a0390c5ee7a680f07f8e64e296145fe1` |
| `autocad-ucsicon-noorigin.scr` | AutoCAD 2021 | `db40def427db3f50b65d16fb342a17a2b0fcee7148e370bcc1ddeb7e0d625deb` |
| `autocad-ucsicon-noorigin.txt` | AutoCAD 2021 | `abae0121ca739fec4812e5e89f04d8e18837ff0e053039181ee248f3bed49922` |
| `autocad-ucsicon-origin.dxf` | AutoCAD 2021 | `d550f231bc5f11df0df2632492a4f4777777a8d710a1054f69bedf5a468ea611` |
| `autocad-ucsicon-origin.scr` | AutoCAD 2021 | `7aa1695f47d84ca0106c3fde15ec7824bd73b88e184f0750391ce260d4e723c4` |
| `autocad-ucsicon-origin.txt` | AutoCAD 2021 | `7909029743d800ba70962ff4c08120e9669a023226ee8d4f2ca6b7c329c1bc2b` |
| `autocad-snap-ang30.dxf` | AutoCAD 2021 | `b4998d098078f0e4d37f079c23f71d40b6f640c81e3b92476a037d8d3a5da8b3` |
| `autocad-snap-ang30.scr` | AutoCAD 2021 | `10c0deb391f04143696af3bc2ce1807b8d7ebc3de4b93d096052a069b36d1cb0` |
| `autocad-snap-ang30.txt` | AutoCAD 2021 | `0999f226be3f2ceb21665c3f2f5a0712aa961e264c5dc0a1b04df789d95b01b9` |
| `autocad-snap-iso.dxf` | AutoCAD 2021 | `158cc1a1a81e7f8aa27b0e8cf63f7823d666ca93cddff836fb067a3ea5cd07d3` |
| `autocad-snap-iso.scr` | AutoCAD 2021 | `35db9b01cfc428fb7a33de6de46993b65c0fa9cfee68e1ee27fbc726c5dfc6a6` |
| `autocad-snap-iso.txt` | AutoCAD 2021 | `cf0c48a27ff4cc66f7a40f3b7ac1007970af328f765d73273805644ecfc7cd92` |
| `autocad-snap-polare.dxf` | AutoCAD 2021 | `36cf05126e2197c50ca550d41255a6a82be592fb4235cd4a424f5fb697b69c4e` |
| `autocad-snap-polare.scr` | AutoCAD 2021 | `282777569043f7ea96fe309b22021bcf00ab669237db81c505676379f63b27d7` |
| `autocad-snap-polare.txt` | AutoCAD 2021 | `216abb225702faf274fc39fb7734314f12ab5b26b63fe339f470bcbf207dc10a` |
| `autocad-rectang.dxf` | AutoCAD 2021 | `1bd17e3fe42dc43cf321a4f7d19c8c94ecea9e2f57b656a57091a78d69d825d2` |
| `autocad-rectang.scr` | AutoCAD 2021 | `bdd8b738c0f939d97e0308b1c584a657e0b7932ba5822f0a450b4ec15cdb51b0` |
| `autocad-rectang.txt` | AutoCAD 2021 | `ee41e831182a243cbd7884541e6202edd87ebdcd5a84d79969e53a5d471b77dd` |
| `autocad-polygon.dxf` | AutoCAD 2021 | `2b4164ce86467651c547500e9f17aa1488d6b990d61fd4d3d8033df497828dfa` |
| `autocad-polygon.scr` | AutoCAD 2021 | `7e106cb76e07469095da5f48177bc8d6e0bfc48153ece14adac5abb78008a4bc` |
| `autocad-polygon.txt` | AutoCAD 2021 | `7c82a64ea6c75884e2f053f7720525a087582231f2cd1f9d3195c4d67f8a2a4e` |
| `autocad-selezione.dxf` | AutoCAD 2021 | `76466488c04a6c0cc28f51aa5f949b596f5aa21756dc1a13ac1e3a1db803528c` |
| `autocad-selezione.scr` | AutoCAD 2021 | `d413332830bf3dc398e46d49a89cf2a246d218f51be5ce17a61715f12a824c46` |
| `autocad-selezione.txt` | AutoCAD 2021 | `3baf4e76a895e50f4ad7dfee4f87c9612afd3c0dde77abe72aa166673c2d2f36` |
| `autocad-vicino.dxf` | AutoCAD 2021 | `4bbccf5daeabe99673eaf0ab4aaf7ed3245a65e78d121c1660d391d62a8ea8ee` |
| `autocad-vicino.scr` | AutoCAD 2021 | `1b5154f3af95b1294b0a4ccd53d441a1b8f3a55fcfca56d96d4e56642dd7c93a` |
| `autocad-vicino.txt` | AutoCAD 2021 | `688b736b4a8993a33a01d5a46d9b8711eb9458b013f4b876501d15c637e4ff67` |

## Seduta in AutoCAD

Risposte del responsabile nel suo
[commento alla #920](https://github.com/Matleyx78/tecnigrafo/issues/920#issuecomment-7067255469),
con le schermate dell'icona dell'UCS, fatte in AutoCAD 2021 completo. In
breve:

1. **`ATTREQ`** sta nel registro e gli script dei DXF di prova lo mettono a 0:
   il valore del profilo di oggi non dice quello di fabbrica. La guida di
   Autodesk, anche per LT, dà 1 come valore iniziale. LT non provato.
2. **Disegno nuovo da `acadiso.dwt`** (la copia di fabbrica in
   `UserDataCache`): `GRIDMODE` 1, `GRIDUNIT` 10,10, `SNAPMODE` 0, `SNAPUNIT`
   10,10, `SNAPBASE` 0,0, `SNAPANG` 0, `SNAPSTYL` 0, **`GRIDDISPLAY` 3**,
   `GRIDMAJOR` 5, `UCSICON` 3; `SNAPTYPE` 0 (profilo).
3. e 4. **Snap alla griglia con l'orto e con il polare: non conclusive.** Con
   l'orto (e con il polare a 0°) la linea da 0,0 è finita in 29.6482,0, non
   arrotondata alla griglia; ma lo snap non arrotondava nemmeno da solo
   (`ID` con il solo F9: 39.5309,12.2156), quindi in quella sessione non agiva
   sui clic. Il polare acceso spegne l'orto (si escludono).
5. **Centro contro *Vicino*** (`OSMODE` 516), puntatore sul bordo di un
   cerchio: compare la clessidra di *Vicino*.
6. ***Prolungamento***: primo punto di una `LINE` nuova, 10 oltre l'estremo
   30,−50 → 40,−50. ***Parallelo***: 20 lungo la parallela a una linea da
   0,−80 a 30,−70, partendo da 0,−60 → 18.9737,−53.6754.
7. **Icona dell'UCS** (`UCSICON` 3): nell'angolo in basso a sinistra quando
   l'origine è fuori vista o sul bordo (dopo `ZOOM` *Estensioni* l'icona non ci
   starebbe intera); sull'origine dopo `ZOOM` *Centrato* su 0,0.
8. **Alias**: le righe di `acad.pgp` sono state date in chat e non si
   pubblicano. L'AutoCAD del responsabile è in italiano, con nomi di comando e
   alias italiani, diversi dagli alias inglesi elencati nella scheda.

## Che cosa si è visto nei disegni

Da sapere per chi scrive il piano e i riferimenti (i valori si leggono comunque
con l'oracolo):

- **`MTEXT`**: `\S1^2;` AutoCAD lo salva come `\S1^ 2;` (con lo spazio). Il
  colore vero `\c` è **BGR**: `\c16744448;` (`FF8000`) si vede blu, e il
  `TEXT` esploso ha il gruppo 420 = 33023 (`0080FF` in RGB) e ACI 150.
  Esplodendo restano altezza, fattore di larghezza (41), obliquo (51) e colore;
  sottolineato, sopralineato e barrato diventano `%%U`, `%%O`, `%%K`;
  grassetto, corsivo e spaziatura si perdono (le posizioni ne tengono conto);
  il font che non c'è dà lo stile `MtXpl_FontInesistent`; un `\f` uguale al
  font dello stile si fonde in un `TEXT` solo. Prima riga di base 2,5 sotto
  l'angolo, 3,0 con il sopralineato, 5,0 con `\H2x;`, 2,976 con il font
  sostituito. Il paragrafo largo 60 va a capo in cinque righe.
- **Attributi**: *Lock position* è il **secondo gruppo 280** (= 1), non un bit
  del 70. Con la giustificazione `MC` il punto 11 è quello dato e il 10 si
  ricalcola dalla larghezza del valore. `-ATTDEF` mostra i modi correnti e li
  cambia a uno a uno; *Constant* chiede il valore e non la richiesta. Le
  richieste, i predefiniti e i valori accettano gli spazi. Con `ATTREQ` 1
  `-INSERT` chiede i valori (non *Constant* né *Preset*) e alla fine
  «Verificare i valori di attributo» per i soli *Verify*. `AFLAGS` è della
  sessione: 16 (*Lock position*) all'avvio di AutoCAD 2021, poi resta l'ultimo
  valore. L'`-ATTEDIT` globale sui non visibili prende anche gli invisibili e
  rigenera il disegno.
- **`ATTSYNC`** *Name* non chiede conferma e chiude con «SINCATT completato.»:
  chi ha la stessa etichetta tiene il valore e prende posizione e altezza dalla
  definizione, l'etichetta tolta sparisce, quella nuova arriva con il
  predefinito, l'ordine è quello della definizione. `-BLOCK` su un nome che
  esiste chiede «Il blocco "…" esiste già. Ridefinirlo? [Sì/No] <N>:» subito
  dopo il nome.
- **`BATTMAN`**: nell'AutoCAD italiano il campo della richiesta si chiama
  *Messaggio*. *Sincronizza* riordina anche gli `ATTRIB` degli inserimenti
  esistenti e tiene i valori.
- **`-HATCHEDIT`**: l'origine sta nell'XDATA `ACAD` (1010/1020) e nel punto
  base di ogni riga del motivo (43/44); con *Ignore* AutoCAD scrive comunque
  tutti i contorni. *Properties* e *Style* cambiano anche `HPNAME`, `HPSCALE`,
  `HPANG` e `HPISLANDDETECTION` correnti. In AutoCAD 2021 `HPNAME` parte da
  `ANGLE` e `HPDRAWORDER` da 3. Lo stile delle isole si imposta con
  `HPISLANDDETECTION` (`HPSTYLE` non esiste).
- **`VPORT` `*Active`**: 13/23 `SNAPBASE`, 14/24 `SNAPUNIT`, 15/25 `GRIDUNIT`,
  50 `SNAPANG` in **gradi**, 74 `UCSICON`, 75 `SNAPMODE`, 76 `GRIDMODE`, 77
  `SNAPSTYL`, 78 `SNAPISOPAIR`, **60 `GRIDDISPLAY`, 61 `GRIDMAJOR`**
  (confermati), 65 = 1. **`SNAPTYPE` non sta nel disegno**:
  `autocad-snap-polare.dxf` è identico allo stato di partenza (salvo date e
  GUID), e in un disegno nuovo aperto dopo `SNAPTYPE` resta 1: è del profilo,
  quindi non c'è niente da conservare nel file.
- **`RECTANG`**: `LWPOLYLINE` chiusa, gruppo 43 sempre scritto; vertici in
  senso antiorario a partire dal primo angolo dato, anche dato dall'alto a
  destra. *Chamfer* 3 e 5 nell'angolo 60,0: 3 sul lato verticale e 5
  sull'orizzontale (60,3 → 65,0). *Fillet* 4: bulge 0,4142 (tan 22,5°), primo
  vertice 120,4. *Elevation* nel 38, *Thickness* nel 39. *Rotation* 30 con
  *Dimensions* 40×20 da 540,0: 540,0 → 574.641,20 → 564.641,37.3205 →
  530,17.3205. `RECTANG` ricorda le opzioni («Modalità rettangolo corrente:
  …») e anche rotazione e quote.
- **`POLYGON`**: `LWPOLYLINE` chiusa, antioraria. Con il raggio scritto il
  poligono poggia su un lato orizzontale: con i lati dispari un vertice è in
  alto (90°), con i pari il primo vertice è a 90° + 180°/n. *Circumscribed* 20:
  vertici a 20/cos(180°/n). Con il raggio dato da un punto, *Inscribed* mette il
  primo vertice nel punto, *Circumscribed* fa del punto il centro di un lato.
  *Edge*: il primo vertice è il primo punto, il poligono sta a sinistra del
  verso dello spigolo.
- **Selezione**: `ERASE` *Last* cancella l'ultima creata e, ripetuto, l'ultima
  rimasta visibile. `U` dopo `MOVE` annulla lo spostamento, e *Previous* resta
  la selezione del `MOVE` annullato. Dopo una `LINE` annullata, *Last* prende
  l'ultima entità visibile. Dopo un `ERASE`, *Previous* risponde «Non esiste
  nessun gruppo di selezione precedente.». Da script, un `SELECT` *Previous*
  andato a vuoto lascia aperto un gruppo di annullamento, e la `U` dopo si
  ferma lì («È stato incontrato un inizio di gruppo…») senza annullare: per
  questo quella prova è in fondo allo script.
- ***Vicino*** dà la proiezione perpendicolare sull'oggetto: sulla linea
  20.070588,5.017647 da 20,5.3; su arco e cerchio sul raggio del punto; sul
  lato diritto e sull'arco della polilinea 155,0 e 180,10; sull'ellisse
  217.450335,4.339643. **Sul testo multiriga AutoCAD non aggancia**: «Nessun(o/a)
  trovato(a) per punto specificato. Punto non valido.».

## Da decidere nel piano

Fatti che toccano decisioni della scheda; la scelta resta al responsabile.

- *Vicino* «per tutte le entità della 0.8»: AutoCAD non lo dà sul testo
  multiriga.
- La griglia di `acadiso.dwt` ha `GRIDDISPLAY` **3** (anche il bit 1, «oltre i
  limiti»), non solo la griglia adattiva.
- Lo snap polare (`SNAPTYPE`) non sta nel DXF: «conservato nel file» non ha
  niente da conservare.
- `ATTREQ`: il valore di fabbrica secondo Autodesk è 1, anche in LT.
- La regola fra snap alla griglia, orto e polare non ha prove valide (punti 3 e
  4 della seduta).
- Gli alias dell'AutoCAD italiano sono diversi da quelli inglesi della scheda.
