# [0.6-P8] Oracolo ezdxf: layer, tipi di linea, proprietà, colori e tipi ISO

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #234
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura
- **Dipende da:** P5
- **Modulo:** `tests/verifica/oracolo`
- **Da leggere:** [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md) (parte
  sull'oracolo), [ADR 0024](../../adr/0024-colori.md) punto 3,
  [ADR 0029](../../adr/0029-verifica-dei-tipi-di-linea-iso.md);
  `docs/verifica/domande.md` (sezioni pubblicate da P5);
  `tests/verifica/oracolo/README.md`, `ezdxf_attesi.py`, `controlla_casi.py`

## File

- `tests/verifica/oracolo/ezdxf_attesi.py`
- `tests/verifica/oracolo/controlla_casi.py`
- `tests/verifica/oracolo/README.md`
- `.github/workflows/ci.yml` solo se il passo deve cambiare (lo approva il
  responsabile)

## Cosa fare

1. **Risposte dei file pubblici:** per ogni file, oltre a ciò che stampa
   oggi, `? layers D -> …`, `? linetypes D -> …`, e le proprietà nelle
   risposte `? entity`, nel formato pubblicato da P5 (`layer=`, `color=`,
   `linetype=`, `ltscale=`, `lineweight=` solo se diversi dal predefinito).
2. **Risposte standard**, una volta per esecuzione, in una sezione
   `=== ezdxf: standard ===`:
   - `? aci <n> -> r,g,b` per n da 1 a 255, dalla tabella di ezdxf.

   Le definizioni di `ACAD_ISO02W100` e `ACAD_ISO10W100` **non** vengono dai
   tipi standard di ezdxf, che non le ha (ADR 0029): sono quelle del DXF di
   AutoCAD `autocad-colori-layer-tipilinea`, e il log le stampa con
   `? linetypes D` del punto 1, senza altro lavoro. La sezione standard non
   le contiene.
3. **Controllo dei casi dichiarati "da ezdxf":** `controlla_casi.py`
   confronta anche `layers`, `linetypes`, `aci` e le proprietà di `entity`.
4. Aggiorna l'autoverifica (`--self-test`) con un DXF scritto a mano che ha
   due layer, un tipo di linea, un colore RGB e uno spessore.

## Regole della tappa che valgono qui

- ezdxf gira solo in CI, con versione e hash fissati; nessun agente la
  installa (ADR 0020). Non cambiare la versione.
- Chi scrive i riferimenti copia i valori dal log di questo passo.

## Test richiesti

- L'autoverifica dell'oracolo copre ogni risposta nuova.

## Fatto quando

- Il passo `Oracolo ezdxf: answers` della CI stampa le risposte nuove per tutti
  i file pubblici e la sezione standard (solo `? aci`). Il log della fonte
  `autocad-colori-layer-tipilinea` mostra i tipi ISO nella sua riga
  `linetypes:`.
- La CI è verde.
