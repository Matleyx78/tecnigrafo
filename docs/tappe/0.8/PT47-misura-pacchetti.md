# [0.8-PT47] Misura dei pacchetti della 0.8 (#442)

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: strumento del metodo e documento
- **Dipende da:** PT46, l'ultimo pacchetto della tappa: la misura conta tutte le
  pull request della 0.8, quindi parte quando le altre issue del milestone
  `v0.8` sono chiuse
- **Modulo:** documenti (lo script c'è già)
- **Da leggere:** [DECISIONI.md](../../DECISIONI.md), 2026-11-10, «Pacchetti
  medi»; l'[analisi della 0.7.5-P8](../0.7.5/analisi-pacchetti-medi.md), sezioni
  1, 3.9 e 5; la [misura della 0.7.5](../0.7.5/misura-pacchetti.md);
  `scripts/misura_pacchetti.py`; il piano della 0.8, sezione 6 (difficoltà,
  agente, modello e impegno previsti, e «Motivi»)

## File

- `docs/tappe/0.8/misura-pacchetti.md` (nuovo)
- `scripts/misura_pacchetti.py` solo se serve per i prefissi `PT`/`RT` o per le
  lettere dei blocchi di riferimenti; con i suoi test

## Cosa fare

1. Rifai la misura con lo script e il metodo della 0.7.5-P39, su tutte le pull
   request della 0.8.
2. Confrontala con l'analisi della #442 e con la misura della 0.7.5.
3. Per ogni pacchetto che il piano dava come **impegnativo** in passato e che
   questa tappa ha realizzato in **fascia media** (le schede «media» tradotte da
   decisioni scritte: scheletro, blocchi, quote, retini, `PEDIT`, tracciamento),
   come è andato: giri di correzione, buchi trovati in revisione, correzioni dopo
   il merge, tempo.
4. Proponi al responsabile la **decisione definitiva** sulle condizioni della
   #442 (criterio di chiusura della tappa), con le cifre a sostegno e quello che
   le cifre non dicono.

## Fatto quando

- Il documento della misura è unito, con la proposta di decisione.
