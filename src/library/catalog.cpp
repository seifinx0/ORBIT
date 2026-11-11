#include "orbit/library/game.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

namespace orbit::library {
namespace {
constexpr std::size_t kMaxSfoBytes = 1024u * 1024u;

unsigned short u16(const unsigned char* p) noexcept { return static_cast<unsigned short>(p[0] | (p[1] << 8)); }
unsigned int u32(const unsigned char* p) noexcept {
    return static_cast<unsigned int>(p[0]) | (static_cast<unsigned int>(p[1]) << 8) |
           (static_cast<unsigned int>(p[2]) << 16) | (static_cast<unsigned int>(p[3]) << 24);
}
bool valid_title_id(const char* id) noexcept {
    if (!id || std::strlen(id) < 9 || std::strlen(id) > 12) return false;
    for (const char* p = id; *p; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))) return false;
    return true;
}
bool file_exists(const char* path) noexcept {
    struct stat st{};
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}
bool read_text_file(const char* path, unsigned char* buffer, std::size_t capacity, std::size_t& length) noexcept {
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    const std::size_t n = std::fread(buffer, 1, capacity, file);
    const bool overflow = !std::feof(file);
    std::fclose(file);
    if (overflow || n == 0) return false;
    length = n;
    return true;
}
bool copy_field(const ParamSfo& sfo, const char* key, char* out, std::size_t cap) noexcept {
    return sfo.text(key, out, cap);
}
}

bool ParamSfo::parse(const unsigned char* b, std::size_t n) noexcept {
    count_ = 0;
    if (!b || n < 20 || n > kMaxSfoBytes || b[0] != 0x00 || b[1] != 0x50 || b[2] != 0x53 || b[3] != 0x46) return false;
    const std::size_t key_off = u32(b + 8), data_off = u32(b + 12), entries = u32(b + 16);
    if (entries > entries_.size() || key_off < 20 || key_off > n || data_off < key_off || data_off > n ||
        entries > (n - 20) / 16) return false;
    const std::size_t table_end = 20 + entries * 16;
    if (table_end > key_off) return false;
    for (std::size_t i = 0; i < entries; ++i) {
        const unsigned char* e = b + 20 + i * 16;
        const std::size_t ko = u16(e), len = u32(e + 4), maxlen = u32(e + 8), vo = u32(e + 12);
        const unsigned short format = u16(e + 2);
        if (ko >= data_off - key_off || len > maxlen || vo > n - data_off || len > n - data_off - vo) return false;
        const unsigned char* key = b + key_off + ko;
        const std::size_t key_limit = data_off - (key_off + ko);
        const unsigned char* terminator = static_cast<const unsigned char*>(std::memchr(key, 0, key_limit));
        if (!terminator) return false;
        const std::size_t keylen = static_cast<std::size_t>(terminator - key);
        if (keylen == 0 || keylen >= entries_[i].key.size()) return false;
        if (format != 0x204 && format != 0x4 && format != 0x404) continue;
        Entry& dst = entries_[count_++];
        std::memcpy(dst.key.data(), key, keylen);
        dst.value = b + data_off + vo;
        dst.length = len;
        dst.format = format;
    }
    return true;
}

bool ParamSfo::text(const char* key, char* out, std::size_t cap) const noexcept {
    if (!key || !out || cap == 0) return false;
    out[0] = '\0';
    for (std::size_t i = 0; i < count_; ++i) {
        const Entry& e = entries_[i];
        if (e.format != 0x204 && e.format != 0x4) continue;
        if (std::strcmp(e.key.data(), key) != 0 || e.length == 0) continue;
        std::size_t len = e.length;
        if (e.value[len - 1] == 0) --len;
        if (len >= cap) return false;
        std::memcpy(out, e.value, len);
        out[len] = '\0';
        return true;
    }
    return false;
}

void Catalog::clear() noexcept { games_ = {}; count_ = 0; }

const Game* Catalog::find(const char* id) const noexcept {
    if (!id) return nullptr;
    for (std::size_t i = 0; i < count_; ++i) if (std::strcmp(games_[i].title_id.data(), id) == 0) return &games_[i];
    return nullptr;
}

