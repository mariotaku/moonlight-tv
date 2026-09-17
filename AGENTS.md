# AGENTS.md

Guidance for AI coding agents working in this repository.

## What this is

Moonlight TV: a GameStream client in C11 for LG webOS TVs, Raspberry Pi, Steam Link and desktop Linux. CMake build, SDL2 + LVGL v8 UI, gettext i18n. The primary target is webOS — several features are gated per-platform in the root `CMakeLists.txt` (`TARGET_WEBOS` is auto-detected from the toolchain triple, e.g. `arm-webos-linux-gnueabi`).

## Build

Submodules are required, and `nanors` is nested inside `moonlight-common-c`, so always clone/update with `--recursive`.

Desktop Linux (mirrors `.github/workflows/build-desktop.yml`):

```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev libopus-dev libcurl4-openssl-dev uuid-dev \
  libavcodec-dev libavutil-dev libexpat1-dev libmbedtls-dev libfontconfig1-dev gettext
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DTARGET_DESKTOP=ON
cmake --build build
```

webOS (needs the buildroot NDK from `openlgtv/buildroot-nc4`; CI pins the release tag in `.github/actions/build-webos/action.yml`):

```bash
TOOLCHAIN_FILE=/opt/arm-webos-linux-gnueabi_sdk-buildroot/share/buildroot/toolchainfile.cmake \
  ./scripts/webos/easy_build.sh -DCMAKE_BUILD_TYPE=Release   # configure + build + cpack (.ipk)
```

Gotchas:

- `gettext` (msgfmt) is a hard configure-time requirement for non-webOS builds — `cmake/MoonlightI18n.cmake` does `FATAL_ERROR` without it. webOS builds skip gettext (`FEATURE_I18N_GETTEXT OFF`) and instead convert `.po` → `cstrings.json` at package time via `scripts/webos/po2json.awk`.
- This means Windows configure fails out of the box unless gettext is installed; there are no Windows build docs in the repo.
- Release builds in CI use `Release` (desktop/RasPi/Steam Link) or `RelWithDebInfo` (webOS release job).

## Tests

`BUILD_TESTS` defaults to ON for desktop, OFF for webOS/Steam Link. Unity framework, tests link against `moonlight-lib`.

```bash
cmake --build build && ctest --test-dir build            # all tests
ctest --test-dir build -R test_settings                  # single test by name
xvfb-run ctest -C Debug                                  # what CI runs (e2e tests create SDL windows)
```

Test executables are declared with `add_unit_test(NAME SOURCES)` in `tests/{core,app}/CMakeLists.txt`; fixtures resolve via the `FIXTURES_PATH_PREFIX` compile definition.

## Architecture

Three layers, dependency-ordered:

1. **Protocol / backend** — `core/moonlight-common-c` (submodule; Limelight `Li*` API, decode units, input events) and `core/libgamestream` (GameStream HTTPS pairing/launch client). `src/app/backend/` builds on these: `pcmanager` (host discovery via mDNS, pairing, wake-on-LAN, worker threads) and `apploader` (game list + covers).
2. **Session** — `src/app/stream/`. `session.c` owns lifecycle/config; `session_video.c`/`session_audio.c` are the `DECODER_RENDERER_CALLBACKS`/audio callbacks that feed **SS4S** (`third_party/ss4s`), the platform-output abstraction that picks a driver (webOS `ndl`/`lgnc`/`ndl-webos5`, ffmpeg on desktop…) at runtime. `stream/input/` translates SDL input to `LiSend*` calls and owns the special combos.
3. **UI** — `src/app/ui/` on LVGL v8 (mariotaku fork submodule, SDL renderer). Fragment-based: `launcher/` (host list, apps grid + `coverloader`), `settings/` (panes in `settings/panes/`, widgets from `pref_obj.h`), `streaming/` (in-stream overlay + stats). `src/app/lvgl/` bridges SDL⇄LVGL (display driver, indev drivers).

Cross-cutting mechanics worth knowing before touching anything:

