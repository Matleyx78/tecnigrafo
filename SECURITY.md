# Sicurezza

## Versioni supportate

Fino alla versione 1.0 riceve le correzioni di sicurezza solo l'ultima versione
pubblicata. Le correzioni escono come versioni `X.Y.Z` di sola correzione
([docs/ROADMAP.md](docs/ROADMAP.md)).

## Segnalare una vulnerabilità

**Non aprire una issue pubblica.** Usa la segnalazione privata di GitHub: scheda
*Security* del repository → *Report a vulnerability*. La segnalazione la vede solo
il responsabile del progetto.

Scrivi, se puoi:

- la versione di Tecnigrafo e il sistema operativo;
- che cosa succede e che cosa ti saresti aspettato;
- i passi per riprodurre il problema, con un file di esempio quando serve (per
  esempio un DXF o un `.cadx` costruito apposta);
- le conseguenze che vedi (crash, lettura o scrittura di file fuori dal previsto,
  esecuzione di codice).

## Che cosa succede dopo

- Risposta di presa in carico entro 7 giorni.
- Se la vulnerabilità è confermata, la correzione si prepara in privato e si
  pubblica con una nuova versione; la segnalazione diventa pubblica, con il tuo
  nome se lo desideri, solo dopo l'uscita della correzione.
- Se non è una vulnerabilità, ti spieghiamo perché e, se è un bug, ti chiediamo
  di aprire una issue normale.

Tecnigrafo è un progetto volontario: i tempi di correzione dipendono dalla
gravità e da chi è disponibile, ma ogni segnalazione riceve una risposta.

## Che cosa interessa di più

Tecnigrafo apre file che arrivano da altri. I punti più delicati sono quindi la
lettura dei file (DXF, formato nativo `.cadx`, e in futuro gli altri formati
importati) e, quando ci saranno, gli script e le estensioni.
