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
(ADR 0020, esteso alle entità della tappa da PT3); i valori attesi si copiano
dal log dell'oracolo, non si calcolano a mano e non si ricavano da Tecnigrafo.
Le hash SHA-256 si scrivono qui quando i file sono pubblicati.

| File | Che cosa contiene | Riferimenti |
|---|---|---|
| `autocad-solid.dxf` | `SOLID` da 3 e da 4 vertici, uno «a farfalla», uno su un layer con colore, uno specchiato | RT10 |
| `autocad-stili-testo.dxf` | stili `Standard`, uno con altezza fissa 5, uno con fattore di larghezza 0,8 e obliquo 15°; un `TEXT` per stile | RT11, RT12 |
| `autocad-testo-multiriga.dxf` | `MTEXT` con larghezza 0 e 60, i nove punti di allineamento, due paragrafi (`\P`), una parola più larga della colonna, interlinea 1,5, rotazione 30°; uno con formati (`\f`, `\H`, `\C`, `\S1/2;`) | RT13, RT14, RT15 |
| `autocad-blocchi.dxf` | un blocco semplice; uno con entità sul layer `0` e colore *DaBlocco*; uno annidato in un altro; inserimenti con scala 1, 2, −1 (specchiato), 2 e 1 (non uniforme) e rotazione 30°; un blocco con `ATTDEF` variabile, invisibile, costante e preimpostato, inserito con i valori predefiniti (`ATTREQ` 0); un inserimento specchiato con `MIRROR` e `MIRRTEXT` 0; un blocco ridefinito dopo l'inserimento; un blocco non esplodibile | RT16, RT17, RT18, RT20, RT21 |
| `autocad-esplodi.dxf` | copie degli inserimenti di `autocad-blocchi.dxf` esplose, compreso il blocco con attributi; una quota esplosa | RT29 |
| `simbolo-contatto.dxf`, `simbolo-con-tipo-di-linea.dxf` | due «simboli» da inserire come file: `$INSBASE` diverso dall'origine, un `ATTDEF` costante e uno variabile, un layer e un tipo di linea che il disegno di destinazione ha con proprietà diverse | RT19, RT21 |
| `autocad-inserisci-file.dxf` | il disegno in cui i due simboli sono stati inseriti con `-INSERT`, una volta con il nome già esistente e *Redefine* = *Yes* | RT19, RT21 |
| `autocad-quote.dxf` | con `ISO-25`: lineari orizzontale, verticale e ruotata, allineata, angolare fra due linee, radiale e diametrale su un cerchio e su un arco, una quota stretta con le frecce fuori; con uno stile `PROVA` (`DIMTAD` 0, `DIMDEC` 3, `DIMDSEP` `.`, `DIMTXT` 3,5); una con testo sostituito `<> mm`; una quota ruotata, una scalata e una specchiata dopo la creazione; una quota coordinata (tipo che Tecnigrafo conserva soltanto) | RT22, RT23, RT24, RT25, RT26, RT27, RT28 |
| `autocad-retini.dxf` | `ANSI31`, `NET` e `SOLID` su un rettangolo di quattro linee con un cerchio dentro (punto interno) e con isole annidate; un retino da *Select objects*; un motivo ruotato di 30° e scalato 2; gli stili *Normal*, *Outer* e *Ignore*; un retino con un motivo diverso (`ANSI37`) | RT30, RT31, RT42, RT43, RT44 |
| `autocad-pedit.dxf` | linee e archi convertiti in polilinee; *Join* con distanza 0; *Multiple* con distanza 2 e *Extend*, *Add* e *Both*, su linee che non si toccano, su linee che si sovrappongono e su un arco seguito da una linea; *Close*, *Open*, *Reverse*, *Ltype gen* | RT32, RT33 |

## Che cosa provare a mano in AutoCAD LT

Oltre ai disegni, da scrivere nella pull request che li pubblica:

1. `-DIMSTYLE` → *STatus* di `Standard` e `ISO-25` in un disegno nuovo da
   `acadiso.dwt` (ADR 0047, punto 2).
2. Tracciamento: in `LINE`, acquisiti due punti, dopo aver indicato il primo
   punto i segni dei punti acquisiti restano o spariscono? (ADR 0049, punto 2.)
