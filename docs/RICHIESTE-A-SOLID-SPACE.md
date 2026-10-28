# Come chiedere qualcosa a solid-space

[solid-space](https://github.com/Matleyx78/solid-space) è il kernel geometrico di
Tecnigrafo ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md)): predicati
geometrici robusti, solver dei vincoli, naming topologico stabile e solidi 3D su
OCCT. Cresce **solo su richiesta di Tecnigrafo**: non ha un piano di tappe scritto
in anticipo. Quando a Tecnigrafo serve un comportamento geometrico che non c'è, si
scrive una richiesta.

La fonte delle regole è in solid-space (modello:
[docs/richieste/0000-modello.md](https://github.com/Matleyx78/solid-space/blob/main/docs/richieste/0000-modello.md),
decisione:
[ADR 0007](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md)).
Questo file ne è un riassunto con le regole del lato di Tecnigrafo: se i due
divergono sul modo di scrivere e valutare la richiesta, vale solid-space.

## Quando si scrive una richiesta

- Serve un calcolo o un'operazione geometrica che spetta a solid-space (ADR 0030,
  punto 2: decidere da che parte sta un punto, se due elementi si intersecano,
  sono tangenti o coincidono, costruire un'intersezione o un raccordo con un
  errore garantito, risolvere un vincolo, riconoscere una faccia dopo una
  modifica, fare una booleana) e Tecnigrafo non riesce a farlo, o lo fa male.
- **Non** si scrive per ciò che è di Tecnigrafo: comandi, interfaccia, annulla e
  ripeti, salvataggio nel `.cadx`, tolleranze in pixel. Quelli restano qui.

Il bisogno si cerca in due momenti:

1. **Quando si prepara il piano** di una tappa: le richieste entrano nella sezione
   "Richieste a solid-space" del piano ([tappe/0000-modello.md](tappe/0000-modello.md)),
   e i pacchetti che ne dipendono lo dicono in "Dipende da".
2. **Durante la tappa**, se emerge un bisogno che il piano non aveva visto.
   L'agente che lo trova si ferma e scrive il testo della richiesta nella issue
   del suo pacchetto (AGENTS.md, "Quando fermarsi"); il responsabile decide se
   aprirla e aggiorna il piano in "Modifiche al piano".

Conviene scrivere la richiesta **appena si vede il bisogno**, non quando la tappa
è ferma ad aspettare.

## Come si scrive

Una richiesta dice **di che cosa c'è bisogno, non come soddisfarlo**. Non si scrive
«usiamo OCCT» né «scrivete un algoritmo»: se basta OCCT, se va corretto, se serve un
pezzo nuovo o se il bisogno non spetta a solid-space lo decide solid-space. Può
essere ampia: solid-space risponde con gli obiettivi che può raggiungere.

1. **Titolo.** «Richiesta: » e il bisogno in una frase.
2. **Da dove nasce.** La tappa di Tecnigrafo e il collegamento alla issue, alla
   pull request o alla discussione in cui è emerso.
3. **Il bisogno.** Che cosa Tecnigrafo non riesce a fare o fa male, e che cosa
   succede oggi.
4. **Casi concreti.** Ingressi, file, sequenze di operazioni in cui il bisogno si
   presenta. Non sono i casi di verifica: quelli li scrive solid-space.
5. **Quando è soddisfatta.** Una condizione che Tecnigrafo può osservare.
6. **Vincoli.** Robustezza, prestazioni, sistemi operativi, dipendenze che
   Tecnigrafo non può avere (per esempio compilare senza OCCT).
7. **Che cosa blocca.** Che cosa non può andare avanti in Tecnigrafo senza questa
   richiesta, e da quando.

Non si scrivono soluzioni, nomi e firme di funzioni desiderate, valori attesi dei
casi di verifica. Un vincolo di interfaccia che Tecnigrafo ha già si scrive tra i
vincoli, come fatto.

## Dove si scrive e come arriva a solid-space

1. **In Tecnigrafo**, una issue con l'etichetta `richiesta-solid-space` e i sette
   campi qui sopra, dal modello *Richiesta a solid-space*
   (`.github/ISSUE_TEMPLATE/richiesta-solid-space.md`). La apre il responsabile, anche partendo dal testo che un
   agente ha scritto nella issue di un pacchetto.
2. **In solid-space**, il responsabile ricopia la richiesta in una issue con
   l'etichetta `richiesta` e scrive in "Da dove nasce" il riferimento completo
   `Matleyx78/tecnigrafo#N`. GitHub mostra allora il collegamento anche nella
   issue di Tecnigrafo.
3. **Di nuovo in Tecnigrafo**, il responsabile aggiunge nella prima riga della
   issue il riferimento `Matleyx78/solid-space#M`: è quello che il piano e gli
   agenti leggono, e che si cita in "Dipende da".

Chi lavora in Tecnigrafo (persone o agenti) non scrive in solid-space: la
richiesta resta un testo finché il responsabile non la apre là.

## Che cosa succede dopo

1. solid-space scrive un ADR di valutazione e sceglie uno di quattro esiti: OCCT
   basta così com'è, OCCT va corretto o migliorato, serve un pezzo nostro, oppure
   il bisogno non spetta a solid-space (e la richiesta si chiude con la
   spiegazione). Se la richiesta chiede più di quanto si possa fare, la
   valutazione dice quali obiettivi raggiungibili adotta e che cosa resta fuori.
2. Con i primi tre nasce una tappa di solid-space. L'ordine lo dà il piano di
   Tecnigrafo: si lavora su solid-space e poi si torna. Nel frattempo Tecnigrafo
   lavora sulle parti che non ne dipendono.
3. A tappa chiusa il responsabile controlla il «quando è soddisfatta», aggiorna
   la versione di solid-space fissata in Tecnigrafo (ADR 0030, punto 9) e chiude
   le due issue.

## Esempio (inventato)

> **Richiesta: decidere in modo coerente da che parte sta un punto rispetto a un
> segmento**
>
> **Da dove nasce:** tappa 0.7 di Tecnigrafo (comandi di modifica), issue #… sul
> taglio di entità.
>
> **Il bisogno:** il comando di taglio decide se due entità si intersecano e da
> che parte di un segmento sta il pezzo da tenere. Con punti quasi allineati le
> due decisioni si contraddicono: una dice che i segmenti si attraversano,
> l'altra che stanno dalla stessa parte, e il taglio tiene il pezzo sbagliato o
> dice che non c'è nulla da tagliare. Inoltre lo stesso disegno dà risultati
> diversi su Windows, Linux e macOS.
>
> **Casi concreti:** disegno `taglio-01.dxf`, entità 12 e 15; tre punti
> allineati a meno di 1e-12.
>
> **Quando è soddisfatta:** il taglio tiene sempre il pezzo giusto, anche su
> punti quasi allineati, e la CI dà lo stesso risultato sui tre sistemi
> operativi.
>
> **Vincoli:** nessuna dipendenza dal 3D né da OCCT; compila su Windows, Linux e
> macOS.
>
> **Che cosa blocca:** la tappa 0.7 non può chiudere i comandi di taglio e
> prolungamento.
