#include "orbit/library/preferences.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

namespace orbit::library {
namespace {
bool same(const char* a, const char* b) noexcept { return a && b && std::strcmp(a, b) == 0; }
bool safe_id(const char* id) noexcept {
    if (!id) return false;
    const auto n = std::strlen(id);
    if (n < 9 || n > 12) return false;
    for (const char* p = id; *p; ++p) if (!((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))) return false;
    return true;
}
bool safe_name(const char* name) noexcept {
    if (!name || !*name || std::strlen(name) > 32) return false;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(name); *p; ++p)
        if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == ' ' || *p == '-' || *p == '_')) return false;
    return true;
}
}

bool Preferences::valid_id(const char* id) const noexcept { return safe_id(id); }
void Preferences::reset() noexcept { favorites_ = {}; favorite_count_ = 0; collections_ = {}; collection_count_ = 0; layout = Layout::Carousel; sort = SortOrder::Title; animation_level = 2; accent = 0; }
bool Preferences::favorite(const char* id) const noexcept {
    if (!valid_id(id)) return false;
    for (std::size_t i = 0; i < favorite_count_; ++i) if (same(favorites_[i].data(), id)) return true;
    return false;
}
bool Preferences::toggle_favorite(const char* id) noexcept {
    if (!valid_id(id)) return false;
    for (std::size_t i = 0; i < favorite_count_; ++i) if (same(favorites_[i].data(), id)) {
        favorites_[i] = favorites_[--favorite_count_]; favorites_[favorite_count_] = {}; return true;
    }
    if (favorite_count_ == favorites_.size()) return false;
    std::strncpy(favorites_[favorite_count_++].data(), id, 12); return true;
}
bool Preferences::add_collection(const char* name) noexcept {
    if (!safe_name(name) || collection_count_ == collections_.size()) return false;
    for (std::size_t i = 0; i < collection_count_; ++i) if (same(collections_[i].name.data(), name)) return false;
    std::strncpy(collections_[collection_count_++].name.data(), name, 32); return true;
}
bool Preferences::in_collection(std::size_t i, const char* id) const noexcept {
    if (i >= collection_count_ || !valid_id(id)) return false;
    const auto& c = collections_[i];
    for (std::size_t j = 0; j < c.count; ++j) if (same(c.ids[j].data(), id)) return true;
    return false;
}
bool Preferences::add_to_collection(std::size_t i, const char* id) noexcept {
    if (i >= collection_count_ || !valid_id(id) || in_collection(i, id)) return false;
    auto& c = collections_[i]; if (c.count == c.ids.size()) return false;
    std::strncpy(c.ids[c.count++].data(), id, 12); return true;
}
bool Preferences::remove_from_collection(std::size_t i, const char* id) noexcept {
    if (i >= collection_count_ || !valid_id(id)) return false;
    auto& c = collections_[i];
    for (std::size_t j = 0; j < c.count; ++j) if (same(c.ids[j].data(), id)) { c.ids[j] = c.ids[--c.count]; c.ids[c.count] = {}; return true; }
    return false;
}

bool Preferences::load(const char* path) noexcept {
    reset(); if (!path) return false;
    FILE* f = std::fopen(path, "rb"); if (!f) return false;
    char line[128]{};
    if (!std::fgets(line, sizeof(line), f) || std::strcmp(line, "ORBIT-PREFS-1\n") != 0) { std::fclose(f); reset(); return false; }
    while (std::fgets(line, sizeof(line), f)) {
        const std::size_t n = std::strlen(line); if (n == 0 || line[n - 1] != '\n') { reset(); std::fclose(f); return false; }
        line[n - 1] = '\0';
        unsigned value = 0;
        if (std::sscanf(line, "layout=%u", &value) == 1 && std::strncmp(line, "layout=", 7) == 0) { if (value <= 1) layout = static_cast<Layout>(value); }
        else if (std::sscanf(line, "sort=%u", &value) == 1 && std::strncmp(line, "sort=", 5) == 0) { if (value <= 1) sort = static_cast<SortOrder>(value); }
        else if (std::sscanf(line, "animation=%u", &value) == 1 && std::strncmp(line, "animation=", 10) == 0) { if (value <= 2) animation_level = static_cast<std::uint8_t>(value); }
        else if (std::sscanf(line, "accent=%u", &value) == 1 && std::strncmp(line, "accent=", 7) == 0) { if (value <= 3) accent = static_cast<std::uint8_t>(value); }
        else if (std::strncmp(line, "favorite=", 9) == 0) (void)toggle_favorite(line + 9);
        else if (std::strncmp(line, "collection=", 11) == 0) (void)add_collection(line + 11);
        else if (std::strncmp(line, "member=", 7) == 0) {
            char* split = std::strchr(line + 7, ':'); if (!split) continue; *split = '\0';
            const unsigned idx = static_cast<unsigned>(std::strtoul(line + 7, nullptr, 10));
            if (idx < collection_count_) (void)add_to_collection(idx, split + 1);
        }
    }
    const bool ok = std::ferror(f) == 0; std::fclose(f); if (!ok) reset(); return ok;
}

bool Preferences::save(const char* path) const noexcept {
    if (!path || std::strlen(path) > 220) return false;
    char temporary[256]{}; std::snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    FILE* f = std::fopen(temporary, "wb"); if (!f) return false;
    bool ok = std::fprintf(f, "ORBIT-PREFS-1\nlayout=%u\nsort=%u\nanimation=%u\naccent=%u\n", static_cast<unsigned>(layout), static_cast<unsigned>(sort), animation_level, accent) > 0;
    for (std::size_t i = 0; ok && i < favorite_count_; ++i) ok = std::fprintf(f, "favorite=%s\n", favorites_[i].data()) > 0;
    for (std::size_t i = 0; ok && i < collection_count_; ++i) {
        const auto& c = collections_[i]; ok = std::fprintf(f, "collection=%s\n", c.name.data()) > 0;
        for (std::size_t j = 0; ok && j < c.count; ++j) ok = std::fprintf(f, "member=%zu:%s\n", i, c.ids[j].data()) > 0;
    }
    if (std::fflush(f) != 0) ok = false;
    if (std::fclose(f) != 0) ok = false;
    if (!ok || rename(temporary, path) != 0) { unlink(temporary); return false; }
    return true;
}
} // namespace orbit::library
