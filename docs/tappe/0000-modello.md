# Tappa X.Y — Titolo

- **Stato:** in programma | aperta | chiusa
- **Milestone GitHub:** `vX.Y`
- **Apertura:** AAAA-MM-GG — **Chiusura:** AAAA-MM-GG

<!--
Una tappa "in programma" compila solo le sezioni 1-4 (la scheda).
All'apertura il responsabile aggiunge le sezioni 5-8 (il piano dettagliato).
-->

## 1. Obiettivo

Cosa sarà possibile fare alla fine della tappa, in una o due frasi, dal punto di
vista di chi usa il programma.

## 2. Confini

**Dentro:** l'elenco esatto di ciò che la tappa realizza.

**Fuori:** ciò che sembra collegato ma appartiene ad altre tappe (indicare quale).

## 3. Criteri di chiusura

Verificabili, uno per riga, con casella da spuntare alla chiusura.

- [ ] …

## 4. Progetto di massima

Moduli coinvolti, nuove classi principali, ADR da scrivere, rischi, e che cosa
servirà chiedere a solid-space (si dettaglia nella sezione 5).

## 5. Prerequisiti

Tappe chiuse e ADR accettati da cui questa dipende.

### Richieste a solid-space

I bisogni geometrici che spettano al kernel ([ADR 0030](../adr/0030-solid-space-kernel-di-tecnigrafo.md),
punto 2), cercati mentre si prepara il piano e aperti in solid-space con le
regole di [RICHIESTE-A-SOLID-SPACE.md](../RICHIESTE-A-SOLID-SPACE.md). Qui c'è
solo il collegamento: il testo della richiesta sta nella issue di solid-space, e
il suo stato si legge nel Project «Tecnigrafo». Un bisogno emerso durante la
tappa si aggiunge qui e si annota in "Modifiche al piano". Se non ne servono, si
scrive "nessuna" e perché.

| Richiesta | Pacchetti che ne dipendono |
|---|---|
| [Matleyx78/solid-space#…](https://github.com/Matleyx78/solid-space/issues/…) | P… |

Un piano con richieste si scrive **in due tempi** (ADR 0030, punto 13):

1. subito, le schede dei pacchetti che non dipendono da solid-space; i pacchetti
   che ne dipendono hanno una riga nella tabella, con «scheda dopo il contratto»
   al posto del link, e stanno dopo gli altri;
2. dopo che solid-space ha accettato le valutazioni con il loro contratto, le
   loro schede, scritte sulle intestazioni accettate. Ogni scheda dice quali
   intestazioni usa e, se il pacchetto sposta il sottomodulo, a quale pacchetto
   di solid-space (ADR 0030, punto 9).

In "Dipende da" questi pacchetti citano la richiesta (`Matleyx78/solid-space#N`)
finché la tappa di solid-space non ha i suoi pacchetti, poi il pacchetto di
solid-space che serve. I loro riferimenti prendono i valori geometrici
dall'archivio dei casi di solid-space, citando il caso (ADR 0030, punto 14), e
verificano facciata e comandi.

## 6. Pacchetti di lavoro

Ogni pacchetto è **una pull request**. I pacchetti senza dipendenze tra loro si
possono fare in parallelo; due pacchetti che toccano gli stessi file sono legati
da "Dipende da" ([LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md),
"Preparare il piano").

Un pacchetto che cambia il comportamento del programma ha anche un pacchetto di
riferimenti `RN`, scritto da un altro agente e unito prima del lavoro
(ADR 0011). La sua issue si cita in "Dipende da".

Dalla tappa 0.5 questa sezione contiene **solo la tabella**: ogni pacchetto ha
una sua **scheda** in `docs/tappe/X.Y/PN-<nome>.md` (modello in
[0000-pacchetto.md](0000-pacchetto.md)), e la sua issue la cita. Così un agente
legge la scheda del suo pacchetto e non tutto il piano. Le regole per dividere
il lavoro sono in [LAVORO-IN-PARALLELO.md](../LAVORO-IN-PARALLELO.md), sezione
"Preparare il piano".

| Pacchetto | Scheda | Issue | Dipende da | Riferimenti | Difficoltà | Agente | Modello | Impegno |
|---|---|---|---|---|---|---|---|---|
| P1 Titolo | [P1](X.Y/P1-nome.md) | #… | — | R1 (gemini) / non servono: motivo | facile / media / impegnativa | claude / codex / gemini | leggero / medio / forte | basso / medio / alto |

L'**ordine delle righe** è l'ordine in cui gli agenti prendono "il prossimo
pacchetto" (AGENTS.md). Le issue di riferimenti `[X.Y-RN]` hanno una riga
ciascuna, prima del loro pacchetto di lavoro.

La tabella la legge `scripts/apri-tappa.sh`, che crea le issue all'apertura
(GUIDA-RESPONSABILE.md, B6), quindi segue queste regole: le colonne con questi
nomi; la prima cella comincia con `PN` o `RN` seguito dal titolo della issue; la
scheda è un link a un file che esiste, oppure un testo (per esempio «scheda dopo
il contratto») se non c'è ancora; la colonna Agente contiene solo il nome
dell'etichetta `agente:…` (`claude`, `codex`, `gemini`), mai il
modello tra parentesi, che si dice nella colonna Modello; "Dipende da" elenca,
separati da virgole, pacchetti delle righe precedenti (`P3`, `R3`), issue di
Tecnigrafo (`#12`) o di altri repository (`Matleyx78/solid-space#20`), oppure
`—`; la colonna Issue resta `#…` finché lo script non ci scrive il numero.

**File comuni.** I file che più pacchetti toccano nello stesso periodo (di norma
i `CMakeLists.txt` e i registri come `builtin_commands.cpp`): ognuno aggiunge
solo le sue righe, in ordine alfabetico, e chi arriva dopo si aggiorna. Si
elencano qui e si ripetono nelle schede che li toccano.

## 7. Prova manuale

Passi da eseguire a mano su ogni sistema operativo prima della chiusura.

## 8. Modifiche al piano

Cambiamenti decisi durante la tappa, con data e motivo.
