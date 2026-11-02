# Desk Pet - Pebble appstore listing

Everything needed to submit **Desk Pet** from the
[Developer Dashboard](https://developer.repebble.com/dashboard) (or `pebble publish`).
Apps go live **immediately** (no review queue), so submit as **Unlisted** first,
test it from the link on a real Round 2, then switch to Listed.

## Listing fields

| Field | Value |
|---|---|
| Type | Watchface (decided by `watchapp.watchface` in `package.json`, cannot change later) |
| App name | Desk Pet |
| Version | 1.0.0 (format must be `major.minor.0`; every release needs a new, unused version) |
| UUID | in `package.json`, must stay the same for all later releases |
| Platforms | gabbro (Pebble Round 2) |
| Category | none (watchfaces are listed under Faces automatically) |
| Source code URL | optional, e.g. https://github.com/BitterMelonFarm/watchfaces |

### Description (plain text)

A tiny pixel pet who lives out its day on your wrist. What it is doing follows the clock:
sleeping in bed at night, stretching and brushing its teeth in the morning, commuting,
coding at its desk (with video-call meetings), lunch, dinner with the kids, a bedtime
story, and relaxed weekends with walks in the park, TV and video games.

Nine pets to choose from: blob, cat, bunny, bear and frog, plus four cute Lovecraftian
ones - baby Cthulhu, a shoggoth, a deep one and a floating Great Eye - each with its own
little quirks in the scenes.

Your steps today fill a bar toward 8,000. Fall behind pace in the afternoon and your pet
gets sad (rain cloud!); hit the goal and it celebrates.

Controls: flick your wrist once to pet your pet (hearts!), flick twice within two seconds
to switch to the next pet. Your choice is remembered.

The time is shown in a pink-framed display with a blinking colon. Battery shows as five hearts. The pet animates once a second only while the face is
visible and the battery is above 15%; otherwise it ticks once a minute to save power.

Privacy: Desk Pet reads your step count from Pebble Health to draw the step bar. Nothing
is sent anywhere; there is no network access, tracking or account.

### Release notes (1.0.0)

First release.

## Images

Screenshots must be **exactly 260x260, PNG/JPEG/GIF, no watch frame**, up to 5, taken from
the real firmware (the renders in `docs/` use a stand-in font and a round mask, so they are
**not** suitable as store screenshots). Capture real ones with the emulator or a watch:

```sh
cd desk-pet
pebble build
pebble install --emulator gabbro
pebble emu-set-time ...          # see `pebble emu-set-time --help` to show each scene
pebble screenshot --all-platforms   # writes screenshots/gabbro_*.png at the right size
```

Good five to pick: coding at the desk (weekday ~10:00), bedtime story (20:15), park walk
(weekend ~10:00), sleeping (23:00) and a Cthulhu/Great Eye variant.
A 720x320 banner is optional.

## Pre-submission checklist

- [x] Built with a released SDK (4.33.1); `pebble build` succeeds with no code warnings
- [x] `.pbw` is 27 KB (limit 4.4 MB)
- [x] Version `1.0.0` matches the `major.minor.0` rule; UUID generated, unique
- [x] `health` capability declared (the face reads step count)
- [x] Memory-safety sweep of all scenes x pets x times with AddressSanitizer/UBSan: clean
- [x] No network, location or phone-side code
- [ ] **Run on the emulator or a real Round 2** (not done in the build environment: the
      emulator never answered, so the 1 fps animation, tap gestures, step bar and Health
      access are untested on real firmware)
- [ ] **Real screenshots** captured (see above)
- [ ] Decide the public author name (`author` in `package.json` is currently `edutch95`)
- [ ] Add a LICENSE file if you give a Source code URL
- [ ] Read the [legal agreements](https://developer.repebble.com/legal.md)
- [ ] Submit as **Unlisted**, test from the link, then make it Listed

## Not supported yet

Only `gabbro` is built. The older round watch (`chalk`) would need layout work: the fonts
do not scale, so the time and date collide with the room panel at 180x180. Rectangular and
black-and-white watches are not supported.
