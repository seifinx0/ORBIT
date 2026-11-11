# ORBIT Game Library architecture

## Installed-title discovery

`library::Catalog` performs a read-only, non-recursive scan of `/user/app` and `/mnt/ext0/user/app`. Those are the internal and extended-storage app roots used by the PS4 app layout. It accepts only direct child directories whose name is a 9–12 character uppercase alphanumeric Title ID, then checks `sce_sys/param.sfo` in that directory. SFO `TITLE_ID` must match the directory name, and only `CATEGORY=gd` full-game applications enter the library. Duplicate IDs across roots are ignored. Symbolic-link app directories are rejected; no installed file is modified.

The local SDK's `libSceAppInstUtil.so` exposes existence/size functions, but its app-slot list declaration is an empty-parameter stub and does not provide a usable title enumeration contract. ORBIT therefore does not call that stub or infer a title list from it. The scanner's host fixture tests cover valid game metadata, non-game category filtering, missing artwork, ordering, and malformed paths.

Discovery is bounded to 512 titles and a 1 MiB maximum SFO file. SFO offsets, key terminators, data ranges, entry count, field capacity, folder identity, and category are checked before metadata is accepted. The catalog keeps title, Title ID, Content ID, installed version, category, and the app directory. It prefers `APP_VER` and falls back to `VERSION` only when absent; missing optional fields remain empty. Search matches title and Title ID. Sorting is by title or Title ID.

The console has not yet confirmed whether this standalone process can enumerate both roots under the installed GoldHEN configuration. If the roots cannot be opened, the application reports that storage is unavailable; it does not fabricate titles. The filesystem scan is intentionally limited to these known roots and does not recurse into arbitrary storage.

## Artwork and memory

Each accepted app may provide `sce_sys/icon0.png` and `sce_sys/pic0.png`. The icon is used as cover artwork and the pic as a full-screen hero background where available. Files are loaded only when they are on screen or selected; the complete artwork collection is not decoded at startup. `TextureCache` retains at most 16 textures and 32 MiB of decoded RGBA data, with least-recently-used eviction; each texture remains limited to 16 MiB. Invalid or absent images use an ORBIT fallback.

The current presentation still uses the existing double-buffered 1920×1080 CPU Canvas. State changes redraw both framebuffers; the normal presentation loop does not continuously recompose a static screen. Selection transitions are brief and redraw while their scale interpolation is active. PS4 frame pacing and large-image decode latency remain hardware checks.

## Favorites, collections, and settings

Preferences are stored at `/data/orbit/preferences.cfg`. The bounded versioned text format stores favorite Title IDs, up to eight named collections with up to 64 games each, browsing layout, sort order, animation intensity, and accent palette. Collection names and IDs are validated. Save writes a sibling temporary file and renames it over the preference file so an interrupted write does not replace the last complete file.

The UI has a controller-operated search keyboard, a carousel and an artwork grid, a game details view, favorites, collection membership/creation, sorting, layout/accent/animation preferences, and an explicit refresh action. It does not expose recently-added sorting because the directory/SFO scan has no trustworthy installation timestamp.

## Launch request

The launcher uses the OpenOrbis SDK declaration for `sceLncUtilLaunchApp` from `SystemService.h`, linked through `libSceLncUtil.so`. It obtains the foreground user with `sceUserServiceGetForegroundUser`, fills the SDK's `LncAppParam` with that user and `LaunchApp_None`, and passes the validated title ID. This call pattern is also visible in [etaHEN's launch implementation](https://github.com/etaHEN/etaHEN/blob/main/Source%20Code/daemon/source/main.cpp). A nonnegative return is reported only as acceptance of the request by the service; ORBIT does not claim the target game has reached its running state. Negative results are surfaced in the UI and log.

The launch API declaration and import stub exist in the installed SDK, but ORBIT's launch path has not yet been tested on the target PS4. The final test must confirm whether application permissions permit the handoff and whether the calling app suspends or exits as expected.

## Explicit exclusions and limitations

- No `app.db` parsing or privileged AppInstUtil slot enumeration.
- No edits to game installations, protected system files, or installed metadata.
- No network metadata downloads or guessed artwork.
- No playtime, ratings, descriptions, or install timestamps unless a supported source is added and verified.
- Non-ASCII names may not render correctly until the existing ASCII glyph cache is extended with a tested Unicode-capable font path.
