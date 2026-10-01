#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path temp_file_path(std::string_view stem) {
  const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
  return std::filesystem::temp_directory_path()
      / (std::string(stem) + "_" + std::to_string(nonce) + ".plio");
}

void remove_if_exists(const std::filesystem::path& path) {
  std::error_code ec;
  (void)std::filesystem::remove(path, ec);
}

} // namespace

TEST_CASE("pluss::save/load roundtrip for Series", "[io][series]") {
  const auto path = temp_file_path("pluss_series_roundtrip");

  auto s = pluss::series::arange(0, 6, pluss::ScalarType::Int64).to(pluss::ScalarType::Float32).reshape({2, 3});
  pluss::save(s, path.string());

  const auto loaded = pluss::load<pluss::Series>(path.string());

  REQUIRE(loaded.defined());
  REQUIRE(loaded.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(loaded.to_vec<float>() == s.to_vec<float>());

  remove_if_exists(path);
}

TEST_CASE("pluss::save/load roundtrip for bytes Note", "[io][note]") {
  const auto path = temp_file_path("pluss_note_roundtrip");

  const std::string a("a\0b", 3);
  const std::string b("\xFF\x01", 2);
  const std::vector<std::string_view> flat{
      std::string_view(a.data(), a.size()),
      std::string_view(b.data(), b.size())};
  const std::array<std::int64_t, 1> sizes{2};

  const auto note = pluss::note::bytes(
      std::span<const std::string_view>(flat.data(), flat.size()),
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  pluss::save(note, path.string());
  const auto loaded = pluss::load<pluss::Note>(path.string());

  REQUIRE(loaded.defined());
  REQUIRE(static_cast<const pluss::coden::NoteBase&>(loaded).is_bytes());
  REQUIRE(loaded.numel() == 2);

  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(loaded.numel()));
  loaded.for_each_flat([&](std::string_view sv) {
    out.emplace_back(sv.data(), sv.size());
  });

  REQUIRE(out.size() == 2);
  REQUIRE(out[0].size() == 3);
  REQUIRE(out[0][0] == 'a');
  REQUIRE(out[0][1] == '\0');
  REQUIRE(out[0][2] == 'b');
  REQUIRE(out[1].size() == 2);
  REQUIRE(static_cast<unsigned char>(out[1][0]) == 0xFFu);
  REQUIRE(static_cast<unsigned char>(out[1][1]) == 0x01u);

  remove_if_exists(path);
}

TEST_CASE("pluss::save/load roundtrip for Timeline and TimeIndex", "[io][timeline][timeindex]") {
  const auto timeline_path = temp_file_path("pluss_timeline_roundtrip");
  const auto timeindex_path = temp_file_path("pluss_timeindex_roundtrip");

  const auto timeline = pluss::timeline::from_epoch(
      std::vector<std::int64_t>{1704067200000000000LL, 1704153600000000000LL},
      std::vector<std::int64_t>{2});

  pluss::save(timeline, timeline_path.string());
  const auto loaded_timeline = pluss::load<pluss::Timeline>(timeline_path.string());

  REQUIRE(loaded_timeline.defined());
  REQUIRE(loaded_timeline.ticks() == timeline.ticks());
  REQUIRE(loaded_timeline.tz() == timeline.tz());

  const auto index =
      pluss::timeline::from_epoch(std::vector<std::int64_t>{3, 1, 2}).index(false);
  pluss::save(index, timeindex_path.string());
  const auto loaded_index =
      pluss::load<pluss::Timeline::TimelineIndexAccessor>(timeindex_path.string());

  REQUIRE(loaded_index.size() == index.size());
  REQUIRE(loaded_index.timeline().ticks() == index.timeline().ticks());

  remove_if_exists(timeline_path);
  remove_if_exists(timeindex_path);
}

TEST_CASE("pluss::save/load roundtrip for STL containers and StackArray<Series>", "[io][container]") {
  const auto vec_path = temp_file_path("pluss_vector_roundtrip");
  const auto stack_path = temp_file_path("pluss_stack_roundtrip");

  const std::vector<pluss::Series> batch{
      pluss::series::arange(0, 4, pluss::ScalarType::Int64),
      pluss::series::ones({2, 2}, pluss::ScalarType::Float32)};

  pluss::save(batch, vec_path.string());
  const auto loaded_batch = pluss::load<std::vector<pluss::Series>>(vec_path.string());

  REQUIRE(loaded_batch.size() == batch.size());
  REQUIRE(loaded_batch[0].to_vec<std::int64_t>() == batch[0].to_vec<std::int64_t>());
  REQUIRE(loaded_batch[1].to_vec<float>() == batch[1].to_vec<float>());

  const pluss::StackArray<pluss::Series> stack{
      pluss::series::zeros({3}, pluss::ScalarType::Float32),
      pluss::series::ones({3}, pluss::ScalarType::Float32)};

  pluss::save(stack, stack_path.string());
  const auto loaded_stack = pluss::load<pluss::StackArray<pluss::Series>>(stack_path.string());

  REQUIRE(loaded_stack.size() == stack.size());
  REQUIRE(loaded_stack[0].to_vec<float>() == stack[0].to_vec<float>());
  REQUIRE(loaded_stack[1].to_vec<float>() == stack[1].to_vec<float>());

  remove_if_exists(vec_path);
  remove_if_exists(stack_path);
}
