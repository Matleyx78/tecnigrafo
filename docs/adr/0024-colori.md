# ADR 0024: Colori

- **Stato:** Proposto
- **Data:** 2026-10-25

## Contesto

Il DXF scrive il colore di entità e layer in due modi: l'**indice AutoCAD**
(ACI, gruppo 62, da 1 a 255, più 0 per `ByBlock` e 256 per `ByLayer`) e il
**colore RGB** (gruppo 420, dalla versione 2004). Un file può avere entrambi:
il 420 è il colore, il 62 l'indice più vicino per i programmi che non leggono
il 420. Il colore 7 si disegna bianco su sfondo scuro e nero su sfondo chiaro.

Per disegnare un indice serve la sua tabella RGB, che non sta nel DXF. Le
proprietà delle entità e dei layer sono nell'ADR 0023.

## Decisione

### 1. Il tipo

`core::Color` vale una di quattro cose: `ByLayer`, `ByBlock`, un **indice**
da 1 a 255, un **RGB** a 8 bit per canale. I layer ammettono solo indice e RGB.

### 2. DXF

- **Lettura:** se c'è il 420, il colore è RGB e il 62 si ignora; altrimenti
  vale il 62. Per un layer, un 62 negativo vuol dire **spento** (ADR 0023) e il
  suo valore assoluto è il colore. Un 62 fuori da 0–256 (in valore assoluto
  per i layer) rende l'entità o il layer da conservare grezzo come incoerente,
  e lo si conta nel messaggio d'apertura.
- **Scrittura:** un indice si scrive con il 62; un RGB con il 420 e con il 62
  dell'indice più vicino (punto 4). Il 62 di un RGB letto da un file e non
  cambiato si riscrive com'era.

### 3. La tabella degli indici

- La tabella indice → RGB la scriviamo noi in `core`, dai valori che il DXF
  usa: i colori da 1 a 9, le 24 tinte da 10 a 249 in cinque luminosità e due
  saturazioni, i grigi da 250 a 255.
- È verificata dai riferimenti con le risposte di ezdxf, stampate nel log
  dell'oracolo (ADR 0020; pacchetto 0.6-P8). Dove la nostra tabella e ezdxf non
  coincidono vale ezdxf, che rispecchia i file di AutoCAD.

### 4. Indice più vicino

Per un RGB, l'indice da 1 a 255 con la minore distanza euclidea nello spazio
RGB, escluso il 7; a parità, l'indice minore.

### 5. Colore 7 e tema

L'indice 7 si disegna con il colore principale del tema: nero sul tema chiaro,
bianco sullo scuro (ADR 0009, punto 5). Nessun altro colore cambia con il tema,
nemmeno un RGB bianco. In modalità di controllo il tema è il chiaro (ADR 0011,
punto 5).

### 6. Testo

Nei comandi, nel rapporto e nelle domande un colore si scrive:

| Colore | Scrittura | Accettato anche |
|---|---|---|
| `ByLayer` | `bylayer` | `BYLAYER`, `256` |
| `ByBlock` | `byblock` | `BYBLOCK`, `0` |
| indice | `1`…`255` | `red`, `yellow`, `green`, `cyan`, `blue`, `magenta`, `white` per 1–7 |
| RGB | `255,128,0` | — |

Il rapporto scrive sempre la prima colonna, in minuscolo; i comandi accettano
anche la seconda, senza distinguere maiuscole.

## Alternative considerate

- **Solo RGB nel modello**, convertendo gli indici alla lettura: perderebbe
  l'indice scritto nel file, e il colore 7 non potrebbe seguire il tema.
- **Copiare la tabella da un altro programma:** è la strada che l'ADR 0003
  esclude per il codice; i valori li verifichiamo invece con ezdxf come oracolo.
- **Distanza percettiva (CIELAB) per l'indice più vicino:** più precisa, ma il
  62 serve solo ai programmi vecchi; la distanza RGB è semplice e ripetibile.
- **Cataloghi di colori** (gruppo 430): restano grezzi; non c'è modo di
  ridistribuire i cataloghi commerciali.

## Conseguenze

- Il render risolve il colore effettivo (`ByLayer` dal layer; `ByBlock` vale
  come 7 finché non ci sono blocchi, tappa 0.8) e poi l'RGB dalla tabella.
- La finestra dei colori dell'interfaccia (pacchetto 0.6-P21) mostra gli
  indici con questa tabella e accetta un RGB.
- I colori del tema (sfondo, griglia, evidenziazione) non sono colori del
  disegno e non passano da qui.
