# Milestone 2 graphics progress

## Implemented in 0.2.0 (physically verified)

- Kept the existing direct SceVideoOut path, double-buffer setup, screen dimensions, and v-sync loop.
- Replaced the 5×7 diagnostic bitmap font with SDK FreeType rendering and a bundled Quicksand Medium font under its OFL license.
- Loads and rasterizes printable ASCII glyphs into caches at the four sizes used by the screen. Text drawing uses cached coverage masks, baseline metrics, kerning, multiline layout, and clipped canvas writes; it does not render glyphs or allocate during each presentation.
- Added per-pixel and rectangular opacity compositing. Fully opaque fills retain a row-oriented fast path.
- Updated the existing bring-up screen typography to use the cache and package the font under `/app0/assets/fonts`.
- Added host checks for clipped canvas writes, opaque and translucent fills, glyph metrics, anti-aliased text pixels, and clipped text.

The host font test links against host FreeType and stubs only the PS4 sysmodule-load call. The PS4 build still uses the OpenOrbis FreeType headers/libraries and `ORBIS_SYSMODULE_FREETYPE_OL`, following the installed SDK font sample. The user reports that the 0.2.0 typography checkpoint has been installed and tested on the physical jailbroken PS4.

## Implemented in 0.3.0 (physically verified)

- Added `Canvas::draw_rgba` for clipped, scaled RGBA drawing with nearest or bilinear sampling. Bilinear filtering interpolates premultiplied color to avoid colored fringes around transparent edges.
- Added vertical gradients, stroked rectangles, and rounded rectangle fills while retaining the existing canvas and coordinate space.
- Added move-only PNG `Texture` resources and a 16-entry cache with 256-byte path limits, 16 MiB per-image limit, and 32 MiB aggregate decoded-pixel limit. Cache hits allocate nothing; `clear()` releases decoder-owned memory.
- Uses SDK-bundled `stb_image.h` in PNG-only mode. Decoding happens at load time, and the presentation loop performs no image decoding or texture allocation.
- Packages `assets/images/orbit-mark.png`; the foundation screen draws it with a vector fallback if image loading fails.
- Added host tests for scaling, alpha fringe behavior, clipping, primitives, decode failure, cache hits/cleanup, and the actual ORBIT image.

The user reports that v0.3.0 has been installed and successfully tested on physical PS4 hardware. The confirmed results include 1920×1080 SceVideoOut, double-buffered v-sync, cached FreeType/Quicksand text, PNG texture loading, clipped/scaled alpha composition, canvas primitives, and the packaged ORBIT mark.

## Implemented in 0.4.0 (host-built; console check pending)

- Added fixed-capacity reusable `Panel`, `Card`, `Button`, `Label`, `Image`, `Separator`, `ProgressBar`, `ScrollArea`, `Dialog`, and `Notification` components. Components use the existing Canvas, FontRenderer, and Texture interfaces; no child tree allocations occur while rendering.
- Added nested Canvas clipping, used by labels, component containers, and scroll areas.
- Added focus groups that skip hidden/disabled buttons, Cross activation, and a fixed-capacity screen registry with navigation and a restrained fade-through-dark transition.
- Added an OpenOrbis controller adapter based on the SDK input sample (`sceUserServiceGetInitialUser`, `scePadOpen`, `scePadReadState`). D-pad and left-stick movement use edge detection, 360 ms initial repeat, then 95 ms repeat. Cross confirms, Circle returns, L1/R1 changes pages, and Options opens controller help. The app remains visible if pad startup fails.
- Replaced the static foundation screen with Home, Graphics, and Developer pages. They show verified renderer/build details only; live system memory, network, and installed-app data are not claimed. Static pages redraw both buffers only after an input/state change; during the short screen transition both buffers are updated each v-sync step.
- Added host coverage for clipping, widget focus and callbacks, scroll clipping, controller repeat, screen transitions, and dashboard composition.

The v0.4.0 native package needs a physical PS4 check:

1. Install/update ORBIT. Use D-pad and left stick to move the focus ring; Cross should open the focused page and Circle should return to Home.
2. L1/R1 should cycle Home, Graphics, and Developer with a short fade. Hold a direction and confirm it repeats after a brief pause without repeating Cross.
3. Options should open the controller guide; Circle, Cross, or Options should close it.
4. Confirm text, logo, panels, and focus states stay clipped and legible at 1920×1080. Report observed frame pacing, startup delay, and any controller or memory errors.

The dashboard is an initial functional shell over verified capabilities. Homebrew discovery/launching, file operations, system statistics, network diagnostics, customization, extension management, and richer settings remain future work.
