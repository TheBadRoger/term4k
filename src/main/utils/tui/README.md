# term4k lightweight TUI core (phase 1)

This folder contains the first low-level TUI utilities:

- `DisplayWidth`: UTF-8 display-width helpers for multilingual terminal clipping.
- `FramePacer`: adaptive frame pacing (`120 -> 60 -> 30`) based on render pressure.
- `InputDecoder`: parses keyboard arrows, `j/k`, `Enter`, `q`, and XTerm wheel events.
- `RenderBuffer`: keeps previous/current frame lines and emits ANSI diffs only for changed rows.
- `SceneLoopCore`: reusable thread lifecycle shell for paced scene loops.

Current integration target is `ui::UserSettingsUI`.

