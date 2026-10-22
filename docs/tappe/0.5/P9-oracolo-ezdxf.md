# [0.5-P9] Oracolo ezdxf in CI

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #162
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura della verifica
- **Dipende da:** P7, P8
- **Modulo:** infrastruttura (`tests/verifica/oracolo/`, CI)
- **Da leggere:** [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md) punti 2
  e 3, [ADR 0011](../../adr/0011-modalita-di-controllo.md) punto 6;
  `docs/verifica/domande.md` (sezione "File", da P8) e `docs/verifica/rapporto.md`
  ("Entità"), `.github/workflows/ci.yml` (job `Verifica`),
  `cmake/TecnigrafoVerifica.cmake` (dove finiscono i file scaricati)

## File

- `tests/verifica/oracolo/requirements.txt` (nuovo: ezdxf con versione e hash,
  e le sue dipendenze)
- `tests/verifica/oracolo/ezdxf_attesi.py` (nuovo)
- `tests/verifica/oracolo/controlla_casi.py` (nuovo)
- `tests/verifica/oracolo/README.md` (nuovo)
- `.github/workflows/ci.yml` (passo `Oracolo ezdxf` nel job `Verifica`)

La modifica a `.github/` la approva il responsabile.

## Cosa fare

### `ezdxf_attesi.py <file.dxf> [--name D]`

Legge il DXF con ezdxf e stampa, nella sintassi delle domande al modello
(`docs/verifica/domande.md`, sezione "File"), le risposte di ezdxf:

- `? version D -> …`, `? units D -> …` (il valore di `$INSUNITS`, o 0 se il
  file non lo ha: è il valore predefinito del DXF);
- `? count D -> …` e `? unhandled D -> …` con le regole dell'ADR 0018,
  punto 3 (tipi gestiti, spazio modello, estrusione, entità degeneri);
- per ogni entità gestita: `? entity D <handle> -> <TIPO> …` nel formato
  del rapporto, con i valori **come stanno nel file** (per le entità con
  estrusione `(0,0,-1)` lo script non converte: stampa un commento
  `; OCS: convert by hand (ADR 0018, point 5)` e i valori grezzi);
- i numeri con la rappresentazione più breve che si rilegge uguale (come il
  rapporto), gli angoli in gradi come nel file.

Le regole che lo script applica sono **scritte** nell'ADR e nelle specifiche;
lo script non guarda il codice di Tecnigrafo e non lo esegue.

### `controlla_casi.py <cartella dei casi> <cartella dei file scaricati>`

- Per ogni file di `sorgenti.json` stampa nel log, con un'intestazione, le
  risposte di `ezdxf_attesi.py`: da qui chi scrive i riferimenti le copia
  (ADR 0020).
- Per ogni caso il cui `domande.tgq` comincia con `; oracle: ezdxf`, rifà le
  risposte con ezdxf per ogni `D = DXF source=…` e controlla che le domande
  del caso diano le stesse risposte, entro le tolleranze del caso. Una
  domanda che ezdxf non sa dare (per esempio `roundtrip`) si salta.
- Passa `ezdxf audit` su ogni `.dxf` delle cartelle di uscita dei casi
  (`build/ci/tests/verifica/casi/`) e su `drawing.dxf` estratto da ogni
  `.cadx` che vi trova; un errore di audit fa fallire il passo.
- Esce con un codice diverso da 0 se un controllo fallisce, con un messaggio
  che dice caso, domanda, valore atteso e valore di ezdxf.

### Passo della CI

Nel job `Verifica`, dopo il passo `Verifica`:

```yaml
- name: Oracolo ezdxf
  run: |
    python3 -m pip install --require-hashes -r tests/verifica/oracolo/requirements.txt
    python3 tests/verifica/oracolo/controlla_casi.py tests/verifica/casi build/verifica-sorgenti
```

con `if: always()` solo per la stampa delle risposte, così chi scrive i
riferimenti le vede anche quando un caso è rosso.

### `README.md`

A cosa serve l'oracolo, come leggerne le risposte nel log della CI, come
dichiarare un caso, come aggiornare ezdxf (pacchetto dedicato che ricontrolla
tutti i casi).

## Regole della tappa che valgono qui

- ezdxf non si collega mai a Tecnigrafo e non entra nel programma (ADR 0011).
- Nessun file di `tests/verifica/casi/` né `sorgenti.json`: li aggiungono i
  pacchetti di riferimenti.

## Test richiesti

- La pull request stessa: il passo gira e passa (oggi non ci sono casi
  dichiarati né file pubblici: deve passare senza fare niente, dicendolo).
- Nella pull request: l'uscita di `ezdxf_attesi.py` su due DXF di prova
  scritti a mano (uno con un'entità non gestita, uno con estrusione
  `(0,0,-1)`), allegata come testo.

## Fatto quando

- Il passo `Oracolo ezdxf` installa ezdxf con hash fissati, stampa le
  risposte per i file pubblici, controlla i casi dichiarati e fa `audit` dei
  DXF scritti.
- Senza casi dichiarati e senza file pubblici passa.
- La CI è verde.
