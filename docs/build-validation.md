# Build and validation record

## Existing build pipeline

Milestone 1 was built on Kali using the OpenOrbis SDK, Clang, LLD, and local .NET 8 at `~/.dotnet`. The existing Makefile publishes LibOrbisPkg's `PkgTool.Core` into the ignored `build/tools/publish/` directory as a self-contained Linux x64 single-file executable. The source checkout is copied into `build/tools/source/`; the SDK checkout is not modified. This avoids the bundled .NET Core 3.0 executable's OpenSSL 3 incompatibility without system-wide runtime changes.

The package keeps Title ID `ORBT00001` and Content ID `IV0000-ORBT00001_00-ORBITFOUNDATION1`. The user reports that the foundation and v0.3.0 texture checkpoint were installed and successfully tested on physical PS4 hardware. The reported v0.4.0 host/native/package checks did not include a confirmed physical test.

## Current release candidate: v0.5.0

Validation performed for this implementation:

- `make test-host`: passed. Canvas, FreeType, PNG drawing/cache (including 17-image LRU eviction), controller timing, UI, dashboard, installed-title SFO scan fixtures, and preference persistence tests all passed.
- OpenOrbis native ELF build and FSELF generation: passed. ELF size report: 548,631 bytes text, 15,952 bytes data, 5,488 bytes BSS.
- `pkg_build`: passed using the existing self-contained `PkgTool.Core` under `build/tools/publish/`.
- `pkg_validate`: all content hashes, PFS digests, PKG hashes, and signatures passed.
- `pkg_extract --verbose`: passed. Extracted EBOOT and all packaged app assets/runtime modules. SHA-256 matched `eboot.bin`, both Quicksand files, ORBIT mark, libc PRX, and Fios2 PRX byte-for-byte.
- Extracted outer `PARAM_SFO` entry and read it with `sfo_listentries`: `APP_VER=0.5.0`, `TITLE=ORBIT Game Library`, `TITLE_ID=ORBT00001`, and `CONTENT_ID=IV0000-ORBT00001_00-ORBITFOUNDATION1`.
- Package size: 6,619,136 bytes.
- Physical console test: not performed for v0.5.0.

The release candidate package target is:

`build/IV0000-ORBT00001_00-ORBITFOUNDATION1.pkg`

The outer PKG entry list contains Sony metadata and `ICON0_PNG`; the payload files live in the inner PFS under `/uroot`. `pkg_extract` confirmed the packaged inner payload and `pkg_extractentry` extracted `PARAM_SFO` (entry index 9). These host-side checks do not prove that game discovery or title launch works on console.

## Final PS4 evaluation checklist

1. Install the v0.5.0 package and confirm ORBIT opens at 1920×1080 with the controller.
2. Compare the discovered title list against installed full games. Check internal and extended storage, title names, IDs, versions, icons, and backgrounds.
3. Browse the carousel and grid quickly, use row navigation, search by title and ID, sort by title/ID, and refresh after title changes.
4. Favorite a game, create more than one collection, change membership, restart ORBIT, and confirm those settings persist.
5. Open a title detail view and launch a game. Record the exact response, whether ORBIT suspends/exits, and whether the game reaches its running screen.
6. Observe frame pacing during selection animation, artwork transitions, and rapid scrolling; test missing/corrupt artwork and a large library.

Do not report the v0.5.0 release as production verified until these target-console checks pass.
