# [0.8-PT15] `cmd`: comando `MTEXT` dalla riga di comando

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT15]` (#…), agente claude
- **Dipende da:** PT12, PT13, RT15
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  punto 2; [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md);
  `cmd/src/commands/text_command.cpp`; la cronologia di `MTEXT` copiata dal
  responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/mtext_command.hpp`,
  `cmd/src/commands/mtext_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults`: tipo e fattore
  dell'interlinea; file comune con PT44)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/mtext_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

`MTEXT`, con le richieste di AutoCAD LT dalla cronologia del responsabile;
quelle attese:

1. `Current text style:  "Standard"  Text height:  2.5  Annotative:  No`
   (riga d'informazione; senza *Annotative* se in AutoCAD LT non c'è);
2. `Specify first corner:`
3. `Specify opposite corner or [Height/Justify/Line spacing/Rotation/Style/Width]:`
   - *Height*: `Specify height <2.5>:`; *Justify*: le nove sigle `TL`, `TC`,
     `TR`, `ML`, `MC`, `MR`, `BL`, `BC`, `BR`; *Line spacing*: tipo (`At least`,
     `Exactly`) e fattore, che **restano** per i `MTEXT` successivi del
     disegno, fra i valori che i comandi ricordano (`CommandDefaults`, ADR
     0014): diventano correnti appena confermati, anche se poi `Esc` non crea
     il testo; a ogni avvio, e in ogni disegno che si apre, *At least* 1.
     Differenza da AutoCAD, che tiene il tipo nel profilo (`TSPACETYPE`,
     comune a tutti i disegni e conservato dopo il riavvio) e il fattore in
     `TSPACEFAC`: qui sono di ogni disegno e ripartono a ogni avvio, come
     `POLARANG` e `TRIMEXTENDMODE` (decisioni del 2026-11-13); *Rotation*:
     `Specify rotation angle <0>:`, che non resta; *Style*:
     nome di uno stile che c'è; *Width*: `Specify width:` (scrivere la larghezza
     salta l'angolo opposto). Ogni opzione torna alla richiesta 3.
4. Le righe del testo, ognuna un paragrafo: `MText:`; una riga vuota termina.
   Il valore unisce le righe con `\P`.

Dall'angolo opposto: la larghezza è la distanza orizzontale fra i due angoli
nel sistema ruotato del testo; il punto d'inserimento è il primo angolo, e il
punto di allineamento predefinito è `TL`. Lo stile è quello corrente
(`TEXTSTYLE`), l'altezza quella dello stile se è fissa, altrimenti l'ultima usata.
Un passo di annullamento: `Mtext`. `Esc` durante le righe conserva il testo già
scritto, come `TEXT` (ADR 0015); senza righe non crea nulla.

## Regole della tappa che valgono qui

- Il comando non ha l'editor nella finestra: anche nel programma si scrive
  nella riga di comando (decisione del 2026-11-11; l'editor è l'idea #662).
- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `mtext_command_test.cpp`: due righe; *Width* al posto dell'angolo; *Justify*
  `MC`; *Style* con uno stile che non c'è; `Esc` dopo una riga; `U`;
  *Line spacing* *Exactly* 1,5 in un `MTEXT` e il `MTEXT` dopo, senza
  l'opzione, ancora *Exactly* 1,5, con la rotazione tornata a 0; *Line
  spacing* *Exactly* 1,5 e `Esc` prima del testo (nessuna entità) e il
  `MTEXT` dopo a *Exactly* 1,5; `Esc` dopo una riga (testo conservato, con
  l'interlinea scelta); due runner (due disegni): *Exactly* 1,5 nel primo, il
  secondo parte da *At least* 1, e il primo la conserva.

## Fatto quando

- I casi di RT15 passano; la CI è verde.
