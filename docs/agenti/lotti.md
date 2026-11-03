# Lavoro a lotti

Da leggere quando il messaggio del responsabile dice «pacchetti di riferimenti
pronti» o «rivedi le pull request aperte» ([AGENTS.md](../../AGENTS.md), "Se ...
leggi anche ..."). Vale insieme al nucleo e ai file del lavoro che fai
([riferimenti.md](riferimenti.md), [revisione.md](revisione.md)).

## Lavoro a lotti

Due eccezioni alla regola "un pacchetto per sessione":

- **"Prendi i pacchetti di riferimenti pronti"**: uno dopo l'altro, nell'ordine
  delle condizioni 1–3 e 5 ([pacchetto.md](pacchetto.md), "Se ti chiedono di
  prendere il prossimo pacchetto"), ciascuno con il suo ramo partito da `main`
  aggiornato e la sua pull request portata alla CI verde; alla fine un solo
  riepilogo con tutte le pull request.
- **"Rivedi le pull request aperte"**: una alla volta, tutte quelle senza
  `AA-Presa in carico` scritte da un modello diverso dal tuo, prese in carico come
  dice il nucleo, con un commento di revisione. Non approvare e non unire.

Se in un lotto ti fermi per uno dei motivi di "Quando fermarsi" o dei file della
tabella, scrivilo nella pull request (o nella issue, se ti fermi per un buco nel
contratto o nella scheda) e passa al pacchetto successivo.
