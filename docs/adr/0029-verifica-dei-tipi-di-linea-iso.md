# ADR 0029: Verifica dei tipi di linea ISO con un DXF di AutoCAD

- **Stato:** Proposto
- **Sostituisce in parte:** ADR 0025, punto 2, solo la frase sulla verifica delle
  definizioni con ezdxf
- **Data:** 2026-10-26

## Contesto

L'ADR 0025 (punto 2) fissa i nomi `ACAD_ISO02W100` e `ACAD_ISO10W100` per i due
tipi di linea che Tecnigrafo offre, e dice che le loro definizioni non le sceglie
Tecnigrafo: sono quelle che quei nomi hanno nei DXF degli altri programmi. Per
verificarle indica le definizioni standard di ezdxf, stampate nel log
dell'oracolo (ADR 0020), come per la tabella dei colori (ADR 0024, punto 3).

Il pacchetto 0.6-P8 (#234) ha trovato che il passo non regge. Nel sorgente di
ezdxf 1.4.5 (`ezdxf/tools/standards.py`) il catalogo dei tipi di linea è quello
ANSI: non ha `ACAD_ISO02W100` né `ACAD_ISO10W100`, e `linetypes(scale=...)`
restituisce solo quel catalogo con le lunghezze scalate. La scheda di P8
prevedeva proprio questo caso: fermarsi e rivedere la decisione. Codex si è
fermato, dopo aver letto il sorgente senza eseguirlo (commento su #234).

La fonte che serve esiste già. `tests/verifica/sorgenti.json` elenca
`autocad-colori-layer-tipilinea`, un DXF salvato da AutoCAD dal responsabile,
ripulito dei dati sensibili con lo script di pulizia e pubblicato con licenza
CC0 nel repository `Matleyx78/dxf-test-file`. Contiene nella tabella `LTYPE`
`ACAD_ISO02W100`, `ACAD_ISO05W100` e `ACAD_ISO14W100`. Il responsabile vi ha
aggiunto `ACAD_ISO10W100`, salvando di nuovo da AutoCAD con il tipo caricato.

## Decisione

1. **Le definizioni dei due tipi (descrizione e motivo) sono quelle che AutoCAD
   scrive nella tabella `LTYPE` di un DXF.** La fonte è la voce
   `autocad-colori-layer-tipilinea` di `tests/verifica/sorgenti.json`, letta con
   ezdxf: le righe di `? linetypes D` nel log dell'oracolo (ADR 0020,
   pacchetto 0.6-P8). Il file deve contenere entrambi i nomi; un cambiamento del
   file passa da una pull request di riferimenti che aggiorna indirizzo,
   versione e hash.
2. **ezdxf non è la fonte di queste definizioni.** Resta il lettore dei DXF e la
   fonte della tabella dei colori (ADR 0024, punto 3). Il passo dell'oracolo non
   stampa definizioni ISO nella sezione `=== ezdxf: standard ===`, che contiene
   solo le risposte `? aci`.
3. **Se i valori letti non coincidono con quelli di `core`** (scheda 0.6-P2,
   `linetype_pattern.cpp`), valgono quelli letti, come per i colori: si
   correggono la descrizione, il motivo e i test in un pacchetto di lavoro.
4. **Il resto del punto 2 dell'ADR 0025 non cambia:** i nomi ISO, la tabella dei
   due tipi, la regola che un DXF aperto porta le sue definizioni e l'assenza dei
   file `.lin` nella 0.6.

## Alternative considerate

- **Un tipo ANSI di ezdxf al posto dei due ISO:** è ciò che ezdxf offre, ma il
  nome sarebbe diverso e un valore assunto da un nome diverso è proprio ciò che
  l'ADR 0025 esclude.
- **Tenere i valori attuali e verificarli solo nella prova manuale di chiusura
  con altri CAD:** i riferimenti dei tipi ISO non avrebbero un'origine
  indipendente, e un errore nostro verrebbe copiato nei test.
- **Copiare `acadiso.lin`:** già scartato dall'ADR 0025; il file è di Autodesk.
- **Sostituire il secondo tipo con `ACAD_ISO05W100` o `ACAD_ISO14W100`,
  già nel file:** cambierebbero la tabella dell'ADR 0025, le schede P2 e P5, i
  test e la tabella di parità, e il tipo a punto e linea della 0.6 non sarebbe
  più quello previsto.

## Conseguenze

- La scheda 0.6-P8 perde le definizioni ISO dalla sezione standard e la clausola
  "se ezdxf non ha il nome, fermati"; il pacchetto può procedere (scheda
  corretta insieme a questo ADR).
- La decisione 2 del piano della tappa 0.6 e la scheda 0.6-P2 dicono
  "verificate con un DXF di AutoCAD letto con ezdxf".
- La voce aggiornata di `sorgenti.json` e i casi che copiano le definizioni dal
  log (R10 per la fonte, R13 per `-LINETYPE Load`) sono pacchetti di
  riferimenti, separati da questa correzione.
- L'ADR 0025 riceve nell'intestazione la riga "Sostituito in parte da".
- L'oracolo dipende da un file che solo il responsabile può produrre con AutoCAD:
  se serve un altro tipo ISO, il file va rigenerato allo stesso modo.
