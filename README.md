# C greičio matavimo CLI

Lengvas C kalbos komandų eilutės įrankis, kuris:

- nustato naudotojo šalį,
- iš vietinio JSON serverių sąrašo parenka geriausią serverį,
- pamatuoja atsisiuntimo spartą,
- pamatuoja išsiuntimo spartą.

Įrankis naudoja **libcurl** HTTP užklausoms ir **cJSON** serverių sąrašo apdorojimui.

## Projekto struktūra

- `main.c` – komandų eilutės argumentų apdorojimas ir vykdymo eiga.
- `json_parser.c` – `speedtest_server_list.json` nuskaitymas ir išskaidymas.
- `geo.c` – šalies nustatymas.
- `server_select.c` – geriausio serverio parinkimo logika.
- `speedtest.c` – atsisiuntimo / išsiuntimo testų logika.
- `speedtest_server_list.json` – serverių katalogas.

## Kompiliavimas

```bash
make
```

Po šios komandos sukuriamas vykdomasis failas `speedtest`.

> Pastaba: esamas `Makefile` naudoja Homebrew tipo `cJSON` kelius:
>
> - antraštėms: `/opt/homebrew/opt/cjson/include`
> - bibliotekoms: `/opt/homebrew/opt/cjson/lib`
>
> Jei jūsų aplinkoje keliai kitokie, pakoreguokite `CFLAGS` ir `LIBS` reikšmes `Makefile` faile.

## Naudojimas

```bash
./speedtest [parinktys]
```

### Parinktys

- `-a`, `--auto` – pilnas automatinis testas (vietovė + geriausias serveris + atsisiuntimas + išsiuntimas)
- `-g`, `--geo` – nustatyti naudotojo šalį
- `-b`, `--best-server` – parinkti geriausią serverį
- `-d`, `--download` – vykdyti atsisiuntimo testą
- `-u`, `--upload` – vykdyti išsiuntimo testą
- `-s`, `--server-id <id>` – serverio ID rankiniam atsisiuntimo / išsiuntimo testui
- `-f`, `--file <kelias>` – serverių JSON failo kelias (numatytasis: `speedtest_server_list.json`)
- `-h`, `--help` – parodyti pagalbą

## Pavyzdžiai

Pilnas automatinis testas:

```bash
./speedtest --auto
```

Tik vietovės nustatymas:

```bash
./speedtest --geo
```

Tik geriausio serverio parinkimas:

```bash
./speedtest --best-server
```

Atsisiuntimo ir išsiuntimo testai su konkrečiu serveriu:

```bash
./speedtest --download --upload --server-id 1234
```

```bash
