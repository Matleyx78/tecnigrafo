# Linguaggio degli script (`.tgs`)

Specifica del linguaggio con cui la modalità di controllo pilota il programma
([ADR 0011](../adr/0011-modalita-di-controllo.md), punto 3). È un **contratto**:
una direttiva si aggiunge, non si cambia; per cambiarla serve un nuovo ADR.

Lo stesso script si esegue al **livello 1** (`tecnigrafo-script`, senza
interfaccia) e al **livello 2** (`tecnigrafo --script`, con la finestra e
eventi veri di tastiera e mouse), e deve dare lo stesso
[rapporto](rapporto.md).

## Il file

- Testo UTF-8, con o senza BOM, righe terminate da LF o CRLF.
- Una riga per azione. Il terminatore di riga non fa parte della riga; un
  terminatore finale non aggiunge una riga vuota: `PLINE⏎` è una riga sola,
  `PLINE⏎⏎` sono due, la seconda vuota.
- Solo il **primo carattere** decide il tipo di riga:

| Prima della riga | Significato |
|---|---|
| niente (riga vuota) | `Invio` |
| `;` | commento, ignorato |
| `!` | direttiva |
| qualunque altro | testo scritto sulla riga di comando, poi `Invio` |

Una riga normale si conserva **esattamente come è scritta**, spazi compresi:
` ; x` (con uno spazio davanti) non è un commento ma testo, e `  @10,0  `
arriva al programma con i suoi spazi. Il runner dei comandi toglie poi gli spazi
dove la sintassi lo prevede (ADR 0005); con le richieste di testo della tappa
0.4 (ADR 0014) gli spazi restano.

## Righe normali

Una riga normale è ciò che l'utente scrive sulla riga di comando, con la
sintassi dell'ADR 0005: nomi di comandi e alias (`PLINE`, `PL`), coordinate
(`10,20`, `@5,0`, `50<45`, `@50<45`, `50`), parole chiave (`C`, `Close`, `U`).
Uno script `.scr` di AutoCAD che usa solo comandi presenti in Tecnigrafo è già
uno script valido.

Come nell'interfaccia, prima di consegnare il testo la riga di comando ne
scrive l'eco nella cronologia: la domanda in quel momento, uno spazio e il
testo (`Specify start point: 0,0`); per una riga vuota la sola domanda. Il
[rapporto](rapporto.md) riporta la cronologia.

## Direttive

Una direttiva è `!` seguito subito dal nome (`! esc` è un errore), poi gli
argomenti separati da spazi o tabulazioni. Nomi e parole fisse sono in
minuscolo. Numeri e punti hanno la sintassi delle coordinate dell'ADR 0005:
punto decimale, segno ed esponente facoltativi, nessuno spazio dentro un punto
(`1.5,-2`, non `1.5, -2`). Le **coordinate sono del disegno**, mai in pixel.

| Direttiva | Effetto | Livello |
|---|---|---|
| `!view cx,cy scale WxH` | vista fissa: centro `cx,cy`, `scale` unità del disegno per pixel (> 0), area di `W`×`H` pixel (interi da 1 a 16384) | 1 e 2 |
| `!move x,y` | il puntatore va al punto del disegno | 1 e 2 |
| `!click x,y` | il puntatore va al punto e fa clic con il tasto sinistro | 1 e 2 |
| `!click x,y remove` | lo stesso, con `Shift` premuto (toglie dalla selezione) | 1 e 2 |
| `!rightclick` | clic con il tasto destro: conferma, come `Invio` | 1 e 2 |
| `!esc` | il tasto `Esc` | 1 e 2 |
| `!snap on` / `!snap off` | snap ad oggetto acceso o spento, senza scrivere nella cronologia | 1 e 2 |
| `!ortho on` / `!ortho off` | orto acceso o spento, senza scrivere nella cronologia | 1 e 2 |
| `!report` | aggiunge al rapporto una sezione con lo stato in quel momento | 1 e 2 |
| `!level 1` / `!level 2` | il livello che lo script richiede | 1 e 2 |
| `!tolerance <tipo> <valore>` | cambia una tolleranza del confronto, per tutto lo script | 1 e 2 |
| `!key <tasto>` | un tasto o una scorciatoia, come la scrive Qt: `F3`, `F8`, `Ctrl+Z`, `Delete` | solo 2 |
| `!screenshot <nome>` | salva la vista in `<nome>.png`; il nome ha solo lettere, cifre, `_` e `-` | solo 2 |

### Dettagli

- **`!view`.** Senza `!view` la vista è centro `0,0`, scala 1, 800×600. Al
  livello 1 conta solo la scala, che fissa le tolleranze in pixel del runner:
  la scelta con un clic (3 pixel) e l'apertura dello snap (10 pixel)
  ([ADR 0008](../adr/0008-selezione-e-snap.md), punto 7). Al livello 2 fissa
  anche la finestra, e da lì la conversione delle coordinate in pixel. Si può
  ripetere: vale da quel punto in poi.
- **`!click`** muove prima il puntatore sul punto, come un mouse vero, poi fa
  clic. Il puntatore resta lì: una distanza scritta dopo (`50`) prende la
  direzione da quel punto.
- **`!snap` e `!ortho`** impostano lo stato degli aiuti senza passare dai tasti:
  non scrivono `<Osnap on>` nella cronologia. Per provare i tasti `F3` e `F8`,
  con il loro messaggio, si usa `!key` al livello 2.
- **`!level 2`** può stare in qualunque punto dello script. Uno script con
  `!key` o `!screenshot` deve contenerlo, altrimenti è un errore: così uno
  script che si legge al livello 1 si esegue sempre al livello 1. Uno script con
  `!level 2` non si esegue al livello 1: `tecnigrafo-script` lo salta (codice
  77, [casi.md](casi.md)). `!level 1` e `!level 2` insieme sono un errore.
- **`!tolerance`** vale per il confronto dell'intero script, ovunque si trovi:

| Tipo | Valore | Predefinito | Significato |
|---|---|---|---|
| `absolute` | numero ≥ 0 | `1e-9` | due numeri sono uguali se differiscono al più di tanto, in unità del disegno |
| `relative` | numero ≥ 0 | `0` | oppure se differiscono al più di tanto per il maggiore dei due, in valore assoluto |
| `channel` | intero da 0 a 255 | `8` | immagini: un pixel è diverso se un canale differisce di più di tanto |
| `pixels` | percentuale da 0 a 100 | `0.1` | immagini: il caso fallisce se i pixel diversi superano questa percentuale |

  Le tolleranze si cambiano solo così, cioè solo da chi scrive i riferimenti
  (ADR 0011, punto 6).

## Errori

Uno script con una riga che non rispetta questa specifica non è un caso valido:
`tecnigrafo-script` indica il numero della riga e il motivo, ed esce con il
codice 2 ([casi.md](casi.md)). Sono errori, tra gli altri: una direttiva
sconosciuta o scritta in maiuscolo, argomenti mancanti o in più, un numero
scritto con la virgola, `!key` senza `!level 2`.

## Esempio

```
; PLINE chiusa, poi snap all'estremo con la vista a mezza unità per pixel
!view 0,0 0.5 800x600
PLINE
0,0
100,0
@0,50
50<90
C
!report
PL
!move 101,49
!click 101,49
!ortho on
!move 130,60
20
```

Lo script completo e il suo rapporto, scritto a mano, sono in
`tests/script/dati/pline.tgs` e `pline-atteso.txt` (dati di prova del modulo,
non casi di verifica).
