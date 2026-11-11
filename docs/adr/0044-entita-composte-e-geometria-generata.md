# ADR 0044: Entità composte e geometria generata

- **Stato:** Proposto
- **Data:** 2026-11-11

## Contesto

La tappa 0.8 ([scheda](../tappe/0.8-quote-retini-blocchi.md)) porta nel
modello sette tipi di entità che oggi restano dati non gestiti
([ADR 0018](0018-conservazione-dati-non-gestiti.md), punto 3): `SOLID`,
`ELLIPSE`, `MTEXT`, `ATTDEF`, `INSERT` (con i suoi `ATTRIB`), `DIMENSION` e
`HATCH`. Tre di
loro non si disegnano da sé: un inserimento mostra le entità di un blocco, una
quota le entità di un blocco anonimo `*D…` che il file porta con sé, un retino
righe e riempimenti calcolati dal contorno. Anche il testo multiriga si disegna
come righe di testo calcolate.

Il modello dei tipi chiuso (`std::variant`, [ADR 0004](0004-modello-geometrico-2d.md))
fa fallire la compilazione di ogni visita che non tratta un tipo nuovo. È voluto,
ma con sette tipi e una ventina di visite vuol dire che il primo pacchetto che ne
aggiunge uno tocca file di quattro moduli, e ogni pacchetto dopo di lui gli
stessi file: una catena lunga quanto la tappa. Lo stesso vale per le tabelle
nuove (stili di testo, stili di quota, blocchi) dentro `Document` e per le loro
modifiche annullabili dentro `Editor`.

La decisione del 2026-11-07 ([DECISIONI.md](../DECISIONI.md)) chiede che il
modello della quota tenga separate tre cose: come si mostra, che cosa misura e
chi decide il valore.

## Decisione

### 1. Tipi nuovi

- `core::Entity` diventa
  `std::variant<Polyline, Line, Circle, Arc, Text, Solid, Ellipse, MText,
  AttributeDefinition, Insert, Dimension, Hatch>`, in quest'ordine. I campi di
  ciascun tipo sono negli ADR [0045](0045-blocchi-e-attributi.md) (`Insert`,
  `AttributeDefinition`), [0046](0046-stili-di-testo-e-testo-multiriga.md)
  (`MText`), [0047](0047-quote-e-stili-di-quota.md) (`Dimension`) e
  [0048](0048-retini.md) (`Hatch`) e [0052](0052-ellisse.md) (`Ellipse`); `Solid` è
  nel punto 7.
- Ogni tipo nuovo ha la sua intestazione e il suo file in `core`
  (`solid.hpp`, `ellipse.hpp`, `mtext.hpp`, `attribute_definition.hpp`, `insert.hpp`,
  `dimension.hpp`, `hatch.hpp`) con le funzioni che le visite chiamano:
  ingombro, trasformazione, punti di snap, grip, distanza dal puntatore. Le
  visite comuni (`similarity.cpp`, `proximity.cpp`, `snap_points.cpp`,
  `grips.cpp`, `dashes.cpp`, `document.cpp`, e quelle di `render`, `script`,
  `app` e `io`) chiamano solo quelle funzioni, una riga per tipo.
- **Uno scheletro, poi i tipi.** Un solo pacchetto aggiunge i sette tipi, le
  intestazioni e tutte le righe delle visite, con funzioni che per un tipo non
  ancora realizzato non sono mai raggiunte: finché il pacchetto di un tipo non
  è unito, nessun lettore e nessun comando crea entità di quel tipo (restano
  dati non gestiti), e una precondizione lo controlla. Dopo lo scheletro, ogni
  tipo tocca solo i suoi file: le visite comuni escono dalle catene dei
  pacchetti.

### 2. Geometria generata

- `core::generate(const Document&, const EntityRecord&)` restituisce la
  **geometria generata** di un'entità: un elenco di `GeneratedItem`, ognuno con
  una forma, già nel sistema del disegno, e le proprietà effettive con cui si
  disegna.
