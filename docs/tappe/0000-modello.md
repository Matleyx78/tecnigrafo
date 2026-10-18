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

Moduli coinvolti, nuove classi principali, ADR da scrivere, rischi.

## 5. Prerequisiti

Tappe chiuse e ADR accettati da cui questa dipende.

## 6. Pacchetti di lavoro

Ogni pacchetto è **una pull request**. I pacchetti senza dipendenze tra loro si
possono fare in parallelo; due pacchetti che toccano gli stessi file sono legati
da "Dipende da".

Un pacchetto che cambia il comportamento del programma ha anche un pacchetto di
riferimenti `RN`, scritto da un altro agente e unito prima del lavoro
(ADR 0011). La sua issue si cita in "Dipende da".

### PX — Titolo

- **Issue:** #…
- **Difficoltà:** facile | media | impegnativa
- **Agente:** claude | codex | …
- **Riferimenti:** issue `[X.Y-RN]` (#…), agente gemini | non servono: motivo
- **Dipende da:** —
- **Moduli:** core | render | app | …
- **File:** elenco dei file da creare o modificare
- **Cosa fare:** descrizione precisa del lavoro, con le interfacce da rispettare
- **Test richiesti:** i casi minimi che devono esistere
- **Fatto quando:** condizioni verificabili per considerare il pacchetto concluso;
  ognuna deve avere almeno un caso di verifica nel pacchetto di riferimenti

## 7. Prova manuale

Passi da eseguire a mano su ogni sistema operativo prima della chiusura.

## 8. Modifiche al piano

Cambiamenti decisi durante la tappa, con data e motivo.
