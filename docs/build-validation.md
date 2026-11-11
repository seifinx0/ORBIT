# Build validation status

Milestone 1 was built on Kali GNU/Linux Rolling (2026.3), using the OpenOrbis SDK in `~/Downloads/PS4-Dev/SDK`, Clang 21.1.8, LLD 21.1.8, and the existing local .NET SDK/runtime 8.0.428 / 8.0.34 at `~/.dotnet`.

## PkgTool compatibility fix

The SDK-bundled Linux `PkgTool.Core` targets .NET Core 3.0. Its package-building crypto initialization reports `No usable version of libssl was found` on this host, which has .NET 6 and OpenSSL 3. SFO commands happened to run in the old executable, but `pkg_build` did not.

The working replacement was reproduced from the local LibOrbisPkg checkout. The actual project is `~/Downloads/PS4-Dev/Tools/LibOrbisPkg/PkgTool.Core/PkgTool.Core.csproj`; shared `Directory.Build.props` selects `net8.0`. ORBIT's Makefile:

1. Reads the LibOrbisPkg checkout at `$(SDK)/../Tools/LibOrbisPkg` (override with `LIBORBISPKG_ROOT`).
2. Archives its committed source into the ignored `build/tools/source` tree, keeping generated files out of the tooling checkout.
3. Uses `DOTNET_ROOT=~/.dotnet` to publish Linux x64 self-contained and single-file to `build/tools/publish/PkgTool.Core`.
4. Uses that executable for `sfo_new`, `sfo_setentry`, `pkg_build`, package listing, and validation. No system runtime or OpenSSL changes are needed.

The replacement was executed successfully for `sfo_new`, multiple `sfo_setentry` operations, `sfo_listentries`, and the actual ORBIT `pkg_build`. The first package attempt exposed a malformed Content ID suffix in the initial project: it was 15 characters instead of the required 16. ORBIT now uses `IV0000-ORBT00001_00-ORBITFOUNDATION1` (36 characters), and the SFO Title ID is `ORBT00001`.

## Package validation

- Output: `build/IV0000-ORBT00001_00-ORBITFOUNDATION1.pkg`
- Size: 6,619,136 bytes.
- `pkg_validate` passed the package size limit, all listed content and entry hashes, PFS/body digests, and signatures.
- Extracted `PARAM_SFO` reports Title ID `ORBT00001`, Content ID `IV0000-ORBT00001_00-ORBITFOUNDATION1`, and title `ORBIT Command Center`.
- Extracted package payloads matched their build inputs byte-for-byte: `eboot.bin`, `icon0.png`, `sce_sys/about/right.sprx`, `sce_module/libc.prx`, and `sce_module/libSceFios2.prx`.
- `pkg_listentries` and `pkg_extract` confirmed the package metadata and inner payload. The package builder also generates the expected keystone entry.

The user reports the Milestone 1 and 0.2.0 FreeType typography builds were installed and tested on a physical PS4 at 1920×1080; the original foundation was observed at approximately 59.95 FPS.

The 0.2.0 package kept the same output path and Content ID, set `APP_VER` to `0.2.0`, and included the Quicksand font plus OFL notice.

The 0.3.0 package preserves that identity, adds `assets/images/orbit-mark.png`, and contains the cached PNG decoder and RGBA renderer. `make test-host`, the native ELF/FSELF build, `pkg_build`, and `make inspect` passed. `pkg_validate` accepted every listed hash, digest, and signature. Extracted `PARAM_SFO` reports `APP_VER=0.3.0`, `TITLE_ID=ORBT00001`, and `CONTENT_ID=IV0000-ORBT00001_00-ORBITFOUNDATION1`. Extraction confirmed `eboot.bin`, the Quicksand font and license, and the ORBIT PNG match their build inputs byte-for-byte. The package is 6,619,136 bytes at `build/IV0000-ORBT00001_00-ORBITFOUNDATION1.pkg`. This graphics update has not yet been tested on physical hardware; see [Milestone 2 progress](milestone-2-graphics.md).
