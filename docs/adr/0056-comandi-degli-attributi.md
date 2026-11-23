# ADR 0056: Comandi degli attributi

- **Stato:** Accettato
- **Data:** 2026-11-21
- **Corretto:** 2026-11-22 — punto 5, dopo la revisione delle interfacce PT5
  (#928): fra `ATTRIB` con etichette uguali tiene valore e handle il primo
  libero, gli altri spariscono, e l'`ATTRIB` che resta conserva i suoi dati
  non gestiti; la sincronizzazione aggiorna ogni record `INSERT` del blocco,
  anche dentro altre definizioni, una volta (decisioni del 2026-11-22 in
  `docs/DECISIONI.md`, «Revisione delle interfacce PT5 della 0.8.1»).
- **Corretto:** 2026-11-23 — punto 2, dopo il buco di RT14 (#954): le domande
  del testo di `-ATTDEF` sono scritte qui, perché `TEXT` non ha *Justify* né
  *Style*; Esc a metà comando non cambia niente (decisione del 2026-11-23 in
  `docs/DECISIONI.md`, «Domande del testo di `-ATTDEF` (PT14)»).

## Contesto

L'[ADR 0045](0045-blocchi-e-attributi.md), punto 6, ha lasciato fuori i comandi
per creare e modificare gli attributi e le domande all'inserimento. Il
responsabile li ha portati nella 0.8.1 ([DECISIONI.md](../DECISIONI.md),
2026-11-21, «Testo formattato, griglia, snap e attributi della 0.8.1», punto 7,
ed «Esiti dei DXF di prova e della seduta della 0.8.1», punto 4, che porta
`ATTREQ` a 1). I DXF `autocad-attributi`, `autocad-attdisp-on`/`-off`,
`autocad-attsync` e `autocad-battman`, con le loro cronologie
([tappe/0.8.1/dxf-di-prova.md](../tappe/0.8.1/dxf-di-prova.md)), mostrano le
richieste e i risultati di AutoCAD.

## Decisione

### 1. Il modello

- **Posizione bloccata** (*Lock position*): un campo di `AttributeDefinition` e
  di `Attribute`, il **secondo** gruppo 280 del DXF (= 1); il primo 280 (la
  versione) resta dov'è. Si legge, si scrive e si copia dall'`ATTDEF`
  all'`ATTRIB`.
- **Ordine degli attributi** di una definizione: quello degli `ATTDEF` fra le
  entità del blocco. Con `-BLOCK` e `_ALL` è l'**inverso** della creazione,
  come AutoCAD (la selezione `ALL` dà gli oggetti dal più recente).
- **`ATTDISP`** è un dato del disegno, `$ATTMODE`: 0 (*OFF*, nessun attributo
  si disegna), 1 (*Normal*, gli invisibili no, predefinito), 2 (*ON*, anche gli
  invisibili). Selezione e snap seguono ciò che si disegna. Gli `ATTDEF` nel
  disegno si disegnano sempre, come in AutoCAD.

### 2. `-ATTDEF` e `AFLAGS`

- `AFLAGS` è un valore della **sessione**, come `PEDITACCEPT` (ADR 0050), con i
  bit di AutoCAD (1 invisibile, 2 costante, 4 da verificare, 8 preimpostato, 16
  posizione bloccata). All'avvio vale 16, come in AutoCAD 2021 (*Lock position*
  acceso; [DECISIONI.md](../DECISIONI.md), 2026-11-22, «Decisioni D1–D10 del
  piano della 0.8.1», punto 4).
- `-ATTDEF` (alias `ATT`, anche `ATTDEF`), con le richieste dell'AutoCAD
  italiano tradotte: `Current attribute modes: Invisible=N Constant=N
  Verify=N Preset=N Lock position=N`, `Enter an option to change
  [Invisible/Constant/Verify/Preset/Lock position] <done>:` (ogni opzione
  inverte un modo e ripete le due righe), `Enter attribute tag name:`, `Enter
  attribute prompt:` (non per *Constant*), `Enter default attribute value:` o,
  per *Constant*, `Enter attribute value:`, poi le domande del testo, tradotte
  dalla cronologia con le parole di AutoCAD in inglese:
  `Current text style:  "<stile>"  Text height:  <altezza>  Justify:  <giustificazione>`
  (altezza con quattro decimali), `Specify start point of text or [Justify/Style]:`; *Justify* con `Enter an
  option [Left/Center/Right/Align/Middle/Fit/TL/TC/TR/ML/MC/MR/BL/BC/BR]:` e
  la richiesta del punto di ciascuna (scheda
  [PT14](../tappe/0.8.1/PT14-attdef.md)); *Style* con `Enter style name or
  [?] <stile corrente>:`, che rende corrente lo stile scelto e ripete le due
  righe con il nome nuovo; poi altezza e rotazione. Alla fine `AFLAGS`
  prende i modi usati; con Esc a metà comando nessun `ATTDEF`, nessun passo
  di annullamento e `AFLAGS` invariato. Etichetta senza spazi (`validAttributeTag`); richiesta,
  predefinito e valore accettano gli spazi.

### 3. Domande all'inserimento

- `ATTREQ` è un valore della sessione, 1 all'avvio come AutoCAD (decisione
  citata, punto 4). Con 1 `-INSERT`, dopo la rotazione, scrive `Enter attribute
  values` e chiede ogni attributo non costante e non preimpostato,
  nell'ordine della definizione: `<richiesta> <predefinito>:` (con la richiesta
  vuota, l'etichetta); Invio dà il predefinito. Poi, se ce ne sono, `Verify
  attribute values` e di nuovo i soli *Verify*, con il valore appena dato come
  predefinito. Con 0 si fa come nella 0.8.
- Gli script che non vogliono le domande mettono `ATTREQ` 0 in testa, come
  quelli di AutoCAD.

### 4. Modificare

- **`-ATTEDIT`** (alias `ATE`, anche `ATTEDIT`), come la cronologia: `Edit
  attributes one at a time? [Yes/No] <Y>:`; i tre filtri `Enter block name
  specification <*>:`, `Enter attribute tag specification <*>:`, `Enter
  attribute value specification <*>:`, con i caratteri jolly di AutoCAD (`*`,
  `?`, `#`, `@`, `.`, `[…]`, `~`, `` ` ``, separati da virgole), senza
  distinguere maiuscole e minuscole. Poi:
  - **uno per uno:** `Select Attributes:` (clic, finestra, intersezione, solo
    attributi visibili che passano i filtri), `<n> attributes selected.`, e per
    ciascuno `Enter an option [Value/Position/Height/Angle/Style/Layer/Color/Next]
    <N>:`, con *Value* `Enter type of value modification [Change/Replace]
    <R>:`; *Position*, *Height*, *Angle* rifiutati per un attributo con la
    posizione bloccata;
  - **globale:** `Performing global editing of attribute values.`, `Edit only
    attributes visible on screen? [Yes/No] <Y>:` (con *No* anche gli invisibili
    e `Drawing must be regenerated afterwards.`), i filtri, con *Yes* `Select
    Attributes:`, poi `Enter string to change:` e `Enter new string:`, che
    sostituisce **ogni** occorrenza nei valori scelti.
  Tutto il comando è un passo di annullamento.
