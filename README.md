# ORBIT

ORBIT is a native OpenOrbis PS4 homebrew command center. The SceVideoOut foundation and v0.3.0 graphics engine are physically verified. Version 0.4.0 adds a fixed-capacity UI framework, screen transitions, DualShock 4 navigation, and the first functional dashboard pages on the existing 1920×1080 renderer.

## Build

Requirements: OpenOrbis SDK, Clang, LLD, Make, the local .NET 8 installation at `~/.dotnet`, and the LibOrbisPkg source checkout beside the SDK under `../Tools/LibOrbisPkg`.

```sh
export OO_PS4_TOOLCHAIN="$HOME/Downloads/PS4-Dev/SDK"
export DOTNET_ROOT="$HOME/.dotnet"
export PATH="$DOTNET_ROOT:$PATH"
make
make inspect
```

The package is written to `build/IV0000-ORBT00001_00-ORBITFOUNDATION1.pkg`. The Makefile archives the LibOrbisPkg source into `build/tools/source`, publishes its `PkgTool.Core` as a self-contained Linux x64 single-file executable at `build/tools/publish/PkgTool.Core`, then uses that replacement for SFO and PKG operations. Set `LIBORBISPKG_ROOT=/path/to/LibOrbisPkg` if the checkout is elsewhere. The replacement avoids the bundled .NET Core 3.0 executable’s OpenSSL 3 incompatibility and requires no system-wide runtime changes. `make inspect` lists package entries and validates package hashes and signatures.

`make test-host` checks canvas composition, FreeType rendering, PNG decoding, texture caching, UI components, controller repeat timing, screen transitions, and dashboard composition; it needs host FreeType development headers and `pkg-config`. The current dashboard test package is version `0.4.0`; see [the graphics progress note](docs/milestone-2-graphics.md) for hardware validation status.

Install the PKG on a GoldHEN-enabled PS4 using the console’s package installer. A host build cannot verify installation or on-console launch; those remain physical hardware checks.

## Project layout

- `src/platform/`: OpenOrbis platform boundary.
- `src/graphics/`: software canvas, alpha compositing, cached FreeType glyph rendering, and bounded PNG texture caching.
- `src/ui/`: fixed-capacity components, focus groups, clipping, scrolling, dialogs, and screen transitions.
- `src/input/` and `src/platform/controller.cpp`: controller event timing and the OpenOrbis ScePad adapter.
- `src/modules/dashboard.cpp`: Home, Graphics, and Developer pages using reusable UI components.
- `include/orbit/`: public interfaces.
- `config/`: default interface configuration.
- `assets/`: Quicksand font, ORBIT mark, and other visual/audio assets.
- `tests/`: isolated platform experiments.
- `sce_sys/`: PS4 package metadata, icon, and entitlement resource.
- `sce_module/`: SDK runtime modules packaged with the app.
- `docs/graphics-architecture.md`: evidence and tradeoffs for the rendering choice.

The software canvas remains the rendering backend. PNG images decode through the SDK's bundled stb_image header, with no added runtime module. Benchmark the full UI on hardware before considering any backend change.
