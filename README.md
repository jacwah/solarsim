# Solar system simulation
A school physics project.

## Compile and run
Requires `SDL2` and `SDL2_ttf` libraries and headers to be installed. Currently only runs on macOS because it looks for a font at `/Library/Fonts/Arial.ttf`, but can easily support other platforms by modifying the `FONT_PATH` macro in `main.c`.

    $ make
    $ ./solarsim

    # or simply
    $ make run