- **`TEXTEDIT`** su un `ATTDEF` cambia etichetta, richiesta e predefinito; su un
  inserimento i valori ([ADR 0054](0054-testo-formattato-ed-editor.md), punto 6;
  nel programma una finestra con etichette e valori).
- **Pannello Proprietà:** gli attributi dell'inserimento, con il valore
  modificabile.

### 5. Definizioni e sincronizzazione

- **Sincronizzare** un inserimento con la sua definizione (`ATTSYNC`, e
  *Sincronizza* di `BATTMAN`), come la cronologia di `autocad-attsync`: per ogni
  `ATTDEF` non costante della definizione, nell'ordine, l'`ATTRIB` con la
  stessa etichetta tiene il **valore** e prende tutto il resto (posizione,
  altezza, rotazione, stile, modi, proprietà) dall'`ATTDEF` portato nel disegno
  dalla trasformazione dell'inserimento; quelli senza `ATTRIB` nascono con il
  predefinito; gli `ATTRIB` la cui etichetta non c'è più spariscono. Gli handle
  degli `ATTRIB` che restano non cambiano. Fra etichette uguali vale la prima:
  ogni `ATTDEF` prende il primo `ATTRIB` ancora libero con la sua etichetta, e
  quelli in più spariscono; l'`ATTRIB` che resta conserva i suoi dati non
  gestiti (XDATA, dizionari: [ADR 0006](0006-modello-dati-dwg-e-formato-cadx.md)).
- **`ATTSYNC`**: `Enter an option [?/Name/Select] <Select>:`, poi il nome o
  l'inserimento; aggiorna **tutti** gli inserimenti del blocco, senza conferma
  (ogni record `INSERT`, nel disegno e dentro le definizioni di altri blocchi,
  una volta), e chiude con `ATTSYNC complete.`.
- **`BATTMAN`** (solo nel programma): sceglie un blocco con attributi, mostra
  gli `ATTDEF` nell'ordine, e li **sposta** (su e giù), li **modifica**
  (etichetta, richiesta, predefinito, modi, testo) e li **toglie**; ogni
  operazione è un passo di annullamento sulla definizione. *Sincronizza*
  applica la regola qui sopra a tutti gli inserimenti, ordine compreso.
- **`ATTDISP`**: `Enter attribute visibility setting [Normal/ON/OFF]
  <Normal>:`, che cambia `$ATTMODE` senza annullamento e segna il disegno
  modificato.

## Alternative considerate

- **`ATTREQ` 0 per difetto:** era la decisione del 2026-11-11, sostituita
  dopo la verifica in AutoCAD (decisione citata, punto 4).
- **`BATTMAN` nel pacchetto di grafica a lato:** scartato dal responsabile, non
  è solo aspetto.
- **`AFLAGS` a 0:** più comodo per chi scrive gli script, ma diverso da
  AutoCAD; scartato dal responsabile.

## Conseguenze

- `EATTEDIT` completo, l'estrazione dei dati e gli attributi multiriga restano
  fuori (scheda della 0.8.1).
- I casi della 0.8 che inseriscono blocchi non hanno `ATTDEF` che chiedono un
  valore: con `ATTREQ` 1 non cambiano (prova a secco della scheda
  [PT17](../tappe/0.8.1/PT17-attreq.md)).
