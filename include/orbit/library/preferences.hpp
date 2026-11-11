#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace orbit::library {

enum class Layout : std::uint8_t { Carousel = 0, Grid = 1 };
enum class SortOrder : std::uint8_t { Title = 0, TitleId = 1 };
struct Collection { std::array<char, 33> name{}; std::array<std::array<char, 13>, 64> ids{}; std::size_t count{0}; };

class Preferences final {
public:
    static constexpr std::size_t kMaxFavorites = 512;
    static constexpr std::size_t kMaxCollections = 8;
    static constexpr std::size_t kMaxCollectionGames = 64;
    bool favorite(const char* title_id) const noexcept;
    bool toggle_favorite(const char* title_id) noexcept;
    bool add_collection(const char* name) noexcept;
    bool add_to_collection(std::size_t collection, const char* title_id) noexcept;
    bool remove_from_collection(std::size_t collection, const char* title_id) noexcept;
    std::size_t collection_count() const noexcept { return collection_count_; }
    const Collection* collection(std::size_t i) const noexcept { return i < collection_count_ ? &collections_[i] : nullptr; }
    bool in_collection(std::size_t i, const char* title_id) const noexcept;
    bool load(const char* path) noexcept;
    bool save(const char* path) const noexcept;
    void reset() noexcept;

    Layout layout{Layout::Carousel};
    SortOrder sort{SortOrder::Title};
    std::uint8_t animation_level{2}; // 0 reduced, 1 subtle, 2 full
    std::uint8_t accent{0};           // Theme palette index
private:
    bool valid_id(const char* title_id) const noexcept;
    std::array<std::array<char, 13>, kMaxFavorites> favorites_{};
    std::size_t favorite_count_{0};
    std::array<Collection, kMaxCollections> collections_{};
    std::size_t collection_count_{0};
};

} // namespace orbit::library
