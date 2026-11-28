# [X.Y-PTN] Titolo

<!--
Modello della scheda di un pacchetto, dalla tappa 0.5 (vedi 0000-modello.md,
sezione 6). Dalla 0.8: PT/RT/CT in Tecnigrafo, PS/RS/CS in solid-space;
fino alla 0.7.5 restano P/R/C, senza rinominare nulla. Rami di esempio:
pt0.8-05-… e rt0.8-05-…. La scheda sta in docs/tappe/X.Y/PTN-<nome>.md
e la issue la cita.

La scheda è autosufficiente: l'agente legge AGENTS.md, questa scheda, gli ADR
che cita e CODING_STYLE.md, e nient'altro della tappa. Quello che gli serve
del piano (regole sui testi, file comuni, decisioni) si ripete qui, in breve.
Le regole per la dimensione sono in LAVORO-IN-PARALLELO.md, "Preparare il piano":
la misura conta il codice (al massimo una quindicina di file e circa 600 righe;
test, documenti e casi di riferimento non contano).
-->

- **Tappa:** X.Y — [piano](../X.Y-titolo.md)
- **Issue:** #…
- **Difficoltà:** facile | media | impegnativa
- **Agente:** claude | codex
- **Modello e impegno:** leggero | medio | forte; impegno basso | medio | alto
  (tabella in LAVORO-IN-PARALLELO.md)
- **Riferimenti:** issue `[X.Y-RTN]` (#…), agente … | non servono: motivo
- **Dipende da:** —
- **Modulo:** uno solo (più i suoi test e le righe dei file comuni)
- **Da leggere:** gli ADR e i file di codice che servono davvero, con il motivo;
  se il pacchetto nasce dalla divisione di un altro, le schede sorelle

## File

L'elenco completo dei file da creare o modificare. Al massimo una quindicina
di file di codice, test e documenti esclusi; se ne servono di più, il pacchetto
va diviso.

## Cosa fare

Il lavoro, con le interfacce da rispettare (nomi, firme, comportamenti) scritte
per intero.

## Regole della tappa che valgono qui

Solo quelle che riguardano questo pacchetto: testi e messaggi, file comuni,
decisioni del piano. Se non ce ne sono, si scrive "nessuna".

## Test richiesti

I casi minimi che devono esistere.

## Fatto quando

Condizioni verificabili per considerare il pacchetto concluso. Ognuna deve avere
almeno un caso di verifica nel pacchetto di riferimenti.

## Prova a secco dei riferimenti

Solo per le schede con riferimenti; la scrive chi scrive il piano, insieme alla
scheda (LAVORO-IN-PARALLELO.md, "Preparare il piano"). Uno script e due o tre
domande (o le righe del rapporto da controllare), **senza i valori attesi**:
non va in `tests/verifica/casi/`, chi scrive i riferimenti parte da qui. Se
servono più di qualche riga, il pacchetto è troppo grosso e va diviso.

```text
# script.tgs
…
# domande.tgq
…
```

- **Strumenti:** si scrive con le direttive e le domande di `main` (o
  dell'onda 0: quale pacchetto). Se una domanda chiede all'oracolo ezdxf un
  campo che `tests/verifica/oracolo/ezdxf_attesi.py` impara solo con questo
  pacchetto, la scheda sceglie qui una strada: **A**, il campo dell'oracolo
  entra prima del RT (nell'onda 0 o in un pacchetto in testa); **B**, il caso
  non si dichiara `; oracle: ezdxf` finché questo pacchetto non è unito, e la
  scheda prevede la piccola pull request di soli riferimenti che poi rimette
  la dichiarazione. L'oracolo controlla anche i casi in attesa, quindi senza
  una delle due il caso è rosso subito (RT23, #1028).
- **Dipendenze:** ogni comando o entità che usa viene da `main` o dai pacchetti
  in "Dipende da".
- **Risposte condivise:** questo pacchetto cambia una risposta di una fonte di
  verifica (oracolo ezdxf, rapporto, catalogo delle domande, archivio dei casi
  di solid-space)? Se sì, quali casi già uniti tocca e come restano verdi fino
  all'ultimo pacchetto che ne dipende; altrimenti «nessuna». Un campo nuovo, o
  il gruppo DXF da cui nasce, si cerca in **tutti** i casi già uniti: i DXF di
  `dxf-test-file` e anche i `dati.dxf` scritti a mano in
  `tests/verifica/casi/` (PT15, #951). Per ogni caso che cambia, la scheda del
  RT dice di correggerlo, con `; cambia con #<PT>` e la riga nel suo file di
  `in-attesa/`.
- **Regole già scritte:** quali documenti la risposta deve rispettare oltre
  alla fonte (`docs/verifica/rapporto.md`, il PT gemello, regole delle tappe
  precedenti, funzioni di `core` già esistenti). Chi scrive i riferimenti li
  controlla uno per uno ([agenti/riferimenti.md](../agenti/riferimenti.md)).
