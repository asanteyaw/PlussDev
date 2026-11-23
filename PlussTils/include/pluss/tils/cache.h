#pragma once

#include <pluss/exception.h>
#include <pluss/io.h>
#include <pluss/version.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace pluss::tils {

// A folder of saved results, so an expensive step runs once for a given set of inputs.
//
//   Cache cache(Cache::default_dir("myapp"));
//   auto scores = cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=1000"}, [&] { return fit(0.1); });
//
// An entry is found by its key and its parts together. The parts are the inputs the result depends
// on (settings, a data stamp, a file's modification time), written as text: change any of them and
// the entry is a different one. The library version is part of every entry, so a new release never
// reads what an older one saved. Values are saved with pluss::io, so any type it supports works:
// Series, Note, Timeline, TimeIndex and the standard containers of them.
//
// A file that cannot be read back as the requested type is replaced, not an error: a cache must
// not be able to make a program fail. Entries are written to a temporary file and renamed, so a
// reader never sees half an entry and two processes computing the same entry at once are safe
// (the last to finish wins; both computed the same inputs).
class Cache {
 public:
  struct Stats {
    std::size_t entries{0};      // files in the folder
    std::uintmax_t bytes{0};     // their size
    std::size_t hits{0};         // get() calls answered from a file, since this object was made
    std::size_t misses{0};       // get() calls that computed
  };

  explicit Cache(std::filesystem::path dir) : dir_(std::move(dir)) {}

  // The per-user cache folder for an application: the platform's own place for it.
  [[nodiscard]] static std::filesystem::path default_dir(std::string_view app = "pluss") {
#if defined(_WIN32)
    if (const char* base = std::getenv("LOCALAPPDATA")) return std::filesystem::path(base) / std::string(app) / "cache";
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) return std::filesystem::path(home) / "Library" / "Caches" / std::string(app);
#else
    if (const char* base = std::getenv("XDG_CACHE_HOME"); base != nullptr && *base != '\0') {
      return std::filesystem::path(base) / std::string(app);
    }
    if (const char* home = std::getenv("HOME")) return std::filesystem::path(home) / ".cache" / std::string(app);
#endif
    return std::filesystem::temp_directory_path() / std::string(app);
  }

  [[nodiscard]] const std::filesystem::path& dir() const noexcept { return dir_; }

  // The saved value, or the result of make() saved for next time.
  template <class T, class Make>
  T get(std::string_view key, std::initializer_list<std::string> parts, Make&& make) {
    return get<T>(key, std::vector<std::string>(parts), std::forward<Make>(make));
  }

  template <class T, class Make>
  T get(std::string_view key, const std::vector<std::string>& parts, Make&& make) {
    if (auto found = find<T>(key, parts)) {
      ++hits_;
      return std::move(*found);
    }
    ++misses_;
    T value = make();
    put(key, parts, value);
    return value;
  }

  // The saved value if there is one that reads back as T.
  template <class T>
  [[nodiscard]] std::optional<T> find(std::string_view key, const std::vector<std::string>& parts) const {
    const auto path = path_(key, parts);
    std::error_code ignored;
    if (!std::filesystem::is_regular_file(path, ignored)) return std::nullopt;
    try {
      return pluss::io::load<T>(path.string());
    } catch (const std::exception&) {
      return std::nullopt;
    }
  }

  template <class T>
  void put(std::string_view key, const std::vector<std::string>& parts, const T& value) {
    const auto path = path_(key, parts);
    std::filesystem::create_directories(dir_);
    // A name no other writer uses, so two processes or threads never share a half-written file.
    const auto temp = path.string() + ".tmp." + std::to_string(next_temp_());
    try {
      pluss::io::save(value, temp);
      std::error_code failure;
      std::filesystem::rename(temp, path, failure);
      if (failure) {  // Windows will not rename over an existing file
        std::filesystem::remove(path, failure);
        std::filesystem::rename(temp, path);
      }
    } catch (...) {
      std::error_code ignored;
      std::filesystem::remove(temp, ignored);
      throw;
    }
  }

  [[nodiscard]] bool has(std::string_view key, const std::vector<std::string>& parts) const {
    std::error_code ignored;
    return std::filesystem::is_regular_file(path_(key, parts), ignored);
  }

  // Remove one entry. True when there was one.
  bool erase(std::string_view key, const std::vector<std::string>& parts) {
    std::error_code ignored;
    return std::filesystem::remove(path_(key, parts), ignored);
  }

  // Remove every entry; returns how many.
  std::size_t clear() {
    std::size_t removed = 0;
    std::error_code ignored;
    if (!std::filesystem::is_directory(dir_, ignored)) return 0;
    for (const auto& item : std::filesystem::directory_iterator(dir_, ignored)) {
      if (is_entry_(item.path()) && std::filesystem::remove(item.path(), ignored)) ++removed;
    }
    return removed;
  }

  [[nodiscard]] Stats stats() const {
    Stats out;
    out.hits = hits_;
    out.misses = misses_;
    std::error_code ignored;
    if (!std::filesystem::is_directory(dir_, ignored)) return out;
    for (const auto& item : std::filesystem::directory_iterator(dir_, ignored)) {
      if (!is_entry_(item.path())) continue;
      ++out.entries;
      out.bytes += std::filesystem::file_size(item.path(), ignored);
    }
    return out;
  }

 private:
  static constexpr std::string_view extension_ = ".plcache";

  [[nodiscard]] static bool is_entry_(const std::filesystem::path& path) {
    return path.extension() == extension_;
  }

  // FNV-1a over the version, the key and each part, with a length before each so that
  // ("ab", "c") and ("a", "bc") are different entries.
  [[nodiscard]] static std::uint64_t hash_(std::string_view key, const std::vector<std::string>& parts) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    const auto feed = [&h](std::string_view text) {
      const std::uint64_t size = text.size();
      for (int i = 0; i < 8; ++i) {
        h ^= (size >> (8 * i)) & 0xFF;
        h *= 0x100000001b3ULL;
      }
      for (const char ch : text) {
        h ^= static_cast<unsigned char>(ch);
        h *= 0x100000001b3ULL;
      }
    };
    feed(pluss::version().text);
    feed(key);
    for (const auto& part : parts) feed(part);
    return h;
  }

  [[nodiscard]] std::filesystem::path path_(std::string_view key, const std::vector<std::string>& parts) const {
    PLUSS_CHECK(!key.empty(), "Cache: a key cannot be empty");
    std::string name;
    for (const char ch : key) {
      const bool plain = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '-';
      name += plain ? ch : '_';
    }
    constexpr std::string_view digits = "0123456789abcdef";
    const std::uint64_t h = hash_(key, parts);
    name += '-';
    for (int shift = 60; shift >= 0; shift -= 4) name += digits[(h >> shift) & 0xF];
    name += extension_;
    return dir_ / name;
  }

  [[nodiscard]] static std::uint64_t next_temp_() {
    static std::atomic<std::uint64_t> counter{0};
    return (static_cast<std::uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())) << 20) ^ ++counter;
  }

  std::filesystem::path dir_;
  mutable std::atomic<std::size_t> hits_{0};
  mutable std::atomic<std::size_t> misses_{0};
};

}  // namespace pluss::tils
