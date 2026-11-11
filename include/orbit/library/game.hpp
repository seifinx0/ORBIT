#pragma once

#include <array>
#include <cstddef>

namespace orbit::library {

struct Game {
    std::array<char, 129> title{};
    std::array<char, 13> title_id{};
    std::array<char, 49> content_id{};
    std::array<char, 9> version{};
    std::array<char, 256> install_path{};
    std::array<char, 256> icon_path{};
    std::array<char, 256> background_path{};
    std::array<char, 4> category{};
    bool has_icon{false};
    bool has_background{false};
};

struct ScanResult {
    enum class Status { Ok, Unavailable, InvalidPath } status{Status::Ok};
    std::size_t discovered{0};
    std::size_t rejected{0};
    bool truncated{false};
};

class Catalog final {
public:
    static constexpr std::size_t kCapacity = 512;
    ScanResult scan(const char* installed_apps_root = "/user/app") noexcept;
    ScanResult scan_roots(const char* const* installed_apps_roots, std::size_t root_count) noexcept;
    std::size_t size() const noexcept { return count_; }
    const Game* at(std::size_t index) const noexcept { return index < count_ ? &games_[index] : nullptr; }
    const Game* find(const char* title_id) const noexcept;
    void clear() noexcept;
private:
    std::array<Game, kCapacity> games_{};
    std::size_t count_{0};
};

// Bounded parser for Sony param.sfo. The file must be supplied as bytes so
// discovery and parser behavior can be exercised by host tests.
class ParamSfo final {
public:
    bool parse(const unsigned char* bytes, std::size_t size) noexcept;
    bool text(const char* key, char* output, std::size_t capacity) const noexcept;
private:
    struct Entry { std::array<char, 33> key{}; const unsigned char* value{}; std::size_t length{}; unsigned short format{}; };
    std::array<Entry, 128> entries_{};
    std::size_t count_{0};
};

} // namespace orbit::library