- Le forme sono `Line`, `Arc`, `Circle`, `Ellipse`, `Polyline`, `Text`, `Solid`
  e `FilledRegion` (una regione piena: anelli chiusi di segmenti e archi,
  riempiti con la regola pari-dispari, con una trasformazione affine propria).
  Per le entità semplici l'elenco ha un solo elemento, l'entità stessa.
- **Trasformazione.** Quando la trasformazione che porta una forma nel disegno
  è una similitudine, la forma arriva trasformata con `core::transformed`
  ([ADR 0031](0031-modifiche-alle-entita.md)). Altrimenti (inserimento con
  scala non uniforme) arriva trasformata **esattamente** lo stesso:
  - linee, `Solid` ed ellissi restano linee, `Solid` ed ellissi (ADR 0052);
  - cerchi e archi diventano ellissi e archi d'ellisse; una polilinea con archi
    si divide nei suoi segmenti, linee e archi d'ellisse;
  - il testo si scompone in rotazione, altezza, fattore di larghezza, obliquo e
    specchiatura (scomposizione QR della parte lineare);
  - solo una `FilledRegion` (un retino dentro un blocco) tiene l'affinità con
    sé, e `render` la applica quando la riempie.
- **Chi la usa:** `render` disegna gli elementi; la selezione con un clic e con
  la finestra e lo snap a oggetto li interrogano; l'ingombro di un'entità
  composta, nell'indice spaziale, è quello dei suoi elementi.
- **Calcolata a richiesta.** Non si memorizza nel documento, tranne la grafica
  delle quote (punto 5). Una cache, se servirà, è lavoro della tappa 1.0.
- **Quando cambia.** L'ingombro di un inserimento dipende dalla definizione del
  suo blocco: quando una definizione cambia, `Document` ricalcola l'ingombro di
  tutti gli inserimenti che la usano, anche attraverso blocchi annidati. Per
  gli altri tipi la geometria generata dipende solo dall'entità e dalle tabelle
  degli stili, che non cambiano l'ingombro registrato fino alla prossima
  modifica dell'entità.

### 3. Tabelle nuove

- Stili di testo, stili di quota e definizioni dei blocchi sono tre classi di
  `core` (`TextStyleTable`, `DimStyleTable`, `BlockTable`), ognuna nei suoi
  file, possedute da `Document`, che le espone in lettura e le sostituisce con
  `replace…()`. Nascono vuote nello scheletro; i pacchetti delle tabelle le
  riempiono senza toccare `document.hpp`.
- **Annullamento.** `Editor` ha due cambiamenti generici, uno che sostituisce
  per intero la tabella degli stili di testo o quella degli stili di quota (sono
  piccole) e uno che sostituisce o toglie **una** definizione di blocco (le
  definizioni possono essere grandi). Stanno nella stessa transazione e nella
  stessa cronologia delle entità ([ADR 0041](0041-documenti-e-ambienti.md),
  punto 4). Le operazioni che li usano stanno in file propri di `edit`, uno per
  tabella.

### 4. Dati conservati

- Le entità nuove seguono l'ADR 0018: ciò che il modello non rappresenta sta in
  `EntityExtras` e torna uguale nel file. Una rigenerazione (punto 5, ADR 0047
  e 0048) ricalcola solo ciò che il modello genera; i gruppi sconosciuti non si
  toccano.
- **Varianti non gestite.** Restano dati non gestiti, come oggi: `MINSERT`
  (inserimenti con righe o colonne), un inserimento di un blocco esterno
  (xref) o di un blocco che manca, un `ATTRIB` o `ATTDEF` multiriga, un `MTEXT`
  con colonne, un `HATCH` con archi d'ellisse, spline o sfumature. Un
  inserimento che ha un `ATTRIB` non gestito resta non gestito tutto intero,
  così nessuna operazione lo separa dai suoi attributi.