ScanResult Catalog::scan(const char* root) noexcept {
    const char* roots[] = {root};
    return scan_roots(roots, 1);
}

ScanResult Catalog::scan_roots(const char* const* roots, std::size_t root_count) noexcept {
    clear();
    ScanResult result{};
    bool scanned_any = false;
    if (!roots || root_count == 0) { result.status = ScanResult::Status::InvalidPath; return result; }
    for (std::size_t root_index = 0; root_index < root_count; ++root_index) {
    const char* root = roots[root_index];
    if (!root || root[0] != '/') { result.status = ScanResult::Status::InvalidPath; continue; }
    DIR* dir = opendir(root);
    if (!dir) continue;
    scanned_any = true;
    while (dirent* item = readdir(dir)) {
        const std::size_t folder_id_len = std::strlen(item->d_name);
        if (folder_id_len < 9 || folder_id_len > 12 || !valid_title_id(item->d_name)) continue;
        char app_path[256]{};
        if (std::snprintf(app_path, sizeof(app_path), "%s/%s", root, item->d_name) >= static_cast<int>(sizeof(app_path))) { ++result.rejected; continue; }
        struct stat st{};
        if (lstat(app_path, &st) != 0 || !S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) continue;
        char sfo_path[320]{};
        if (std::snprintf(sfo_path, sizeof(sfo_path), "%s/sce_sys/param.sfo", app_path) >= static_cast<int>(sizeof(sfo_path))) { ++result.rejected; continue; }
        auto* bytes = static_cast<unsigned char*>(std::malloc(kMaxSfoBytes));
        if (!bytes) { ++result.rejected; continue; }
        std::size_t length = 0;
        ParamSfo sfo;
        const bool parsed = read_text_file(sfo_path, bytes, kMaxSfoBytes, length) && sfo.parse(bytes, length);
        if (!parsed) { std::free(bytes); ++result.rejected; continue; }
        Game candidate{};
        char category[4]{};
        if (!copy_field(sfo, "TITLE", candidate.title.data(), candidate.title.size()) ||
            !copy_field(sfo, "TITLE_ID", candidate.title_id.data(), candidate.title_id.size()) ||
            !copy_field(sfo, "CATEGORY", category, sizeof(category)) || !valid_title_id(candidate.title_id.data())) { std::free(bytes); ++result.rejected; continue; }
        if (std::strcmp(category, "gd") != 0) { std::free(bytes); continue; } // Installed full-game applications only.
        if (std::strcmp(candidate.title_id.data(), item->d_name) != 0) { std::free(bytes); ++result.rejected; continue; }
        if (find(candidate.title_id.data())) { std::free(bytes); continue; }
        if (!copy_field(sfo, "CONTENT_ID", candidate.content_id.data(), candidate.content_id.size())) candidate.content_id[0] = '\0';
        if (!copy_field(sfo, "APP_VER", candidate.version.data(), candidate.version.size()) &&
            !copy_field(sfo, "VERSION", candidate.version.data(), candidate.version.size())) candidate.version[0] = '\0';
        std::free(bytes);
        std::snprintf(candidate.category.data(), candidate.category.size(), "%s", category);
        std::snprintf(candidate.install_path.data(), candidate.install_path.size(), "%s", app_path);
        std::snprintf(candidate.icon_path.data(), candidate.icon_path.size(), "%s/sce_sys/icon0.png", app_path);
        std::snprintf(candidate.background_path.data(), candidate.background_path.size(), "%s/sce_sys/pic0.png", app_path);
        candidate.has_icon = file_exists(candidate.icon_path.data());
        candidate.has_background = file_exists(candidate.background_path.data());
        if (count_ == games_.size()) { result.truncated = true; continue; }
        games_[count_++] = candidate;
        ++result.discovered;
    }
    closedir(dir);
    }
    if (scanned_any) result.status = ScanResult::Status::Ok;
    else if (result.status != ScanResult::Status::InvalidPath) result.status = ScanResult::Status::Unavailable;
    // Stable, locale-independent title ordering.
    std::sort(games_.begin(), games_.begin() + count_, [](const Game& a, const Game& b) {
        return std::strcmp(a.title.data(), b.title.data()) < 0;
    });
    return result;
}

} // namespace orbit::library
