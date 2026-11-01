# watchfaces

Watchfaces for the Pebble Round 2 (`gabbro` platform, 260x260 round color).

## desk-pet

A Tamagotchi-style pet whose activity follows the time of day.

| When | Activity |
|---|---|
| 22:00-07:00 | Sleeping (moon, stars, floating z's) |
| 07:00-09:00 | Waking up and stretching |
| Weekdays 09:00-17:00 | Coding at a desk (12:00 lunch) |
| 17:00-18:00 | Family dinner |
| 18:00-22:00 | Playing ball with the kids |
| Weekends | Play in the morning, couch + TV 13:00-17:00 |

Battery is shown as 5 hearts. A green step bar fills toward 8,000 steps (gold when reached); the pet frowns and cries if you fall well behind pace after 11:00, and smiles once the goal is hit. All art is drawn with rectangles (no image assets).

### Build

```sh
uv tool install pebble-tool
pebble sdk install latest
cd desk-pet
pebble build
pebble install --emulator gabbro
```

### Status

- `pebble build` succeeds for `gabbro` against SDK 4.33.1 (no code warnings).
- The Pebble emulator (QEMU + pypkjs) boots the gabbro firmware in the cloud sandbox but never answers the protocol, so the face has **not** been run in the real emulator or on a watch.
- `desk-pet/tools/preview/render.py` compiles the real `main.c` against a stub Pebble API and draws every scene to PNG (stand-in font, no firmware). Output: `desk-pet/docs/preview.png`.

```sh
python3 desk-pet/tools/preview/render.py /tmp/preview   # needs gcc + Pillow
```

![preview](desk-pet/docs/preview.png)
