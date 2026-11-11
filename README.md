# ORBIT Game Library

ORBIT is a standalone native PS4 game library and launcher. It keeps the verified OpenOrbis SceVideoOut renderer and presents installed games through a cinematic artwork carousel or a controller-navigable cover grid. It reads installed title metadata, offers game details, favorites, user-created collections, search, sorting, library customization, and launch requests through the PS4 title-launch service.

ORBIT is an application launched from the PS4 home screen. It does not replace the system shell. Version 0.5.0 is an internally validated release candidate; the revised game discovery and launch path still needs final physical PS4 testing.

## Build

Requirements: OpenOrbis SDK, Clang, LLD, Make, local .NET 8 at `~/.dotnet`, and LibOrbisPkg source at `../Tools/LibOrbisPkg` beside the SDK.

```sh
export OO_PS4_TOOLCHAIN="$HOME/Downloads/PS4-Dev/SDK"
export DOTNET_ROOT="$HOME/.dotnet"
export PATH="$DOTNET_ROOT:$PATH"
make test-host
make
make inspect
```

The package is written to `build/IV0000-ORBT00001_00-ORBITFOUNDATION1.pkg`. The existing Makefile publishes the local LibOrbisPkg `PkgTool.Core` as a self-contained Linux x64 executable under `build/tools/publish/` and uses it for SFO generation, package creation, listing, and validation. Set `LIBORBISPKG_ROOT=/path/to/LibOrbisPkg` if the checkout is elsewhere. This avoids the bundled .NET Core 3.0 executable and does not change system-wide .NET or OpenSSL.

`make test-host` covers canvas composition, FreeType rendering, PNG decoding/cache behavior, UI components, controller repeat timing, catalog/SFO parsing, collection/favorite persistence, and dashboard composition. Native builds use the installed OpenOrbis SDK; package inspection validates the PKG and lists its entries. A successful host or package check cannot establish that `/user/app` is readable or that a title launch is accepted on a particular console.

## Controller use

- D-pad / left stick: browse; in grid mode, move across and between rows.
- Cross: open a game's details, then launch from the details view.
- Circle: return to the previous library view.
- L1 / R1: move through library categories; while the action menu is open, choose a target collection.
- Options: favorites, collection membership/creation, layout, sorting, refresh, and settings.
- Search uses an on-screen controller keyboard; no external keyboard is required.

## Project layout

- `src/library/`: bounded installed-title catalog, SFO reader, and persisted favorites/collections/preferences.
- `src/platform/title_launcher.cpp`: foreground-user lookup and SceLncUtil title-launch request.
- `src/graphics/`: SceVideoOut CPU canvas, FreeType glyph cache, PNG decoding, and bounded LRU texture cache.
- `src/input/` and `src/platform/controller.cpp`: controller event timing and OpenOrbis ScePad adapter.
- `src/modules/dashboard.cpp`: current ORBIT Game Library screens and controller interactions.
- `src/ui/`: reusable fixed-capacity interface components retained for future library views.
- `assets/`: Quicksand font, ORBIT mark, and application assets.
- `tests/`: host-side checks for rendering and independent library logic.
- `sce_sys/` and `sce_module/`: PS4 package metadata and required runtime resources.
- `docs/game-library-architecture.md`: discovery, metadata, artwork, persistence, launch path, and limits.
- `docs/graphics-architecture.md`: renderer research and tradeoffs.

## Known boundaries before final console validation

- Discovery scans full-game (`CATEGORY=gd`) title folders on internal `/user/app` and extended `/mnt/ext0/user/app`, then reads each folder's `sce_sys/param.sfo` and available `icon0.png` / `pic0.png`. Access and installed-title coverage depend on the console's application permissions and storage mounts.
- The metadata reader does not query the system application database. It does not invent playtime, ratings, install dates, or descriptions. “Recently added” sorting is not offered because this scan does not provide a trustworthy install timestamp.
- Artwork enrichment is offline and uses the installed title's own icon/background. Missing assets use a neutral ORBIT fallback.
- Quicksand's current cached glyph set is printable ASCII; non-ASCII title names may display fallback glyphs.
- The SceLncUtil API is declared and linked by the installed SDK, but discovery access, launch acceptance, handoff behavior, artwork quality, animation pacing, and memory use need evaluation on the target PS4.

See [build validation](docs/build-validation.md) and [game library architecture](docs/game-library-architecture.md).
