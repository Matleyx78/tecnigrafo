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
- `tests/core/entity_properties_test.cpp`, `tests/io/dxf_insert_test.cpp`
  (salvataggio e riapertura di `scale-z`)

## Cosa fare

1. **`MTEXT`** (`value`, `style`, `attachment`, `width`, `height`, `rotation`,
   `line-spacing`, `line-spacing-style`): lo stile deve esistere; `width` ≥ 0;
   `height` > 0; `attachment` uno dei nove nomi del rapporto; `line-spacing`
   finito e positivo, come lo accetta il comando `MTEXT` (verbale della
   revisione delle interfacce PT4, #927, [decisione] 8, applicazione dell'ADR
   0036, punto 6): `0` dà `Invalid value.`, anche se oggi `supported(MText)`
   vale sempre.
2. **`ATTDEF`** (`tag`, `prompt`, `default`, i cinque modi, `height`,
   `rotation`, `style`): `tag` con `validAttributeTag`; `lock-position` è il
   campo di PT15.
3. **`INSERT`** (`block`, `insertion`, `scale`, `scale-z`, `rotation`, e un
   `attribute="<TAG>":"<valore>"` per attributo non costante, nell'ordine
   degli `ATTRIB` dell'inserimento, come il palette di AutoCAD): `block`
   sostituisce la definizione con un'altra che esiste, tenendo punto, scale,
   rotazione e gli `ATTRIB` così come sono, anche se il blocco nuovo ha altre
   etichette (per allinearli c'è `ATTSYNC`); un nome che non esiste o che
   darebbe un ciclo è rifiutato. È una scelta di Tecnigrafo: nel palette di
   AutoCAD 2021 il *Nome* è in grigio, con gli attributi e senza (seduta del
   2026-11-22; [DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Esiti della
   seconda seduta in AutoCAD della 0.8.1», punto 4). Il valore di un attributo
   cambia solo il valore. **Etichette uguali** (il confronto delle etichette è
   senza maiuscole): vale la prima, come in `findAttribute`
   ([DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce
   PT4», P21): `properties` dà un solo `attribute=` per etichetta, quello del
   primo `ATTRIB` con quell'etichetta nell'ordine dell'inserimento, e
   `set-property` cambia quello; gli `ATTRIB` doppi dopo il primo restano
   intatti nel disegno e nel file (#927, [decisione] 8, applicazione della
   regola). **`scale-z`** è il gruppo 43
   dell'`AcDbBlockReference`, che oggi sta negli extras (`-INSERT` lo scrive
   già): si legge da lì (1 se manca) e si cambia negli extras della
   `PropertyChange`, senza toccare gli altri gruppi (verbale della revisione del
   piano, [bloccante] 6); la `PropertyChange` si applica con `Editor::replace`
   con gli extras (PT22). Deve essere finito e diverso da 0, come le scale di
   `-INSERT`, altrimenti `Invalid value.` (#927, [decisione] 8). Il disegno 2D
   non lo usa.
4. Ogni cambiamento è valido come l'entità che produce (`supported`), più i
   controlli del nuovo valore scritti qui sopra, altrimenti `Invalid value.`.
   Si controlla solo il valore che cambia: un DXF già caricato con un gruppo 43
   o 44 anomalo (per esempio 0) resta com'è quando si cambia un altro campo, e
   quel gruppo resta intatto negli extras (ADR 0006; #927, [decisione] 8).

## Regole della tappa che valgono qui

- Angoli digitati da `reducedDegreesToRadians`
  ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 2).

## Test richiesti

- Ogni proprietà dei tre tipi, un valore rifiutato per ciascuno, un `INSERT`
  con il blocco sostituito e i suoi `ATTRIB` tenuti; `scale-z` cambiato con
  `Editor::replace` (PT22), salvato con `io` e riletto: 43 nuovo, gli altri
  gruppi intatti (#927, [bloccante] 6).
- `scale-z=0` e `line-spacing=0` rifiutati; un `INSERT` letto con il 43 a 0
  cambia la rotazione e tiene il 43 a 0.
- Due `ATTRIB` `ID` e `id` nello stesso inserimento: `properties` ne dà uno
  (il primo), `set-property` cambia il primo e lascia il secondo intatto.

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

# casi/proprieta-proprie/scala-z/domande.tgq
D = DXF file=inserimento-z.dxf
? properties D <handle dell'INSERT> -> …
? set-property D <handle dell'INSERT> scale-z=2 -> …
? set-property D <handle dell'INSERT> scale-z=0 -> invalid
```

`inserimento-z.dxf` è un DXF del caso scritto a mano: un blocco e un `INSERT`
con i gruppi 41, 42 e 43 (= 3).

- **Strumenti:** `properties`, `set-property` (anche con `D <handle>`)
  dell'onda 0 (PT2); `!block`, `!endblock` e `D = DXF file=…` su `main`. Niente
  `roundtrip D` dopo `set-property`: la domanda non modifica il documento
  delle domande, quindi proverebbe solo il file com'era; la risposta di
  `set-property` mostra `scale-z=2` (dopo ` | `), e il salvataggio con il 43
  nuovo lo prova il test di unità (#927, [bloccante] 6).
- **Dipendenze:** PT15 (il modo `lock-position`).
- **Fonte:** per l'ordine degli attributi, `autocad-proprieta.dxf` e la sua
  cronologia (seduta del 2026-11-22: `DUE`, `UNO`, l'ordine della
  definizione); il cambio di blocco è una regola nostra, che AutoCAD non ha.
- **Risposte condivise:** nessuna.
