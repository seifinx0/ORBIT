# ORBIT

ORBIT is a native OpenOrbis PS4 homebrew command center. This fresh project currently contains the Milestone 1 platform foundation: a native SceVideoOut application that allocates two 1920×1080 buffers, draws a branded bring-up screen, and presents them on v-sync.

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

Install the PKG on a GoldHEN-enabled PS4 using the console’s package installer. A host build cannot verify installation or on-console launch; those remain physical hardware checks.

## Project layout

- `src/platform/`: OpenOrbis platform boundary.
- `src/graphics/`: software canvas primitives for the foundation renderer.
- `include/orbit/`: public interfaces.
- `config/`: default interface configuration.
- `assets/`: visual and audio assets as they are introduced.
- `tests/`: isolated platform experiments.
- `sce_sys/`: PS4 package metadata, icon, and entitlement resource.
- `sce_module/`: SDK runtime modules packaged with the app.
- `docs/graphics-architecture.md`: evidence and tradeoffs for the rendering choice.

The software canvas is intentionally a small foundation. Use the direct video output backend until physical tests establish its reliability and performance envelope; benchmark the full UI before choosing a GPU backend.
