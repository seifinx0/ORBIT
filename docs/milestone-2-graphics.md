# Graphics and interface implementation history

This note records the verified renderer and current library UI. The graphics backend remains the original OpenOrbis SceVideoOut double-buffered CPU Canvas at 1920×1080.

## 0.2.0 typography checkpoint (user reports physical PS4 verification)

- FreeType and the bundled Quicksand Medium font.
- Cached printable ASCII glyphs at fixed screen sizes, kerning, measurement, baseline metrics, multiline layout, and clipped alpha rendering.
- Canvas alpha composition and host checks for clipping, fills, and glyph output.

## 0.3.0 image checkpoint (user reports physical PS4 verification)

- PNG-only `stb_image` decoding and move-only texture resources.
- Scaled RGBA Canvas drawing, bilinear sampling, clipping, gradients, borders, and rounded fills.
- Bounded image cache with 16 MiB per-image and 32 MiB aggregate limits.
- Existing application mark fallback.

## 0.4.0 UI checkpoint (host/native/package checks reported; physical test was not confirmed)

- Reusable fixed-capacity interface components, clipping, focus groups, controller edge/repeat timing, and screen transitions.
- Initial Home/Graphics/Developer shell, later replaced as part of the product-direction change.

## 0.5.0 Game Library implementation

- Replaced the generic command-center screen with an installed PS4 full-game library view: artwork-led carousel, artwork grid, title details, favorites, collection navigation, search, sort, refresh, and customization.
- Added bounded, read-only SFO catalog scanning of `/user/app` and `/mnt/ext0/user/app`. Full-game `gd` entries are validated against folder Title IDs. Duplicate IDs are skipped and missing/invalid metadata is excluded.
- Added favorite/collection/settings persistence at `/data/orbit/preferences.cfg` with validated values and temp-file rename.
- Added foreground-user title launch requests through the SDK-declared `sceLncUtilLaunchApp` linked from `libSceLncUtil.so`. A successful function return is described as request acceptance, not proof that the game started.
- Added controller-operated search and collection-name keyboards, grid row movement, context actions, an LRU texture cache, and selection scaling interpolation.
- Added host tests for installed-title SFO scanning, category exclusion, missing artwork, search support primitives, preferences round-trip, and the existing renderer/UI suite.

## Remaining physical validation

The 0.5.0 release candidate still needs testing on the target PS4. Validate app access to internal and extended storage, detected title count/metadata, artwork path compatibility, favorites/collection persistence after restart, grid/carousel navigation, search, refresh, controller-only use, LNC launch handoff, frame pacing during selection changes, and memory stability with a large library. See [build validation](build-validation.md) and [game library architecture](game-library-architecture.md).
