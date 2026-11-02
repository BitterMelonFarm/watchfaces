# Submitting Desk Pet 1.0.0

Files in this folder:

| File | What it is |
|---|---|
| `desk-pet-1.0.0.pbw` | The release build: watchface, `gabbro` (Pebble Round 2), version 1.0.0, 28 KB |
| `listing.txt` | Name, description and release notes to paste into the dashboard |
| `capture_screenshots.sh` | Captures the five 260x260 store screenshots from the emulator |

## Before you submit

1. **Test it for real.** Sideload `desk-pet-1.0.0.pbw` onto a Round 2 (or run
   `pebble install --emulator gabbro`). This build has only ever been checked in an offline
   renderer plus a memory-safety sweep: the animation, tap gestures, step bar and Health access
   have not run on real firmware.
2. **Get real screenshots:** `bash release/capture_screenshots.sh` (needs a working emulator).
   The store wants exactly 260x260 with no watch frame; the renders in `docs/` are not suitable
   (stand-in font, round mask).
3. **Decide the public author name.** The `.pbw` currently says `edutch95` (the `author` field in
   `package.json`). To change it: edit `package.json`, run `pebble build`, and replace the `.pbw`.
4. Read the [legal agreements](https://developer.repebble.com/legal.md). Add a LICENSE file if you
   will give a source URL.

## Submit (apps go live immediately, there is no review queue)

**Dashboard:** open https://developer.repebble.com/dashboard -> *New* -> upload
`desk-pet-1.0.0.pbw` -> paste name/description from `listing.txt` -> upload the screenshots ->
set *App visibility* to **Unlisted** -> *Submit App*.

**Or the CLI:** from `desk-pet/`: `pebble login`, then
`pebble publish --release-notes "First release." ` (it can capture screenshots itself from the
emulator and uploads them).

Then open the app's link (`https://apps.repebble.com/<id>`) on your phone, install it on the
watch, check it, and switch visibility to **Listed** from *Edit* in the dashboard.

## Later releases

Bump `version` in `package.json` to a new `major.minor.0` (e.g. `1.1.0`), keep the same UUID,
rebuild, and use *New Release* in the dashboard (or `pebble publish`).
