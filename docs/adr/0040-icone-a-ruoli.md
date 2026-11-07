# ADR 0040: Icone a ruoli

- **Stato:** Accettato
- **Data:** 2026-11-07
- **Sostituisce in parte:** [ADR 0009](0009-interfaccia-grafica.md), punto 4
  (il secondo comma: icone monocromatiche)

## Contesto

Le 44 icone di oggi sono monocrome per costruzione: `themedIcon`
(`app/src/theme.cpp`) riempie tutto il disegno con il colore del testo, e la
classe `accent` di [ICONE.md](../ICONE.md) non ha ancora un colore suo. Sono
disegnate su una griglia sola, 24 px, e rimpicciolite per menu e pulsanti
piccoli. Dove manca l'icona, il ribbon ripiega su pulsanti di solo testo; i
comandi di modifica della 0.7 non ne hanno.

Nella prova di icone della
[#483](https://github.com/Matleyx78/tecnigrafo/issues/483) (sei icone di
`ARC` e `CIRCLE`) si sono visti tre problemi: due icone su sei avevano la
geometria sbagliata (in un arco per tre punti il punto intermedio non stava
sull'arco; in un arco centro-inizio-fine il centro segnato distava 32 dagli
estremi con un raggio di 24); i colori scritti nel file sparivano in un tema;
a 16 px i punti scendevano sotto il pixel e il tratteggio scompariva. La
stessa prova, ridisegnata con colori per ruolo e due griglie, è il modello di
questo ADR.

## Decisione

### 1. Quattro ruoli

Ogni elemento di un'icona ha uno e un solo ruolo, scritto come classe:

| Classe | Ruolo | Esempio |
|---|---|---|
| `geo` | geometria che il comando crea | la circonferenza di `CIRCLE` |
| `pt` | punti che l'utente indica | centro, estremi, punto per cui passa |
| `cost` | costruzione: raggi, assi, direzioni, tratteggiata | il raggio di *Center, Radius* |
| `esist` | oggetti già presenti nel disegno, su cui il comando lavora | la linea da tagliare in `TRIM` |

I colori dei ruoli li dà il **tema**, con i gettoni `icon.geo`, `icon.pt`,
`icon.cost` e `icon.esist` dell'[ADR 0039](0039-decisione-visiva-dell-interfaccia.md).
Valori iniziali:

| Gettone | Chiaro | Scuro |
|---|---|---|
| `icon.geo` | `#1971c2` | `#4dabf7` |
| `icon.pt` | `#e8590c` | `#ff922b` |
| `icon.cost` | `#868e96` | `#8b95a1` |
| `icon.esist` | il testo del tema | il testo del tema |

Le icone che non disegnano geometria (file, annulla, pannelli, impostazioni)
usano solo `esist`, cioè il colore del testo, e `pt` per l'eventuale
elemento attivo: prende il posto della classe `accent` di oggi.

### 2. Nessun colore nel file

Un'icona non scrive colori: ogni elemento disegnato porta la classe del suo
ruolo e `currentColor`. `themedIcon` dà a ogni ruolo il colore del suo gettone
quando disegna l'icona, per ogni tema e ogni dimensione. Come lo fa (riscrittura
del file letto per classe, o un foglio di stile SVG) lo sceglie il pacchetto che
lo realizza (0.7.5-P11), che lo scrive in ICONE.md; il risultato deve essere lo
stesso sui tre sistemi.

### 3. Due griglie

- **32 px**, tratto 1,75, per i pulsanti grandi del ribbon e per la pagina
  iniziale;
- **16 px**, tratto 1,25, per menu, pulsanti piccoli, barre, barra di stato e
  pannelli: disegnata **a parte**, non rimpicciolita dalla 32. Punti di almeno
  2 px di diametro, tratteggi che restano visibili.

I file si chiamano come l'azione, con la griglia in fondo
(`draw-circle-cr-32.svg`, `draw-circle-cr-16.svg`). Le dimensioni in mezzo e
quelle per gli schermi ad alta densità si ottengono dalla griglia più vicina.
La griglia da 24 px sparisce quando tutte le icone sono rifatte (0.7.5-P19–P22).

### 4. Geometria controllata in automatico

Un controllo della CI (`scripts/icone.py`, 0.7.5-P11) legge ogni icona e
verifica:

- le regole di ICONE.md (elementi ammessi, classi, griglia, margini, tratto,
  nessun colore scritto);
- la **geometria dichiarata**: un punto che dichiara di stare su un elemento
  (`data-on`) ci sta davvero, un centro dichiarato (`data-center-of`) è il centro
  dell'arco o del cerchio che nomina, entro un decimo di pixel;
- che ogni azione del registro abbia le sue due icone.

Le dichiarazioni si scrivono nell'SVG come attributi e il controllo le
legge; il loro formato lo fissa 0.7.5-P11 in ICONE.md.

### 5. Le icone dei comandi futuri

Ogni tappa che aggiunge un comando ne disegna le icone con queste regole, e le
mette nel registro delle azioni, nel ribbon, nei menu e nelle barre
([LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md), "Comandi
nell'interfaccia"). Le varianti di un comando (le cinque di `CIRCLE`) hanno
un'icona ciascuna. Il numero di icone a regime è stimato in 150–250 per il 2D.

### 6. Ambienti

I ruoli e le due griglie valgono per ogni ambiente
([ADR 0041](0041-documenti-e-ambienti.md)): vincoli dello schizzo, feature e
accoppiamenti si disegnano con gli stessi quattro ruoli. I simboli
dell'elettrico non sono icone dell'interfaccia ma contenuto dei disegni, e
restano fuori da questo ADR.

### 7. Che cosa resta dell'ADR 0009, punto 4

Le icone restano **nostre**, in SVG, con la licenza del progetto, senza forme
copiate da altri programmi o raccolte, nemmeno ridisegnate. Le regole di
disegno restano in [ICONE.md](../ICONE.md), che questo ADR aggiorna.

## Alternative considerate

- **Monocrome come oggi:** semplici, ma a 16 px un cerchio per tre punti e uno
  per due punti si confondono, e manca il modo di dire «questo lo indichi tu».
- **Colori scritti nell'SVG:** si vedono ovunque, ma un colore che va bene sul
  chiaro sparisce sullo scuro (è successo nella prova).
- **Una griglia sola, rimpicciolita:** metà del lavoro, ma a 16 px punti e
  tratteggi si perdono.
- **Icone generate da un modello senza controllo:** due su sei sbagliate nella
  prova; il controllo della geometria costa poco e lo evita.
- **Una raccolta libera esistente:** già scartata dall'ADR 0009 (stile non
  uniforme, licenze).

## Conseguenze

- Ogni comando costa due file di icone invece di uno.
- `themedIcon` diventa multicolore; i test delle icone controllano ruoli e
  griglie.
- Un errore di geometria in un'icona ferma la CI della pull request che lo
  introduce.
- ICONE.md si riscrive con le regole nuove (0.7.5-P11).
