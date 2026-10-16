# Stile del codice

La formattazione è decisa da `clang-format`: non si discute. Questo documento
copre ciò che uno strumento non può decidere da solo.

## Nomi

| Elemento | Stile | Esempio |
|---|---|---|
| Namespace | minuscolo | `tg::core` |
| Tipi, classi, enum | `PascalCase` | `PolylineSegment` |
| Funzioni e metodi | `camelCase` | `boundingBox()` |
| Variabili e parametri | `camelCase` | `maxChordError` |
| Membri privati | `m_` + `camelCase` | `m_vertices` |
| Costanti | `k` + `PascalCase` | `kMaxArcSegments` |
| File | `snake_case` | `scene_renderer.cpp` |
| Macro | evitarle; se proprio servono `TG_MAIUSCOLO` | `TG_VERSION` |

## Regole

- Header con `#pragma once`, estensione `.hpp`; sorgenti `.cpp`.
- Gli header pubblici di un modulo stanno in `<modulo>/include/tg/<modulo>/`.
- Niente `using namespace` negli header.
- Niente `new`/`delete` espliciti, tranne i widget Qt che passano la proprietà al
  genitore. Altrimenti valori, `std::unique_ptr`, contenitori standard.
- Valori di ritorno che non vanno ignorati: `[[nodiscard]]`.
- Errori attesi (file non valido, geometria impossibile) tramite valore di ritorno
  (`std::optional`, più avanti `std::expected`). Le eccezioni sono riservate a
  situazioni davvero eccezionali e non attraversano mai i confini tra moduli.
- Le precondizioni si verificano con `assert` e si documentano nell'header.
- Nessun numero "magico": costanti con nome e commento che ne spiega l'origine.
- Un commento spiega il *perché*, non il *cosa*.

## Geometria

- Le coordinate del disegno sono `double`, in unità di disegno; Y verso l'alto,
  angoli in radianti in senso antiorario da +X (vedi ADR 0004).
- Le coordinate dello schermo sono pixel logici, origine in alto a sinistra, Y
  verso il basso. La conversione passa sempre da `render::Viewport`.
- Mai confrontare due `double` con `==` per decidere un fatto geometrico: si usa
  una tolleranza esplicita.
