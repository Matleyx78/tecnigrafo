# [0.5-P3] Specifica del formato `.cadx`

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: documentazione
- **Dipende da:** —
- **Modulo:** documentazione (`docs/formati/`)
- **Da leggere:** [ADR 0006](../../adr/0006-modello-dati-dwg-e-formato-cadx.md)
  punto 3, [ADR 0019](../../adr/0019-archivi-zip.md),
  [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punti 2 e 3,
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punto 2

## File

- `docs/formati/cadx.md` (nuovo)

## Cosa fare

Scrivere la specifica pubblica del formato nativo, abbastanza precisa perché
chiunque possa leggere e scrivere un `.cadx` senza guardare il codice di
Tecnigrafo. È il contratto che seguiranno P14 (lettura e scrittura) e R14 (i
riferimenti): ogni regola deve essere verificabile.

Contenuto:

1. **Intestazione**: titolo, versione del formato (1), licenza CC BY 4.0
   (ADR 0006), stato.
2. **L'archivio**: ZIP secondo la specifica APPNOTE di PKWARE; metodi ammessi
   *stored* e *deflate*; niente cifratura, ZIP64, archivi su più dischi; nomi
   delle parti in UTF-8, relativi, senza `..` né `\`, senza duplicati; i
   limiti che un lettore conforme può imporre (ADR 0019, punto 2).
3. **Le parti**, ciascuna con nome, obbligatoria o facoltativa, contenuto:
   - `mimetype`: primo, *stored*, senza campi extra, contenuto esatto
     `application/vnd.tecnigrafo.cadx` senza a capo;
   - `manifest.json`: oggetto JSON con `format` (intero, oggi 1),
     `generator` (testo, per esempio `Tecnigrafo 0.5.0`), `parts` (elenco dei
     nomi delle altre parti presenti); ogni altra chiave si conserva;
   - `drawing.dxf`: il disegno completo, DXF ASCII R2018 (`AC1032`), UTF-8,
     autosufficiente: estratto, si apre in qualunque CAD;
   - `extensions.json`: facoltativa; dati propri di Tecnigrafo che fanno
     riferimento alle entità con il loro handle; nella versione 1 è vuota o
     assente, e un lettore la conserva così com'è;
   - `thumbnail.png`: facoltativa, anteprima per i gestori di file;
   - parti sconosciute: un lettore le conserva e le riscrive.
4. **Versioni**: un lettore apre tutte le versioni fino alla sua e rifiuta le
   più recenti con un messaggio che dice entrambe; cosa cambia la versione
   (una modifica che un lettore vecchio leggerebbe male) e cosa no (una
   chiave o una parte facoltativa in più).
5. **Scrittura deterministica** (consigliata): data e ora fisse nelle
   intestazioni ZIP, ordine delle parti `mimetype`, `manifest.json`,
   `drawing.dxf`, poi le altre in ordine di nome.
6. **Esempio** completo di `manifest.json`, e i comandi per costruire a mano
   un `.cadx` valido con lo strumento `zip` (prima `mimetype` con `-0 -X`).

## Regole della tappa che valgono qui

- La specifica non introduce decisioni nuove: se ne serve una, fermati e
  scrivilo nella issue.
- In italiano, come il resto di `docs/`; nomi di file, chiavi JSON e valori
  in inglese.

## Test richiesti

Nessuno: è documentazione. L'esempio di costruzione a mano va provato una
volta con lo strumento `zip` e `unzip -l`, e la pull request dice come.

## Fatto quando

- `docs/formati/cadx.md` copre i sei punti sopra, con regole verificabili.
- Ogni regola si ritrova nell'ADR 0006, 0017, 0018 o 0019.
- La CI è verde.
