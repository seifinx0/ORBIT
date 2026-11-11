# Graphics architecture decision

**Milestone 1 backend: direct SceVideoOut scanout with a small CPU canvas.**

The installed OpenOrbis SDK (February 2025 tree) includes executable samples and link stubs for SceVideoOut, Piglet, FreeType, SDL2, and PNG decoding. The decision is based on those samples and the installed artifacts, not header presence alone.

| Path | Evidence in this SDK | Cost and tradeoff | Decision |
| --- | --- | --- | --- |
| SceVideoOut | `samples/graphics` implements 1080p buffers, direct-memory allocation, buffer registration, flip events, and v-sync; it links against `libSceVideoOut.so`. | Small runtime dependency set and full control. Rasterization is CPU-side, so future animated/high-fill UI needs measurement and likely a GPU backend. | Use for the first reliable native application and simple UI primitives. |
| Piglet EGL / GLES 2 | `samples/piglet` initializes EGL, loads Piglet and precompiled shader modules, and issues GLES rendering. Headers and stubs are installed. | More complex module/context setup. The sample reserves 250 MiB system shared memory and 512 MiB video shared memory, with another 170 MiB flexible mapping limit; the SDK README still lists GPU rendering support as unfinished. | Revisit only after validating a modest memory profile on the target PS4. |
| SDL2 | `samples/SDL2` is a PS4-specific SDL port, linked with SDL2, video, pad, audio, sysmodule, FreeType, and SDL2_image stubs. | Convenient input and abstraction, but a much larger dependency surface for a foundation app. | Not selected as the platform core; can be reconsidered for selected higher-level utilities. |
| FreeType | `samples/font` combines SceVideoOut and the FreeType sysmodule, rasterizing a packaged TTF onto the CPU framebuffer. | Good typography path; font load, glyph cache, and memory use need to be controlled. | Implemented in 0.2.0 with cached printable ASCII masks at the screen's fixed font sizes. |
| PNG decoding | `samples/pngdec` uses the SDK `_common/png` helper; Piglet uses the bundled stb decoder. | Header-only CPU decode adds code size and temporary load-time memory; bounded dimensions and a fixed cache cap decoded memory. | Implemented using SDK `stb_image.h` in PNG-only mode, with no additional runtime module. |

The user reports the 0.2.0 FreeType and alpha-compositing checkpoint was installed and physically verified. The 0.3.0 image path retains CPU-side compositing and needs a new GoldHEN PS4 check, particularly for startup memory and sustained frame pacing.
