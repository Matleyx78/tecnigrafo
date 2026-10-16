# ADR 0003: Licenza GPLv3 o successiva

- **Stato:** Accettato
- **Data:** 2026-10-15

## Contesto

Il progetto non ha scopi commerciali: il software deve restare libero e
disponibile a tutti, anche nelle versioni modificate da altri.

## Decisione

Tutto il codice di Tecnigrafo è distribuito con licenza **GNU GPL v3.0 o
successiva** (`SPDX-License-Identifier: GPL-3.0-or-later`).

Ogni contributo è firmato con il Developer Certificate of Origin (`git commit -s`).
Non si chiede la cessione del copyright: ogni autore resta titolare del proprio
codice, e la licenza non potrà quindi essere cambiata in una licenza non libera.

Dipendenze ammesse solo con licenze compatibili con la GPLv3: LGPL, GPL, MIT, BSD,
Apache 2.0, MPL 2.0. Qt e Open CASCADE (LGPL) sono compatibili.

## Alternative considerate

- **LGPL o MIT:** permetterebbero a terzi di chiudere versioni derivate, contro lo
  scopo del progetto.

## Conseguenze

- Si può riusare, citando la fonte, codice GPL di altri progetti (per esempio
  LibreCAD e QCAD) e usare librerie GPL come LibreDWG.
- Chi distribuisce Tecnigrafo, anche modificato, deve distribuirne il sorgente.
