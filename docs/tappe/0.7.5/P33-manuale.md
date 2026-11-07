# [0.7.5-P33] Manuale: l'interfaccia nuova

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** P31, P18
- **Modulo:** `docs`
- **Da leggere:** [manuale/interfaccia.md](../../manuale/interfaccia.md);
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 1;
  [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 2, 8 e 9;
  [ADR 0042](../../adr/0042-immissione-dinamica.md), punti 2–4; le immagini di
  `docs/adr/0039/`; le schede P12, P13, P14, P16, P18, P19, P28–P31 della tappa

## File

- `docs/manuale/interfaccia.md`
- `docs/manuale/comandi.md`: solo dove nomina un pulsante o un menu che è
  cambiato

## Cosa fare

Il manuale descrive l'interfaccia come è alla fine della tappa, nell'ordine delle
aree dell'ADR 0039:

1. barra del titolo e barra di accesso rapido; scheda *File*; ribbon compatto,
   gruppi e varianti dei comandi (le cinque di `CIRCLE`);
2. schede dei documenti e pagina iniziale *Start*; che cosa succede passando da
   un documento all'altro; che cosa succede chiedendo un ambiente che il
   documento attivo non ospita (un documento nuovo in una scheda, niente si
   chiude); quando il programma chiede di salvare;
3. pannelli *Properties* e *Layers*; i pannelli che si nascondono da soli, se
   P18 è stato fatto;
4. riga di comando e **immissione dinamica**: campi, `Tab`, `Invio`, `F12`, la
   riga equivalente nella cronologia;
5. barra di stato: coordinate, interruttori con le loro scorciatoie,
   indicatore dell'ambiente.

Le immagini sono quelle di `docs/adr/0039/` (si collegano, non si copiano). I
nomi di pulsanti, schede e menu sono quelli in inglese del programma.

## Regole della tappa che valgono qui

- Se P18 non è stato fatto (il responsabile non l'ha approvato), il punto sui
  pannelli che si nascondono non c'è.

## Test richiesti

Nessuno: è un documento.

## Fatto quando

- Il manuale descrive le cinque aree qui sopra come le mostra il programma.
- La CI è verde.
