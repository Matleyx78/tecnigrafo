# Pacchetto di lavoro

Da leggere quando la issue ha titolo `[X.Y-PTN]` o `[FN]`, o porta l'etichetta
`pacchetto` ([AGENTS.md](../../AGENTS.md), "Se ... leggi anche ..."). Vale
insieme al nucleo, che resta da leggere per intero.

I nomi qui sotto usano i prefissi dalla 0.8: `PT`/`RT`/`CT` in Tecnigrafo,
`PS`/`RS`/`CS` in solid-space. Per le tappe fino alla 0.7.5 restano `P`/`R`/`C`,
anche nei file in attesa (`0.7.5-R1.txt`); nulla si rinomina. I rami seguono:
`pt0.8-05-…` per il lavoro, `rt0.8-05-…` per i riferimenti.

## Prima di scrivere codice

Leggi, in quest'ordine:

1. [docs/ROADMAP.md](../ROADMAP.md), solo la tabella "Stato": quale tappa è
   **aperta**.
2. La **scheda** del tuo pacchetto, `docs/tappe/X.Y/PTN-<nome>.md`, citata nella
   issue: file, interfacce, test e criterio di "fatto". Del documento della tappa
   ti basta la tabella dei pacchetti.
3. Gli ADR citati dalla scheda, e [docs/ARCHITETTURA.md](../ARCHITETTURA.md)
   solo se la scheda lo cita o aggiunge un modulo.
4. [docs/CODING_STYLE.md](../CODING_STYLE.md).

Lavora **su un solo pacchetto**. Se il compito non corrisponde a un pacchetto della
tappa aperta, non scrivere codice: serve prima una issue approvata.

Le etichette `agente:…` e chi fa che cosa sono in
[docs/LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md).

### Se ti chiedono di prendere il prossimo pacchetto

Scegli **una sola** issue, la prima nell'ordine del piano che:

1. è aperta e porta la **tua** etichetta `agente:…`;
2. non porta `AA-Presa in carico`;
3. ha chiuse tutte le issue della sua riga "Dipende da";
4. ha chiusa la sua issue di riferimenti (`[X.Y-RTN]`), se esiste;
5. se usa intestazioni di solid-space, non c'è in solid-space una issue aperta
   con l'etichetta `contratto` che le nomina nel titolo. Controlla le
   intestazioni citate dalla scheda del pacchetto, anche in **Da leggere**.

Per "il prossimo pacchetto di riferimenti" le condizioni sono in
[riferimenti.md](riferimenti.md). Se nessuna issue va bene, non iniziare: scrivi
all'utente quali issue sono tue e cosa le blocca. Il pacchetto successivo si
prende in una nuova sessione, dopo il merge; le eccezioni sono in
[lotti.md](lotti.md).

## Modalità di controllo e riferimenti

Un pacchetto è finito quando la CI confronta il programma con casi di verifica
scritti **prima** e da **un altro agente**
([ADR 0011](../adr/0011-modalita-di-controllo.md)). Le regole per chi scrive i
riferimenti sono in [riferimenti.md](riferimenti.md), "In un pacchetto di
riferimenti (`[X.Y-RTN]`)".

### In un pacchetto di lavoro

- Non modificare mai `tests/verifica/casi/` né `tests/verifica/sorgenti.json`.
- Cancella da `tests/verifica/in-attesa/` i file dei riferimenti del tuo
  pacchetto, uno per ogni sua issue di riferimenti: `<X.Y>-RT<N>.txt`, con una
  lettera minuscola in fondo (`<X.Y>-RT<N>a.txt`) se i riferimenti sono divisi
  in più blocchi. Così i suoi casi diventano obbligatori.
- La parola `esente` in un file di `tests/verifica/in-attesa/` la aggiunge solo
  un pacchetto che porta un controllo anticipato, se la sua scheda lo dice
  ([ADR 0037](../adr/0037-controllo-anticipato-dei-riferimenti.md), punto 9).
- Se un caso ti sembra sbagliato, non fermarti al primo: guarda **tutti** i
  casi del pacchetto, senza cambiare nulla, e poi **fermati del tutto una volta
  sola**, anche se il resto del pacchetto è pronto:
  1. non adattare il codice al riferimento che ritieni sbagliato, né il
     riferimento al codice; il caso resta com'è;
  2. non aprire la pull request, non avviare la CI e non eseguire altri controlli;
  3. scrivi nella issue, **in un solo commento** per tutti i casi che ti
     sembrano sbagliati, il nome di ciascuno, il valore atteso, quello che dà il
     programma e perché il riferimento ti sembra sbagliato;
  4. se hai lavoro non pubblicato, pubblica il ramo **senza** pull request (così
     non parte la CI) e scrivi nel commento il nome del ramo;
  5. togli `AA-Presa in carico` e termina la sessione.

  Il caso lo corregge un pacchetto di riferimenti; poi il pacchetto si
  riprende. In un lotto passa al pacchetto successivo. Se la pull request è già
  aperta (per esempio te ne accorgi da un controllo della CI che fallisce), vale
  questa regola e non quella dei controlli che falliscono: scrivi nella pull
  request ciò che chiede il punto 3, togli l'etichetta e termina la sessione,
  senza aspettare la CI.

## Un buco nella scheda

**Un buco nella scheda resta in Tecnigrafo.** Con la lista di
[docs/CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md)
rileggi anche le interfacce di Tecnigrafo che la scheda definisce o che usi
(tipi e funzioni di `core`, `edit`, `cmd`), e come la scheda passa i risultati
da una funzione all'altra. Il buco è **nella scheda** quando il contratto di
solid-space determina la risposta, ma intestazione di Tecnigrafo, ADR e scheda
no, o si contraddicono: per esempio la scheda passa a una funzione solo valori
arrotondati, e due tagli distinti decisi esattamente da solid-space arrivano
uguali (#359). Se la risposta manca anche nell'intestazione di solid-space, il
buco è nel contratto ([contratto.md](contratto.md), "Un buco nel contratto");
se non sai quale dei due è, scrivilo e lascia decidere al responsabile.

### Buchi visibili e no

Vale la regola dei buchi dell'[ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
punto 3, anche per i buchi delle schede di Tecnigrafo. Un buco è **visibile**
quando, per dati dentro il dominio, cambia almeno una di queste cose:

- l'esito o la diagnosi (il messaggio che l'utente legge);
- il numero o l'ordine delle entità risultanti;
- una coordinata di più della tolleranza della regola, epsilon(M)
  dell'[ADR 0036](../adr/0036-tolleranze-di-tecnigrafo.md).

**Un buco visibile lo decide il responsabile.** Allora:

1. fermati prima di scrivere codice o casi, senza scegliere una risposta,
   nemmeno segnalandola;
2. nella issue del tuo pacchetto scrivi che cosa manca, un esempio con i dati e
   le scelte possibili, con riferimenti assoluti; **niente** testo per una issue
   di solid-space;
3. segui i punti 2–4 di "Se serve qualcosa a solid-space"
   ([contratto.md](contratto.md)): che cosa è fatto e che cosa ne dipende, ramo
   pubblicato senza pull request, etichetta tolta e sessione terminata.

Decide il responsabile, in [docs/DECISIONI.md](../DECISIONI.md); la scheda si
corregge con la decisione e il pacchetto si riprende dopo il merge.

**Un buco non visibile non ferma.** Lo sceglie chi implementa: la scelta più
semplice, scritta nella scheda o nel commento dell'interfaccia, fissata da un
test e segnalata nella pull request.
