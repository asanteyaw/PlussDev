#include <pluss/pluss.h>
#include <pluss/tils/cache.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "core cache test failed: " << message << '\n';
    ++failures;
  }
}

}  // namespace

int main() {
  using pluss::tils::Cache;
  namespace fs = std::filesystem;

  const fs::path dir = fs::temp_directory_path() / "pluss_cache_test";
  std::error_code ignored;
  fs::remove_all(dir, ignored);
  Cache cache(dir);
  require(cache.stats().entries == 0, "a new cache has no entries");

  // A miss computes and saves; the next call is a hit and does not compute.
  int computed = 0;
  const auto make = [&] {
    ++computed;
    return pluss::make_series({1.5, 2.5, 3.5});
  };
  const auto first = cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=3"}, make);
  const auto second = cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=3"}, make);
  require(computed == 1, "the second call is answered from the file");
  require(first.host<double>() == second.host<double>() && second.host<double>() == std::vector<double>({1.5, 2.5, 3.5}),
          "the saved value reads back exactly");
  require(cache.stats().hits == 1 && cache.stats().misses == 1, "stats count one hit and one miss");

  // A different part is a different entry.
  (void)cache.get<pluss::Series>("scores", {"alpha=0.2", "rows=3"}, make);
  require(computed == 2, "a changed part computes again");
  (void)cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=3"}, make);
  require(computed == 2, "the first entry is still there");
  require(cache.stats().entries == 2, "two inputs make two entries");

  // The parts are not run together: ("ab", "c") is not ("a", "bc").
  (void)cache.get<pluss::Series>("split", {"ab", "c"}, make);
  (void)cache.get<pluss::Series>("split", {"a", "bc"}, make);
  require(computed == 4, "part boundaries matter");

  // Other types: text and time.
  int note_computed = 0;
  const auto words = [&] {
    ++note_computed;
    return pluss::make_note({"alpha", "beta"});
  };
  (void)cache.get<pluss::Note>("words", {}, words);
  const auto words_again = cache.get<pluss::Note>("words", {}, words);
  require(note_computed == 1, "a Note is cached");
  require(words_again.numel() == 2, "the cached Note has its cells");

  int stamps_computed = 0;
  const auto stamps = [&] {
    ++stamps_computed;
    return pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-06-01T12:30:00Z"});
  };
  (void)cache.get<pluss::Timeline>("stamps", {}, stamps);
  (void)cache.get<pluss::Timeline>("stamps", {}, stamps);
  require(stamps_computed == 1, "a Timeline is cached");

  // find, has, erase.
  require(cache.has("scores", {"alpha=0.1", "rows=3"}), "has() sees an entry");
  require(!cache.has("scores", {"alpha=0.9"}), "has() does not see one that is not there");
  require(cache.find<pluss::Series>("scores", {"alpha=0.1", "rows=3"}).has_value(), "find() returns the entry");
  require(!cache.find<pluss::Series>("scores", {"alpha=0.9"}).has_value(), "find() returns nothing when there is none");
  require(cache.erase("scores", {"alpha=0.1", "rows=3"}), "erase() removes an entry");
  require(!cache.erase("scores", {"alpha=0.1", "rows=3"}), "erase() says when there was nothing to remove");
  (void)cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=3"}, make);
  require(computed == 5, "an erased entry is computed again");

  // A damaged file is a miss and is replaced.
  for (const auto& item : fs::directory_iterator(dir)) {
    if (item.path().filename().string().rfind("scores-", 0) == 0) {
      std::ofstream(item.path(), std::ios::binary | std::ios::trunc) << "not a saved value";
    }
  }
  const int before = computed;
  const auto repaired = cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=3"}, make);
  require(computed == before + 1 && repaired.numel() == 3, "a damaged entry is recomputed");
  (void)cache.get<pluss::Series>("scores", {"alpha=0.1", "rows=3"}, make);
  require(computed == before + 1, "and the repaired entry is a hit");

  // Two threads computing the same entry at once never leave a broken file.
  Cache shared(dir / "shared");
  std::atomic<int> calls{0};
  const auto slow = [&] {
    ++calls;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return pluss::make_series({7.0, 8.0, 9.0});
  };
  std::vector<std::thread> workers;
  for (int i = 0; i < 4; ++i) workers.emplace_back([&] { (void)shared.get<pluss::Series>("same", {"x"}, slow); });
  for (auto& w : workers) w.join();
  const auto settled = shared.find<pluss::Series>("same", {"x"});
  require(settled.has_value() && settled->host<double>() == std::vector<double>({7.0, 8.0, 9.0}), "the shared entry reads back whole");
  require(shared.stats().entries == 1, "no half-written file is left behind");

  // clear() and the default folder.
  const auto removed = cache.clear();
  require(removed >= 5 && cache.stats().entries == 0, "clear() removes every entry");
  require(!Cache::default_dir("pluss_test_app").empty(), "there is a default folder");

  fs::remove_all(dir, ignored);
  if (failures == 0) std::cout << "core cache tests passed\n";
  return failures == 0 ? 0 : 1;
}
