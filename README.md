# ORBIT

ORBIT is a native OpenOrbis PS4 homebrew command center. This fresh project currently contains the Milestone 1 platform foundation: a native SceVideoOut application that allocates two 1920×1080 buffers, draws a branded bring-up screen, and presents them on v-sync.

## Build

Requirements: OpenOrbis SDK, Clang, LLD, Make, and a compatible runtime for the SDK package tools. The SDK’s Linux `PkgTool.Core` targets .NET Core 3.0 and requires a compatible OpenSSL `libssl` version.

```sh
export OO_PS4_TOOLCHAIN="$HOME/Downloads/PS4-Dev/SDK"
make
```

The package is written to `build/IV0000-ORBT00001_00-ORBITFOUNDATION.pkg`. `make inspect` lists its package entries. The SDK’s Linux `PkgTool.Core` needs `DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1` on systems without ICU; the Makefile sets this only for package tool invocations. On the current host, `pkg_build` aborts with `No usable version of libssl was found`: only .NET 6 and OpenSSL 3 are installed. The ELF/FSELF build works, but producing a PKG needs a compatible runtime.

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
