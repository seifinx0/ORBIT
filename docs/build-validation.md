# Build validation status

Build was run on Kali GNU/Linux Rolling (2026.3) with the OpenOrbis SDK in `~/Downloads/PS4-Dev/SDK`, Clang 21.1.8, LLD 21.1.8, and .NET 6.0.8.

Completed successfully:

- Compiled all three C++ translation units for `x86_64-pc-freebsd12-elf`.
- Linked `build/orbit.elf` against the OpenOrbis CRT, libc, libc++, kernel, and SceVideoOut stubs.
- Produced `build/orbit.oelf` and root `eboot.bin` with `create-fself`.
- Generated `sce_sys/param.sfo` and `pkg.gp4` with the SDK tools.

Blocked at package creation:

`PkgTool.Core pkg_build pkg.gp4 build` aborts with `No usable version of libssl was found`. The bundled Linux package tool targets .NET Core 3.0; this host has only .NET 6 and OpenSSL 3 (`libssl.so.3`). The SFO actions run, but package building initializes the incompatible crypto runtime. No PKG was produced.

Physical install, launch, controller behavior, and frame pacing have not been tested because a PS4 console is not attached to this development environment.
