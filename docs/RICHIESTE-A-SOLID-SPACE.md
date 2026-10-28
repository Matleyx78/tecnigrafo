# Come chiedere qualcosa a solid-space

[solid-space](https://github.com/Matleyx78/solid-space) è il kernel geometrico di
Tecnigrafo: predicati geometrici robusti, solver dei vincoli, naming topologico
stabile e solidi 3D su OCCT. Cresce **solo su richiesta di Tecnigrafo**: non ha un
piano di tappe scritto in anticipo. Quando, lavorando a una tappa di Tecnigrafo,
serve un comportamento geometrico che non c'è, si scrive una richiesta.

La fonte delle regole è in solid-space (modello:
[docs/richieste/0000-modello.md](https://github.com/Matleyx78/solid-space/blob/main/docs/richieste/0000-modello.md),
decisione:
[ADR 0007](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md)).
Questo file ne è un riassunto: se i due divergono, vale solid-space.

## Quando si scrive una richiesta

- Serve un calcolo o un'operazione geometrica (decidere un orientamento, risolvere
  un vincolo, riconoscere una faccia dopo una modifica, fare una booleana) e
  Tecnigrafo non riesce a farlo, o lo fa male.
- **Non** si scrive per ciò che è di Tecnigrafo: comandi, interfaccia, annulla e
  ripeti, salvataggio nel `.cadx`. Quelli restano qui.

## Come si scrive

Una richiesta dice **di che cosa c'è bisogno, non come soddisfarlo**. Non si scrive
«usiamo OCCT» né «scrivete un algoritmo»: se basta OCCT, se va corretto, se serve un
pezzo nuovo o se il bisogno non spetta a solid-space lo decide solid-space.

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

## A chi arriva

La richiesta si scrive qui, in Tecnigrafo, e la passa **il responsabile**, che apre
in solid-space una issue con etichetta `richiesta`. Chi lavora in Tecnigrafo
(persone o agenti) non scrive in solid-space: la richiesta resta un testo finché il
responsabile non la apre là.

## Che cosa succede dopo

1. solid-space scrive un ADR di valutazione e sceglie uno di quattro esiti: OCCT
   basta così com'è, OCCT va corretto o migliorato, serve un pezzo nostro, oppure
   il bisogno non spetta a solid-space (e la richiesta si chiude con la
   spiegazione).
2. Con i primi tre nasce una tappa di solid-space, una alla volta. Una richiesta
   che arriva mentre un'altra tappa è aperta aspetta in coda: nel frattempo
   Tecnigrafo lavora sulle parti che non ne dipendono.
3. A tappa chiusa il responsabile controlla il «quando è soddisfatta» e chiude la
   richiesta.

Per questo conviene scrivere la richiesta **appena si vede il bisogno**, non quando
la tappa di Tecnigrafo è ferma ad aspettare.

## Esempio (inventato)

> **Richiesta: decidere in modo affidabile da che parte sta un punto rispetto a un
> segmento**
>
> **Da dove nasce:** tappa 0.7 di Tecnigrafo (comandi di modifica), issue #… sul
> taglio di entità.
>
> **Il bisogno:** il comando di taglio sceglie la parte da tenere in base a da che
> parte sta un punto rispetto a un segmento. Con punti quasi allineati il
> risultato cambia tra una esecuzione e l'altra dei calcoli sullo stesso disegno.
>
> **Casi concreti:** disegno `taglio-01.dxf`, entità 12 e 15; tre punti allineati a
> meno di 1e-12.
>
> **Quando è soddisfatta:** lo stesso disegno dà sempre lo stesso risultato di
> taglio, anche su punti quasi allineati.
>
> **Vincoli:** nessuna dipendenza dal 3D né da OCCT; compila su Windows, Linux e
> macOS.
>
> **Che cosa blocca:** la tappa 0.7 non può chiudere i comandi di taglio e
> prolungamento.
