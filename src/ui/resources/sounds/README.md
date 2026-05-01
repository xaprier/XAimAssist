# Sound Effects

Audio assets used for hit and miss feedback during training sessions.
All files are converted to 16-bit mono WAV (44100 Hz) for use with `QSoundEffect`.

---

## Hit Sounds

Played when a target is successfully hit.

| File | Description | Source |
|------|-------------|--------|
| `hit_swish.wav` | Basketball net swish — satisfying, clean confirmation | [ElevenLabs](https://elevenlabs.io) (`basketball_net_swish.mp3`) |
| `hit_click.wav` | Subtle UI button click — minimal, low-distraction | [Pixabay](https://pixabay.com/sound-effects/immersivecontrol-button-click-sound-463065/) (`button_click.mp3`) |
| `hit_gunshot.wav` | Single gunshot — impactful, FPS-style feedback | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-single-gunshot-52-80191/) (`gunshot.mp3`) |
| `hit_pop.wav` | Soft pop — light and snappy | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-pop-sound-423716/) (`pop2.mp3`) |

---

## Miss Sounds

Played when a shot misses all targets.

| File | Description | Source |
|------|-------------|--------|
| `miss_ricochet.wav` | Basketball missed shot — natural miss feeling | [ElevenLabs](https://elevenlabs.io) (`basketball_missed_shot.ogg`) |
| `miss_empty.wav` | Empty gun click — dry, immediate miss cue | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-empty-gun-shot-6209/) (`gunshot_empty.mp3`) |
| `miss_beep.wav` | Short UI beep — subtle negative feedback | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-ui-sounds-pack-3-4-359718/) (`miss_1.mp3`) |
| `miss_tap.wav` | Light tap / UI click — minimal miss indicator | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-ui-button-click-7-341028/) (`miss_2.mp3`) |
| `miss_pop.wav` | Soft pop — gentle error tone | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-pop-up-something-160353/) (`pop.mp3`) |
| `miss_oops.wav` | "Oops" effect (first 1.5 s trimmed) — humorous miss cue | [Pixabay](https://pixabay.com/sound-effects/film-special-effects-oops-sfx-527192/) (`oops.mp3`) |

---

## Notes

- Original source files (`.mp3`, `.ogg`) are kept alongside the WAV conversions for reference.
- WAV conversion command used: `ffmpeg -i <src> -ar 44100 -ac 1 -acodec pcm_s16le <dst>.wav`
- `miss_oops.wav` is trimmed to 1.5 seconds to remove the long reverb tail of the original.
