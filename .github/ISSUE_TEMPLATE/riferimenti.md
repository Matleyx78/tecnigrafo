---
name: Riferimenti di un pacchetto (solo responsabile)
about: I casi di verifica di un pacchetto della tappa aperta, scritti prima del lavoro (ADR 0011). Li crea il responsabile all'apertura della tappa.
title: "[X.Y-RN] Riferimenti: "
labels: riferimenti
---

**Tappa:** [X.Y](../blob/main/docs/tappe/) — **Riferimenti del pacchetto:** PN (#…)
**Dipende da:** #…

Scrivi i casi di verifica del pacchetto PN, prima che il lavoro cominci. Le regole
sono in [AGENTS.md](../blob/main/AGENTS.md), sezione "Modalità di controllo e
riferimenti", e nell'[ADR 0011](../blob/main/docs/adr/0011-modalita-di-controllo.md).
Fa fede il documento della tappa, sezione del pacchetto PN.

## Cosa verificare

<!-- Le condizioni "Fatto quando" del pacchetto PN: ognuna deve avere almeno un
caso. Il responsabile può aggiungere qui casi o file che vuole vedere. -->

## In breve

- Tocca solo `tests/verifica/casi/`, `tests/verifica/sorgenti.json` e
  `tests/verifica/in-attesa.txt`.
- Valori attesi da calcoli, file pubblici o programmi di riferimento, **mai**
  eseguendo Tecnigrafo.
- File presi dal web: indirizzo, versione, hash SHA-256 e licenza in
  `sorgenti.json`, link alla licenza nella pull request.
- I nuovi casi vanno in `in-attesa.txt` con il numero della issue del lavoro.
