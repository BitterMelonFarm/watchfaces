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

Battery is shown as 5 hearts. All art is drawn with rectangles (no image assets).

### Build

```sh
uv tool install pebble-tool
pebble sdk install latest
cd desk-pet
pebble build
pebble install --emulator gabbro
```

**Status:** written without SDK access (network-blocked); only syntax-checked against a stub header. Not yet built or run in the emulator.
