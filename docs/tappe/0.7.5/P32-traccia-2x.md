# [0.7.5-P32] Traccia della fase 2.x rivista con l'ADR 0041

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** gemini
- **Modello e impegno:** leggero; impegno basso
- **Riferimenti:** non servono: documentazione
- **Dipende da:** —
- **Modulo:** `docs`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md) per intero;
  [tappe/2.x-modellatore-3d.md](../2.x-modellatore-3d.md);
  [ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md), punti 4 e 5;
  [DECISIONI.md](../../DECISIONI.md), 2026-11-06 (la scheda 2.x è una traccia)

## File

- `docs/tappe/2.x-modellatore-3d.md`

## Cosa fare

La scheda resta una **traccia**, non un piano: lo dice in testa. Si aggiorna
con l'ADR 0041, senza aggiungere decisioni che l'ADR non prende:

1. **Prodotto:** un solo programma con ambienti (disegno, parte, schizzo,
   assieme, elettrico più avanti), ognuno con il suo tipo di documento e di
   file, documenti in schede (ADR 0041, punti 1, 2, 7 e 8).
2. **Tappe previste:** le sette di oggi restano, nell'ordine, con questi
   ritocchi:
   - *Kernel e vista 3D*: il modulo `modeling` e le regole di dipendenza del
     punto 10 dell'ADR (mai in senso inverso; il CAD 2D si compila senza OCCT;
     la prova della CI che compila `app` senza `modeling`); il primo tipo di file
     di parte, con il contenitore estratto da `io`;
   - *Schizzi vincolati*: i principi del punto 11 (lo schizzo possiede un
     documento 2D, piano e vincoli; stessi comandi 2D; annulla della sessione di
     schizzo; importazione per copia); i vincoli in un modulo condiviso sotto
     `modeling`;
   - *Tavole*: la tavola è un disegno autosufficiente; se le viste restano
     collegate al modello lo decide la sua tappa;
   - l'ADR delle aree di lavoro al più tardi con la prima tappa (ADR 0009,
     punto 7).
3. **Che cosa non è deciso:** i punti del 12 dell'ADR (riferimenti tra file,
   vincoli nel CAD, elettrico), elencati come aperti.
4. Il «Principio» in fondo resta.

## Regole della tappa che valgono qui

- Nessuna regola nuova: ogni frase nuova rimanda al punto dell'ADR da cui viene.

## Test richiesti

Nessuno: è un documento.

## Fatto quando

- La traccia rimanda all'ADR 0041 per ogni punto toccato.
- La CI è verde.