- **Main loop** (`app.c`): `app_process_events` + `lv_task_handler` + `SDL_Delay(1)` at ~1 kHz. The 1 ms cadence and `LV_INDEV_DEF_READ_PERIOD 1` are deliberate — during streaming, input events are drained and forwarded to the host from the SDL event filter, so this is the input-latency floor. Don't "optimize" it upward.
- **Event bus** (`src/app/util/bus.h`, `platform/sdl/bus.c`): cross-thread work reaches the main thread as `SDL_USEREVENT`s (`bus_pushevent`, `app_bus_post`, blocking `app_bus_post_sync`). `USER_*` codes live in `util/user_event.h`. Note the dispatch runs inside `SDL_FilterEvents`, which holds SDL's event-queue mutex — long work in a bus callback stalls every other thread that pushes events.
- **Settings**: one struct (`app_settings_t` in `app_settings.h`), persisted to `moonlight.ini` by hand-written read/write in `app_settings.c`. Adding a setting means: struct field + default in `settings_initialize` + `ini_write_*` + `INI_NAME_MATCH` parse + a `pref_*` widget in a settings pane. The global `app_configuration` aliases `&app->settings` (`app.c`).
- **Stats/overlay**: `streaming.controller.c` keeps `overlay_showing`/`overlay_pinned` statics; `streaming_stats_shown()` gates whether `session_video.c` computes latency stats at all. The stats panel has no HIDDEN flag of its own — it is hidden only by being a child of the hidden overlay, so reparenting it (the pin) makes it visible immediately.
- **Timing units**: `DECODE_UNIT.receiveTimeUs`/`enqueueTimeUs` and `VIDEO_STATS.*TimeUs` are **microseconds**; they share an epoch with `LiGetMicroseconds()` only because both resolve to `PltGetMicroseconds()` — re-verify on every moonlight-common-c bump (see comment in `session_video.c`).
- **In-stream combos** (`stream/input/`): gamepad START+BACK+LB+RB → overlay (fires on release); keyboard Ctrl+Alt+Shift+S → overlay; webOS remote RED or EXIT → overlay. The hint string about "long press BACK" is stale — no long-press path exists in streaming.

## i18n — the part that will bite you

Catalogs live in `src/i18n/<locale>/messages.po` (16 dirs exist), but **what ships is controlled by `I18N_LOCALES` in the root `CMakeLists.txt`** (11 locales as of writing) — it drives both the `.mo` compilation for gettext builds and the webOS `cstrings.json` generation, *and* the language dropdown contents (`basic.pane.c` tokenizes the `I18N_LOCALES` compile definition; display names come from `i18n_locales[]` in `platform/common/i18n_common.c`). A `.po` that is not in that list is dead weight: es-ES, ko, tr, uk and zh-TW are currently in that state — fully or partly translated but never packaged or offered. Adding a locale = `.po` dir + `I18N_LOCALES` + `i18n_locales[]` entry. CJK needs font support (`ja` is gated behind `#if DEBUG` for that reason).

Workflow facts (from maintainer comments in #122/#428/#430): translations are preferred via Crowdin (`https://crowdin.com/project/moonlight-tv`, project ID 484733); there is **no** automated Crowdin↔repo sync — the maintainer moves files manually; `messages.pot` is regenerated by the maintainer with the `i18n-update-pot` target and is routinely behind the code. Don't hand-edit the `.pot`; adding entries to a `.po` for strings that exist in source is fine and survives `msgmerge`. Strings are marked with `locstr()` (runtime) or `translatable()` (compile-time marker only).

## Submodule / dependency notes

- `third_party/lvgl` is mariotaku's fork on an LVGL-v8 branch, intentionally divergent from upstream v9 — don't try to "update" it.
- `third_party/ss4s` and `third_party/commons` are the actively-maintained mariotaku deps; `commons` also provides `sps_parser` (H.264/HEVC dimension parsing in `session_video.c` — no AV1 parser, so AV1 streams report no dimensions in stats).
- webOS video capability caps in ss4s are measured device limits, not guesses (streams >65 Mbps crash some TVs) — don't raise them without hardware testing.
- Changes to streaming behavior can only be validated on a real webOS device against a real host; CI proves compilation, nothing more.
