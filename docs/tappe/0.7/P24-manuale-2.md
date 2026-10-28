# [0.7-P24] Manuale, seconda parte, e architettura

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** P15, P23
- **Modulo:** documentazione
- **Da leggere:** le schede P14–P23 di questa tappa (per i testi dei comandi);
  gli ADR 0030 e 0035 (solo le decisioni); `docs/manuale/comandi.md`,
  `docs/manuale/interfaccia.md` e `docs/ARCHITETTURA.md` (come sono scritti oggi)

## File

- `docs/manuale/comandi.md`
- `docs/manuale/interfaccia.md`
- `docs/ARCHITETTURA.md`

## Cosa fare

- **`comandi.md`:**
  - una sezione per `TRIM` ed `EXTEND` (i due modi, `TRIMEXTENDMODE`, `Shift`,
    la polilinea bordo di se stessa), `OFFSET` (che cosa è garantito: ADR 0035,
    punto 9), `FILLET`, `CHAMFER`, `CIRCLE` *Ttr*, `JOIN`, con domande, opzioni,
    messaggi e un esempio, come le sezioni esistenti;
  - nella sezione "Negli script", le righe dei comandi nuovi;
  - la selezione scritta (un punto, `W`, `C`, `ALL`) nella sezione sulla
    selezione.
- **`interfaccia.md`:** gli snap a intersezione e a tangente.
- **`ARCHITETTURA.md`:**
  - solid-space in `third_party/solid-space`, collegato da `core` con
    `solid_space::robust`;
  - le facciate di `core` (`intersections`, `offset`, `fillet`, `join`);
  - le trasformazioni e i grip in `core`;
  - le richieste di oggetto, numero e selezione e le sequenze degli script in
    `cmd`.

## Regole della tappa che valgono qui

- Testi in italiano; comandi, domande e messaggi come li scrive il programma, in
  inglese.
- Solo ciò che è già unito su `main`.

## Test richiesti

Nessuno.

## Fatto quando

- Ogni comando elencato sopra ha la sua sezione, e l'architettura descrive
  solid-space.
- La CI è verde.
