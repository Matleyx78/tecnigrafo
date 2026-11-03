# Lavoro che usa solid-space

Da leggere quando la scheda cita intestazioni `solid_space/…`, quando tocchi
`third_party/solid-space` o file che le includono, o quando usi
`scripts/richiesta.sh` ([AGENTS.md](../../AGENTS.md), "Se ... leggi anche ...").
Vale insieme al nucleo. Leggere e commentare in solid-space:
[altro-repository.md](altro-repository.md).

## Di solid-space usa solo il contratto

Di solid-space usa solo ciò che la scheda cita: le intestazioni accettate come
contratto, nella versione fissata dal sottomodulo. Una funzione dichiarata ma
non ancora realizzata non si usa finché il suo pacchetto di solid-space non è
unito. Il sottomodulo lo sposti solo se la scheda lo dice, al commit che indica
([ADR 0030](../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 9).
Un esempio della regola del nucleo su dipendenze, `cmake/` e `.github/` («salvo
che la scheda lo dica esplicitamente») è il pacchetto che porta il sottomodulo di
solid-space (ADR 0030, punto 8). Come tenere il sottomodulo al commit giusto dopo
il clone: [comandi.md](comandi.md).

## Se serve qualcosa a solid-space

Le decisioni geometriche robuste, le tolleranze geometriche, i vincoli, il naming
e i solidi spettano a solid-space, il kernel di Tecnigrafo
([ADR 0030](../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 2). Se il
pacchetto ti porta a scrivere qualcosa di questo genere che la scheda non affida a
solid-space né a una funzione che esiste già (per esempio un test di orientamento
con una soglia tua, o un calcolo di tangenza), **non scriverlo**:

1. scrivi nella issue del pacchetto il testo di una richiesta, con i sette campi
   di [docs/RICHIESTE-A-SOLID-SPACE.md](../RICHIESTE-A-SOLID-SPACE.md), nella
   forma del file che legge `scripts/richiesta.sh` (esempio in fondo a quel
   documento) e con i riferimenti in forma assoluta; dì che cosa ti serve, non
   come farlo;
2. scrivi anche che cosa del pacchetto è già fatto e che cosa dipende da quel
   bisogno, e non continuare da solo: come cambia il piano lo decide il
   responsabile;
3. se hai lavoro non pubblicato, pubblica il ramo **senza** pull request e scrivi
   nel commento il nome del ramo;
4. togli `AA-Presa in carico` e termina la sessione.

Non aprire la richiesta di tua iniziativa: la apre in solid-space il
responsabile, o un agente a cui lui lo chiede ([AGENTS.md](../../AGENTS.md),
"Dove puoi scrivere").

## Un buco nel contratto

**Un buco nel contratto ferma anche i riferimenti.** Prima di scrivere codice
o casi, rileggi le intestazioni che usi, commenti compresi, con la lista di
[docs/CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md).
Se intestazione, ADR e scheda non determinano una risposta, o si
contraddicono, **fermati prima di scrivere codice o casi**: non scegliere una
risposta, nemmeno segnalandola. Nella issue del tuo pacchetto scrivi il testo
della issue da aprire in solid-space con l'etichetta `contratto`: titolo che
nomini l'intestazione e la funzione, che cosa manca, un esempio con i dati e
le scelte possibili, con riferimenti assoluti. È un buco nel contratto, non una
nuova richiesta con i sette campi. Poi segui i punti 2–4 sopra, anche se il
lavoro è già avanzato. La issue `contratto` la apre il responsabile e la
risposta la decide lui; il pacchetto si riprende dopo la decisione.

Se il contratto determina la risposta ma la scheda no, il buco è nella scheda:
[pacchetto.md](pacchetto.md), "Un buco nella scheda".

## Prima della pull request, se sposti il sottomodulo

Era il punto 1 di "Quando fermarsi". Prima di aprire la pull request, se il
pacchetto sposta il sottomodulo di solid-space, controlla il suo
`docs/DECISIONI.md` al commit di destinazione: le intestazioni che usi sono
cambiate dopo il commit su cui è stata scritta la scheda? Confronta quelle
versioni delle intestazioni, commenti compresi. Se sono cambiate, leggi le
decisioni e adegua il pacchetto; se il contratto non determina la risposta o
contraddice la scheda, fermati come dice "Se serve qualcosa a solid-space",
senza scegliere tu.
