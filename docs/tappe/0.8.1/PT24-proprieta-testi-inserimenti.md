# [0.8.1-PT24] `core`: proprietà di `MTEXT`, `ATTDEF` e `INSERT`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT24]` (#…), agente codex
- **Dipende da:** PT15, PT22, RT24
- **Modulo:** `core`
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punti 2 e
  6; [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md), punto 2;
  [ADR 0056](../../adr/0056-comandi-degli-attributi.md), punto 1; le schede
  sorelle [PT22](PT22-proprieta-core.md) e [PT23](PT23-proprieta-retini-quote.md)

## File

- `core/src/entity_properties_texts.cpp`
- `tests/core/entity_properties_test.cpp`

## Cosa fare

1. **`MTEXT`** (`value`, `style`, `attachment`, `width`, `height`, `rotation`,
   `line-spacing`, `line-spacing-style`): lo stile deve esistere; `width` ≥ 0;
   `height` > 0; `attachment` uno dei nove nomi del rapporto.
2. **`ATTDEF`** (`tag`, `prompt`, `default`, i cinque modi, `height`,
   `rotation`, `style`): `tag` con `validAttributeTag`; `lock-position` è il
   campo di PT15.
3. **`INSERT`** (`block`, `insertion`, `scale`, `rotation`, e un
   `attribute="<TAG>":"<valore>"` per attributo non costante): `block` sostituisce
   la definizione con un'altra che esiste, tenendo punto, scale, rotazione e
   gli `ATTRIB`, come AutoCAD; un nome che non esiste o che darebbe un ciclo è
   rifiutato. **Da registrare** prima del pacchetto, nella seduta in AutoCAD:
   che cosa fa il palette degli attributi quando il blocco nuovo ne ha altri.
   Il valore di un attributo cambia solo il valore.
4. Ogni cambiamento è valido come l'entità che produce (`supported`), altrimenti
   `Invalid value.`.

## Regole della tappa che valgono qui

- Angoli digitati da `reducedDegreesToRadians`
  ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 2).

## Test richiesti

- Ogni proprietà dei tre tipi, un valore rifiutato per ciascuno, un `INSERT`
  con il blocco sostituito e i suoi `ATTRIB` tenuti.

## Fatto quando

- I casi di RT24 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/proprieta-proprie/testi-inserimenti/domande.tgq
M = MTEXT insertion=0,0 height=2.5 width=60 attachment=top-left rotation=0 style="Standard" value="uno\\Pdue"
? properties M -> …
? set-property M width=0 -> …
A = ATTDEF position=0,0 alignment-point=0,0 height=2.5 rotation=0 width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="K1" tag="CODICE" prompt="Codice"
? set-property A tag="CODICE 2" -> invalid
!block "B1" base=0,0
L1 = LINE start=0,0 end=10,0
!endblock
!block "B2" base=0,0
C2 = CIRCLE center=0,0 radius=5
!endblock
I = INSERT block="B1" insertion=10,10 scale=1,1 rotation=0
? set-property I block="B2" -> …
? set-property I block="NONESISTE" -> invalid
```

- **Strumenti:** `properties`, `set-property` dell'onda 0 (PT2); `!block` e
  `!endblock` su `main`.
- **Dipendenze:** PT15 (il modo `lock-position`).
- **Fonte:** la seduta «da registrare» del punto 3.
- **Risposte condivise:** nessuna.
