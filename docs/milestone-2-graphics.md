# Milestone 2 graphics progress

## Implemented in 0.2.0 (physically verified)

- Kept the existing direct SceVideoOut path, double-buffer setup, screen dimensions, and v-sync loop.
- Replaced the 5×7 diagnostic bitmap font with SDK FreeType rendering and a bundled Quicksand Medium font under its OFL license.
- Loads and rasterizes printable ASCII glyphs into caches at the four sizes used by the screen. Text drawing uses cached coverage masks, baseline metrics, kerning, multiline layout, and clipped canvas writes; it does not render glyphs or allocate during each presentation.
- Added per-pixel and rectangular opacity compositing. Fully opaque fills retain a row-oriented fast path.
- Updated the existing bring-up screen typography to use the cache and package the font under `/app0/assets/fonts`.
- Added host checks for clipped canvas writes, opaque and translucent fills, glyph metrics, anti-aliased text pixels, and clipped text.

The host font test links against host FreeType and stubs only the PS4 sysmodule-load call. The PS4 build still uses the OpenOrbis FreeType headers/libraries and `ORBIS_SYSMODULE_FREETYPE_OL`, following the installed SDK font sample. The user reports that the 0.2.0 typography checkpoint has been installed and tested on the physical jailbroken PS4.

## Implemented in 0.3.0 (host-built; console check pending)

- Added `Canvas::draw_rgba` for clipped, scaled RGBA drawing with nearest or bilinear sampling. Bilinear filtering interpolates premultiplied color to avoid colored fringes around transparent edges.
- Added vertical gradients, stroked rectangles, and rounded rectangle fills while retaining the existing canvas and coordinate space.
- Added move-only PNG `Texture` resources and a 16-entry cache with 256-byte path limits, 16 MiB per-image limit, and 32 MiB aggregate decoded-pixel limit. Cache hits allocate nothing; `clear()` releases decoder-owned memory.
- Uses SDK-bundled `stb_image.h` in PNG-only mode. Decoding happens at load time, and the presentation loop performs no image decoding or texture allocation.
- Packages `assets/images/orbit-mark.png`; the foundation screen draws it with a vector fallback if image loading fails.
- Added host tests for scaling, alpha fringe behavior, clipping, primitives, decode failure, cache hits/cleanup, and the actual ORBIT image.

The 0.3.0 package needs a physical PS4 check:

1. Install/update ORBIT and confirm the image is sharp, centered, and blends cleanly against navy, with no green/red halo around transparent edges.
2. Confirm text and status panel still render normally at 1920×1080.
3. Check for stable presentation, startup delay, or memory errors and report observed frame rate.

Reusable UI components, screen/layout management, controller navigation, focus states, and transitions remain unfinished Milestone 2/3 work.