### 5. Il modello della quota

`Dimension` tiene separate le tre parti decise il 2026-11-07:

- **come si mostra** (`DimensionPresentation`): stile, variabili sovrascritte,
  posizione e testo sostituito, rotazione del testo;
- **che cosa misura** (`DimensionDefinition`): il tipo e i punti di
  definizione del DXF; un giorno, riferimenti a entità;
- **chi decide il valore** (`DimensionValue`): oggi sempre la misura
  (`ValueSource::Measured`) e il valore misurato (gruppo 42); un giorno un
  vincolo che comanda la geometria.

La **grafica** (le entità del blocco `*D…`) fa parte dell'entità: si legge dal
file e si riscrive come blocco anonimo, e Tecnigrafo la rigenera quando crea o
modifica la quota (ADR 0047). Così una quota fatta da AutoCAD si vede come in
AutoCAD anche dove Tecnigrafo non saprebbe rigenerarla.

### 6. Rapporto e domande

Il rapporto della modalità di controllo ([ADR 0011](0011-modalita-di-controllo.md))
ha una riga per ogni entità nuova, con i suoi campi, e una domanda nuova,
`generated`, che elenca la geometria generata di un'entità. Il formato sta in
[verifica/rapporto.md](../verifica/rapporto.md) e in
[verifica/domande.md](../verifica/domande.md), scritto dal pacchetto dello
scheletro di `script`.

### 7. `SOLID`

- `core::Solid`: tre o quattro vertici nell'ordine del DXF (gruppi 10, 11, 12,
  13; con tre vertici il quarto coincide con il terzo). La figura è il
  quadrilatero `v0, v1, v3, v2`, come lo disegna AutoCAD: un ordine «a farfalla»
  dà due triangoli.
- Si disegna pieno, senza contorno; si seleziona con un clic dentro la figura o
  sul bordo; snap agli estremi (i vertici); grip sui vertici. Si trasforma come
  una polilinea senza archi. Quota, spessore ed estrusione seguono l'ADR 0018,
  punto 5.
- È diverso dal motivo `SOLID` dei retini (ADR 0048).

### 8. Funzioni trigonometriche

Il codice nuovo di `core`, `cmd` e `render` non usa le funzioni
trigonometriche di `<cmath>` ma quelle di `solid_space::robust` (`sinCos`,
`atan2`; `tan`, `acos` e `hypot` quando arrivano), e `std::sqrt`, che IEEE 754
arrotonda in modo esatto (decisione del 2026-11-08, [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)).

## Alternative considerate

- **Un pacchetto per tipo che lo aggiunge al variant:** ognuno toccherebbe le
  stesse venti visite, in catena; la tappa durerebbe quanto la somma dei
  pacchetti.
- **Le entità composte fuori dal variant**, in un elenco a parte: ogni visita
  dovrebbe ricordarsi di guardarle, e il compilatore non aiuterebbe più.
- **Geometria generata memorizzata nel documento:** più veloce da disegnare, ma
  ogni modifica di un blocco o di uno stile dovrebbe tenerla allineata. Si
  rimanda alla tappa 1.0, se le misure lo chiederanno.
- **Spezzate sullo schermo** per i blocchi scalati in modo non uniforme, senza
  l'entità ellisse: era la proposta del piano; il responsabile ha scelto
  l'ellisse (ADR 0052).
- **Rigenerare sempre la grafica delle quote lette:** cambierebbe l'aspetto di
  ogni quota fatta con opzioni che Tecnigrafo non ha.

## Conseguenze

- Lo scheletro è un pacchetto di sola interfaccia: si giustifica perché toglie
  le visite comuni da tutte le catene dei tipi ([LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md),
  «Pacchetti leggeri»).
- `render` impara a riempire più anelli con la regola pari-dispari (anche sotto
  un'affinità) e a disegnare le ellissi.
- Il rapporto cambia formato per le entità nuove; i casi esistenti non cambiano.
