# Revisione di una pull request

Da leggere quando commenti una pull request che non hai aperto tu, quando la
descrizione comincia con **Revisione della correzione**, o quando modifichi una
scheda in `docs/tappe/X.Y/` della tappa aperta, anche senza toccare
`docs/DECISIONI.md` ([AGENTS.md](../../AGENTS.md), "Se ... leggi anche ...").
Chi scrive una correzione legge "Le correzioni delle schede si rivedono" prima
di scrivere la descrizione della pull request. Vale insieme al nucleo. Le pull
request aperte rivedute una dopo l'altra sono in [lotti.md](lotti.md).

## Che cosa blocca una revisione

Vale per ogni revisione, di una correzione come di un pacchetto
([DECISIONI.md](../DECISIONI.md), 2026-11-04).

- Il revisore serve a portare il lavoro al merge, non a fermarlo: **blocca solo
  un errore vero o una regola persa**. Se una regola esistente risponde, la
  applica e va avanti; i dubbi di forma li scrive come **non bloccanti**.
- Prima di dichiarare un buco controlla se una regola esistente lo decide già,
  in particolare: dati fuori dal dominio di solid-space,
  [ADR 0035](../adr/0035-taglio-offset-raccordi-unione.md), punto 23; pezzi
  degeneri, [ADR 0036](../adr/0036-tolleranze-di-tecnigrafo.md), punto 5;
  fasce di una soglia, ADR 0013 di solid-space, punto 4 ("La fascia di una
  soglia", qui sotto). Se sì, lo scrive come applicazione di quella regola, non
  come buco, e non blocca il merge. Con queste regole si trattano anche i dati
  privi di senso pratico, come lunghezze sotto la tolleranza o ampiezze di un
  ulp.
- **Un ingresso fuori da ogni uso reale non è un buco visibile**
  ([DECISIONI.md](../DECISIONI.md), 2026-11-12, «Ingressi fuori dall'uso
  reale»), né per chi rivede né per chi realizza: angoli di migliaia di giri,
  coordinate o scale di molti ordini di grandezza oltre ciò che si disegna,
  numeri al limite del `double`. Basta che il programma non si arresti
  (niente asserzioni, infiniti o NaN che proseguono): poi vale la fascia
  dichiarata o una diagnosi, e si scrive come nota non bloccante. Se serve
  perché non si arresti, la correzione è la più piccola che basta.
- Una garanzia del contratto si verifica **con i test, non con dimostrazioni
  matematiche**: il revisore che teme un ramo non coperto chiede un test che lo
  raggiunga, non un lemma.
- Il revisore usa un **modello diverso** da quello dell'autore, che la pull
  request dichiara nella sezione sull'uso dell'IA: se è il tuo stesso modello,
  non rivederla e dillo al responsabile. Il tuo verbale dice il modello che hai
  usato ([DECISIONI.md](../DECISIONI.md), 2026-11-07).

## Le correzioni delle schede si rivedono

Le decisioni sui buchi si applicano in una sola pull request (guida del
responsabile, B10a), ed è lì che nascono i buchi successivi: una regola breve che
vale da un lato e non dall'altro, o che un calcolo in `double` porta proprio al
limite.

- **Chi la scrive** mette in testa alla descrizione la riga
  **Revisione della correzione: da fare**, con le regole che cambia. Nella
  scheda ogni decisione ha il suo esempio con la risposta scelta, in
  "Test richiesti", e una regola si scrive per tutti i casi che tocca (la fine
  e l'inizio, i due versi, le due unità), non solo per quello dell'esempio.
- **Chi la rivede** è un agente diverso da chi l'ha scritta (di norma Codex;
  Claude se l'ha scritta Codex). La prende in carico come ogni pull request
  (etichetta `AA-Presa in carico` e commento) e scrive nella pull request i
  suoi commenti, ma non la modifica: niente commit, niente cambi a file, ramo,
  titolo o descrizione. Alla fine toglie l'etichetta. Controlla le sole regole
  nuove o corrette con la lista di CONTRATTI.md, punti 1–11, più due domande: la
  regola vale anche al contrario (l'altro estremo, l'altro verso, gli argomenti
  scambiati)? Che cosa succede quando il calcolo scritto nella scheda arriva
  esattamente al limite (un angolo che torna uguale, una somma che tocca il giro
  intero, due proiezioni uguali)? Scrive nella pull request un commento
  **Verbale della revisione della correzione** e, per ogni buco nuovo che
  nessuna regola esistente decide ("Che cosa blocca una revisione"), un
  commento con un esempio e le scelte possibili, come un agente che si ferma.
  Non sceglie lui.

### Buchi visibili e no

Un buco nuovo si segnala come buco solo se è **visibile**, con la definizione di
[pacchetto.md](pacchetto.md), "Buchi visibili e no" (ADR 0013 di solid-space,
punto 3): quello lo decide il responsabile. Per un buco non visibile chi rivede
controlla che la pull request porti la scelta di chi implementa, scritta nella
scheda o nel commento, il test che la fissa e la segnalazione nella
descrizione.

### La fascia di una soglia

Un caso che sta entro un errore dichiarato dal confine di `kMaxCoordinate` o da
un'altra soglia del contratto si chiude dichiarando la fascia
([ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
punto 4): dentro la fascia l'esito è libero. Non serve una decisione del
responsabile e il merge non si blocca:

- chi rivede lo segnala nel verbale come **non bloccante**, con il caso e la
  soglia;
- l'autore scrive la fascia nella scheda nella stessa pull request, senza un
  altro giro di revisione.
