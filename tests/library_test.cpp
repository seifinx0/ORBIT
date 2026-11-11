#include "orbit/library/game.hpp"
#include "orbit/library/preferences.hpp"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {
void put16(std::array<unsigned char, 2048>& b, std::size_t p, unsigned v) { b[p] = v & 255; b[p + 1] = (v >> 8) & 255; }
void put32(std::array<unsigned char, 2048>& b, std::size_t p, unsigned v) { for (int i = 0; i < 4; ++i) b[p + i] = (v >> (8 * i)) & 255; }
std::size_t make_sfo(std::array<unsigned char, 2048>& b, const char* title, const char* id, const char* category) {
    const char* keys[] = {"APP_VER", "CATEGORY", "TITLE", "TITLE_ID", "VERSION"};
    const char* vals[] = {"02.45", category, title, id, "01.23"};
    const std::size_t count = 5, keyoff = 20 + count * 16, dataoff = keyoff + 40;
    std::memset(b.data(), 0, b.size()); b[1] = 0x50; b[2] = 0x53; b[3] = 0x46;
    put32(b, 4, 0x101); put32(b, 8, keyoff); put32(b, 12, dataoff); put32(b, 16, count);
    std::size_t ko = 0, vo = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t e = 20 + i * 16;
        put16(b, e, ko); put16(b, e + 2, 0x204);
        const auto len = std::strlen(vals[i]) + 1;
        const unsigned maxlen = (i == 2 ? 128 : ((i == 3 || i == 4) ? 12 : 8));
        put32(b, e + 4, len); put32(b, e + 8, maxlen); put32(b, e + 12, vo);
        std::memcpy(b.data() + keyoff + ko, keys[i], std::strlen(keys[i]) + 1);
        std::memcpy(b.data() + dataoff + vo, vals[i], len);
        ko += std::strlen(keys[i]) + 1; vo += maxlen;
    }
    return dataoff + vo;
}
void make_app(const std::string& root, const char* folder, const char* title, const char* id, const char* cat) {
    const std::string dir = root + "/" + folder + "/sce_sys";
    assert(mkdir((root + "/" + folder).c_str(), 0700) == 0);
    assert(mkdir(dir.c_str(), 0700) == 0);
    std::array<unsigned char, 2048> bytes{}; const auto size = make_sfo(bytes, title, id, cat);
    FILE* f = std::fopen((dir + "/param.sfo").c_str(), "wb"); assert(f); assert(std::fwrite(bytes.data(), 1, size, f) == size); std::fclose(f);
    FILE* icon = std::fopen((dir + "/icon0.png").c_str(), "wb"); assert(icon); std::fputs("fixture", icon); std::fclose(icon);
}
void cleanup_app(const std::string& root, const char* folder) {
    const std::string dir = root + "/" + folder + "/sce_sys";
    unlink((dir + "/param.sfo").c_str()); unlink((dir + "/icon0.png").c_str());
    rmdir(dir.c_str()); rmdir((root + "/" + folder).c_str());
}
}

int main() {
    char current[512]{}; assert(getcwd(current, sizeof(current)));
    char root_template[640]{}; std::snprintf(root_template, sizeof(root_template), "%s/build/tests/orbit-library-XXXXXX", current);
    char* root = mkdtemp(root_template); assert(root);
    const std::string base(root);
    make_app(base, "CUSA00001", "Alpha Game", "CUSA00001", "gd");
    make_app(base, "CUSA00002", "Not a Game", "CUSA00002", "gde");
    make_app(base, "CUSA00003", "Beta Game", "CUSA00003", "gd");
    assert(mkdir((base + "/CUSA00004").c_str(), 0700) == 0);
    assert(mkdir((base + "/CUSA00004/sce_sys").c_str(), 0700) == 0);
    FILE* invalid = std::fopen((base + "/CUSA00004/sce_sys/param.sfo").c_str(), "wb"); assert(invalid); std::fputs("bad SFO", invalid); std::fclose(invalid);
    orbit::library::Catalog catalog;
    auto scan = catalog.scan(base.c_str());
    assert(scan.status == orbit::library::ScanResult::Status::Ok && scan.discovered == 2);
    assert(catalog.size() == 2 && std::strcmp(catalog.at(0)->title.data(), "Alpha Game") == 0);
    assert(std::strcmp(catalog.at(0)->version.data(), "02.45") == 0);
    assert(catalog.at(0)->has_icon && !catalog.at(0)->has_background);
    assert(catalog.find("CUSA00003") && !catalog.find("CUSA00002"));
    const char* duplicate_roots[] = {base.c_str(), base.c_str()};
    assert(catalog.scan_roots(duplicate_roots, 2).discovered == 2);
    assert(catalog.scan("relative").status == orbit::library::ScanResult::Status::InvalidPath);

    orbit::library::Preferences prefs;
    assert(prefs.toggle_favorite("CUSA00001") && prefs.favorite("CUSA00001"));
    assert(prefs.add_collection("Favorites Plus"));
    assert(prefs.add_to_collection(0, "CUSA00003") && prefs.in_collection(0, "CUSA00003"));
    assert(!prefs.add_collection("bad/name"));
    prefs.layout = orbit::library::Layout::Grid; prefs.animation_level = 1; prefs.accent = 3;
    const std::string pref_path = base + "/prefs.cfg";
    assert(prefs.save(pref_path.c_str()));
    orbit::library::Preferences loaded;
    assert(loaded.load(pref_path.c_str()));
    assert(loaded.favorite("CUSA00001") && loaded.in_collection(0, "CUSA00003"));
    assert(loaded.layout == orbit::library::Layout::Grid && loaded.animation_level == 1 && loaded.accent == 3);
    FILE* bad = std::fopen((base + "/bad.cfg").c_str(), "wb"); assert(bad); std::fputs("NOT-ORBIT\n", bad); std::fclose(bad);
    assert(!loaded.load((base + "/bad.cfg").c_str()));
    unlink((base + "/prefs.cfg").c_str()); unlink((base + "/bad.cfg").c_str());
    unlink((base + "/CUSA00004/sce_sys/param.sfo").c_str()); rmdir((base + "/CUSA00004/sce_sys").c_str()); rmdir((base + "/CUSA00004").c_str());
    cleanup_app(base, "CUSA00001"); cleanup_app(base, "CUSA00002"); cleanup_app(base, "CUSA00003"); rmdir(base.c_str());
    return 0;
}
