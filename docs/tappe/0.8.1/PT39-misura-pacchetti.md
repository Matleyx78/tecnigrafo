# [0.8.1-PT39] Misura dei pacchetti della 0.8.1 (#442)

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: strumento del metodo e documento
- **Dipende da:** PT38, l'ultimo pacchetto della tappa: la misura conta tutte le
  pull request della 0.8.1, quindi parte quando le altre issue del milestone
  `v0.8.1` sono chiuse
- **Modulo:** documenti (lo script c'è già)
- **Da leggere:** [DECISIONI.md](../../DECISIONI.md), 2026-11-10, «Pacchetti
  medi»; la [misura della 0.7.5](../0.7.5/misura-pacchetti.md)
  e quella della [0.8](../0.8/misura-pacchetti.md); la scheda della 0.8
  [PT47](../0.8/PT47-misura-pacchetti.md); `scripts/misura_pacchetti.py`; il
  piano della 0.8.1, sezione 6 (difficoltà, agente, modello e impegno
  previsti, e «Motivi»)

## File

- `docs/tappe/0.8.1/misura-pacchetti.md` (nuovo)
- `scripts/misura_pacchetti.py` solo se serve per leggere la 0.8.1 (per
  esempio il milestone `v0.8.1` o le issue create dallo script di apertura),
  con i suoi test e senza cambiare i calcoli

## Cosa fare

1. Rifai la misura **con lo script e il metodo della 0.8** (PT47), su tutte le
   pull request della 0.8.1: stesse tabelle, stesse definizioni (giri,
   correzioni, fermi, tempi), così le tre tappe si confrontano.
2. Confrontala con la misura della 0.7.5 e con quella della 0.8, e con il
   piano: per ogni pacchetto, difficoltà, fascia e impegno previsti contro
   quello che è successo.
3. La **velocità della catena**, come la vuole il responsabile: per
   Tecnigrafo soltanto (solid-space ha troppi meno pacchetti per tappa per
   confrontarsi), dal primo all'ultimo pacchetto della tappa, con tutto ciò che
   sta in mezzo, contro la 0.8, con i tempi che lo script già calcola.
4. **Nessuna proposta nuova.** La proposta di decisione definitiva sulla #442
   (misura della 0.8, sezione 6) resta in sospeso come l'ha lasciata il
   responsabile: il documento aggiunge solo le cifre della 0.8.1, dicendo dove
   confermano o smentiscono quella proposta.

## Regole della tappa che valgono qui

- Nessuna: il metodo non cambia (decisione del responsabile del 2026-11-22).

## Test richiesti

- Solo quelli dello script, se lo tocchi.

## Fatto quando

- Il documento della misura è unito, con le cifre della 0.8.1 accanto a
  quelle della 0.7.5 e della 0.8.
