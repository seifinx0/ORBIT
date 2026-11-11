# Milestone 2 graphics progress

## Implemented in 0.2.0

- Kept the existing direct SceVideoOut path, double-buffer setup, screen dimensions, and v-sync loop.
- Replaced the 5×7 diagnostic bitmap font with SDK FreeType rendering and a bundled Quicksand Medium font under its OFL license.
- Loads and rasterizes printable ASCII glyphs into caches at the four sizes used by the screen. Text drawing uses cached coverage masks, baseline metrics, kerning, multiline layout, and clipped canvas writes; it does not render glyphs or allocate during each presentation.
- Added per-pixel and rectangular opacity compositing. Fully opaque fills retain a row-oriented fast path.
- Updated the existing bring-up screen typography to use the cache and package the font under `/app0/assets/fonts`.
- Added host checks for clipped canvas writes, opaque and translucent fills, glyph metrics, anti-aliased text pixels, and clipped text.

The host font test links against host FreeType and stubs only the PS4 sysmodule-load call. The PS4 build still uses the OpenOrbis FreeType headers/libraries and `ORBIS_SYSMODULE_FREETYPE_OL`, following the installed SDK font sample.

## Hardware check needed

Milestone 1 was previously reported as installed and physically verified at 1920×1080 and approximately 59.95 FPS. That result applies to the original foundation. This 0.2.0 typography build has not yet been tested on-console.

For the new PKG, install/update ORBIT and confirm:

1. The ORBIT wordmark and status copy render with smooth edges and correct spacing.
2. All four text rows are readable, with no missing glyphs, corruption, or clipping.
3. The display remains at 1920×1080 and the static screen continues to present smoothly.
4. Report the observed frame rate and any boot delay or memory error.

Textures, image decoding, reusable UI components, screen/layout management, controller navigation, focus states, and transitions remain future Milestone 2 work.
