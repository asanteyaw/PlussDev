#include <catch2/catch_test_macros.hpp>
#include <pluss/coden.h>
#include <pluss/coden/note_factory.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iterator>
#include <limits>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "CodeN/core/OpsInternal.h"
#include "CodeN/core/AlignedAllocator.h"

namespace {

using pluss::coden::NoteImpl;
using pluss::coden::ops_internal::NoteAccess;

static_assert(sizeof(pluss::Note) == sizeof(void *),
              "Note must remain a one-pointer intrusive handle");

template <class T> bool is_lane_aligned(::std::span<const T> values) {
  return values.empty() || reinterpret_cast<::std::uintptr_t>(values.data()) %
                                   NoteImpl::kLaneAlignment ==
                               0;
}

} // namespace

TEST_CASE("Note storage represents five homogeneous dtypes",
          "[coden][typed-storage]") {
  const auto text = pluss::note::make({"alpha", "beta"});
  REQUIRE(text.dtype() == pluss::NoteDType::Text);
  REQUIRE(text.is_text());
  REQUIRE_FALSE(text.is_bytes());
  REQUIRE(pluss::coden::dtype_name(text.dtype()) == "Text");

  const auto bytes = pluss::note::bytes(std::string_view{"a\0b", 3});
  REQUIRE(bytes.dtype() == pluss::NoteDType::Bytes);
  REQUIRE(bytes.is_bytes());
  REQUIRE_FALSE(bytes.is_text());

  const std::array<std::int64_t, 2> matrix_shape{2, 2};
  const std::array<std::uint8_t, 4> bool_values{1, 0, 1, 1};
  const auto bools =
      pluss::coden::ops_internal::note_from_bool(bool_values, matrix_shape);
  REQUIRE(bools.dtype() == pluss::NoteDType::Bool);
  REQUIRE(bools.is_bool());
  REQUIRE(bools.sizes() == std::vector<std::int64_t>{2, 2});
  const auto stored_bools = NoteAccess::impl(bools)->bool_values();
  REQUIRE(std::equal(stored_bools.begin(), stored_bools.end(),
                     bool_values.begin()));
  REQUIRE(is_lane_aligned(stored_bools));

  const std::array<std::int64_t, 1> scalar_value{-17};
  const auto integer =
      pluss::coden::ops_internal::note_from_int64(scalar_value, {});
  REQUIRE(integer.dtype() == pluss::NoteDType::Int64);
  REQUIRE(integer.is_int64());
  REQUIRE(integer.dim() == 0);
  REQUIRE(integer.numel() == 1);
  REQUIRE(NoteAccess::impl(integer)->int64_values().front() == -17);
  REQUIRE(is_lane_aligned(NoteAccess::impl(integer)->int64_values()));

  const std::array<double, 8> float_values{
      0.0,
      -0.0,
      1.25,
      -2.5,
      3.0,
      4.5,
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::quiet_NaN()};
  const std::array<std::int64_t, 3> cube_shape{2, 2, 2};
  const auto floats =
      pluss::coden::ops_internal::note_from_float64(float_values, cube_shape);
  REQUIRE(floats.dtype() == pluss::NoteDType::Float64);
  REQUIRE(floats.is_float64());
  REQUIRE(floats.dim() == 3);
  REQUIRE(floats.numel() == 8);
  REQUIRE(NoteAccess::impl(floats)->float64_values()[3] == -2.5);
  REQUIRE(is_lane_aligned(NoteAccess::impl(floats)->float64_values()));
}

TEST_CASE("typed Note storage preserves empty shapes and shared ownership",
          "[coden][typed-storage]") {
  const std::array<std::int64_t, 2> empty_shape{0, 5};
  const auto empty =
      pluss::coden::ops_internal::note_from_int64({}, empty_shape);
  REQUIRE(empty.defined());
  REQUIRE(empty.dim() == 2);
  REQUIRE(empty.numel() == 0);
  REQUIRE(empty.sizes() == std::vector<std::int64_t>{0, 5});
  REQUIRE(NoteAccess::impl(empty)->int64_values().empty());

  const std::array<double, 2> values{2.0, 4.0};
  const std::array<std::int64_t, 1> shape{2};
  auto original = pluss::coden::ops_internal::note_from_float64(values, shape);
  const auto *storage = NoteAccess::impl(original).get();

  auto copied = original;
  REQUIRE(NoteAccess::impl(copied).get() == storage);
  original = pluss::Note::undefined();
  REQUIRE(copied.defined());
  REQUIRE(NoteAccess::impl(copied)->float64_values()[1] == 4.0);

  auto moved = std::move(copied);
  REQUIRE(NoteAccess::impl(moved).get() == storage);
  REQUIRE(NoteAccess::impl(moved)->float64_values()[0] == 2.0);
}

TEST_CASE("typed Note storage adopts completed kernel buffers without copying",
          "[coden][typed-storage]") {
  const std::array<std::int64_t, 1> shape{4};

  NoteImpl::BoolStorage bools{1, 0, 1, 0};
  const auto *bool_data = bools.data();
  const auto bool_note = pluss::coden::ops_internal::note_from_bool_storage(
      std::move(bools), shape);
  REQUIRE(NoteAccess::impl(bool_note)->bool_values().data() == bool_data);

  NoteImpl::Int64Storage integers{2, 3, 5, 7};
  const auto *int_data = integers.data();
  const auto int_note = pluss::coden::ops_internal::note_from_int64_storage(
      std::move(integers), shape);
  REQUIRE(NoteAccess::impl(int_note)->int64_values().data() == int_data);

  NoteImpl::Float64Storage floats{0.5, 1.5, 2.5, 3.5};
  const auto *float_data = floats.data();
  const auto float_note = pluss::coden::ops_internal::note_from_float64_storage(
      std::move(floats), shape);
  REQUIRE(NoteAccess::impl(float_note)->float64_values().data() == float_data);
}

TEST_CASE("typed Note storage rejects inconsistent states",
          "[coden][typed-storage]") {
  const std::array<std::int64_t, 1> shape_two{2};
  const std::array<std::int64_t, 1> one_integer{1};
  REQUIRE_THROWS(
      pluss::coden::ops_internal::note_from_int64(one_integer, shape_two));

  const std::array<std::uint8_t, 2> invalid_bool{0, 2};
  REQUIRE_THROWS(
      pluss::coden::ops_internal::note_from_bool(invalid_bool, shape_two));

  const std::array<std::int64_t, 1> negative_shape{-1};
  REQUIRE_THROWS(
      pluss::coden::ops_internal::note_from_float64({}, negative_shape));

  const std::array<std::int64_t, 4> rank_four{1, 1, 1, 1};
  REQUIRE_THROWS(pluss::coden::ops_internal::note_from_bool({}, rank_four));

  const std::array<std::int64_t, 2> overflowing_shape{
      std::numeric_limits<std::int64_t>::max(), 2};
  REQUIRE_THROWS(
      pluss::coden::ops_internal::note_from_int64({}, overflowing_shape));

  const std::array<double, 1> scalar{1.0};
  const auto floats = pluss::coden::ops_internal::note_from_float64(scalar, {});
  REQUIRE_THROWS(NoteAccess::impl(floats)->bool_values());
  REQUIRE_THROWS(floats.at(0));

  const auto arena = pluss::flame::make_intrusive<pluss::coden::TextArena>();
  const std::array<std::int64_t, 3> scalar_shape{1, 1, 1};
  REQUIRE_THROWS((pluss::flame::make_intrusive<NoteImpl>(
      arena, std::vector<NoteImpl::OffLen>{{0, 1}}, scalar_shape, 0,
      pluss::NoteDType::Text)));
}

TEST_CASE("undefined Note has no dtype", "[coden][typed-storage]") {
  const auto undefined = pluss::Note::undefined();
  REQUIRE_FALSE(undefined.is_text());
  REQUIRE_FALSE(undefined.is_bytes());
  REQUIRE_FALSE(undefined.is_bool());
  REQUIRE_FALSE(undefined.is_int64());
  REQUIRE_FALSE(undefined.is_float64());
  REQUIRE_THROWS(undefined.dtype());
}

TEST_CASE("public typed Note factories preserve dtype shape and native values",
          "[coden][typed-storage][pass3]") {
  const auto truth = pluss::note::make(true);
  REQUIRE(truth.is_bool());
  REQUIRE(truth.bool_item());

  const auto count = pluss::note::make(-42);
  REQUIRE(count.is_int64());
  REQUIRE(count.int_item() == -42);

  const auto score = pluss::note::make(1.25F);
  REQUIRE(score.is_float64());
  REQUIRE(score.float_item() == 1.25);

  const auto flags = pluss::note::make({true, false, true});
  REQUIRE(flags.bools().size() == 3);
  REQUIRE(flags.bools()[1] == 0);

  const std::array<std::int64_t, 2> shape{2, 2};
  const auto integers = pluss::note::make(std::vector<int>{2, 3, 5, 7}, shape);
  REQUIRE(integers.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(integers.integers()[2] == 5);

  const auto reals = pluss::note::make(
      std::vector<float>{0.5F, 1.5F, 2.5F, 3.5F}, shape);
  REQUIRE(reals.reals()[3] == 3.5);
  REQUIRE_THROWS(reals.int_item());
  REQUIRE_THROWS(pluss::note::make(std::vector<int>{1, 2, 3}, shape));
}

TEST_CASE("typed item access is cardinality based and dtype exact",
          "[coden][typed-storage][item]") {
  const auto bool_rank3 = pluss::note::make(
      std::vector<bool>{true}, std::array<std::int64_t, 3>{1, 1, 1});
  const auto int_rank2 = pluss::note::make(
      std::vector<std::int64_t>{-42}, std::array<std::int64_t, 2>{1, 1});
  const auto float_rank1 = pluss::note::make(std::vector<double>{-0.0});

  REQUIRE(bool_rank3.bool_item());
  REQUIRE(int_rank2.int_item() == -42);
  REQUIRE(std::signbit(float_rank1.float_item()));

  REQUIRE_THROWS(pluss::note::make(std::vector<bool>{}).bool_item());
  REQUIRE_THROWS(pluss::note::make(std::vector<std::int64_t>{1, 2}).int_item());
  REQUIRE_THROWS(pluss::note::make(std::vector<double>{}).float_item());
  REQUIRE_THROWS(pluss::note::make(std::int64_t{1}).bool_item());
  REQUIRE_THROWS(pluss::note::make(1.0).int_item());
  REQUIRE_THROWS(pluss::note::make(std::int64_t{1}).float_item());
  REQUIRE_THROWS(pluss::Note::undefined().bool_item());
  REQUIRE_THROWS(pluss::Note::undefined().int_item());
  REQUIRE_THROWS(pluss::Note::undefined().float_item());
}

TEST_CASE("Note row and slab projections share their backing allocations",
          "[coden][typed-storage][pass3]") {
  const std::array<std::int64_t, 2> matrix_shape{2, 3};
  auto matrix = pluss::note::make(
      std::vector<std::int64_t>{10, 11, 12, 20, 21, 22}, matrix_shape);
  const auto base = matrix.integers().data();
  auto rows = matrix.matrix();
  REQUIRE(rows.size() == 2);
  REQUIRE(rows[0].integers().data() == base);
  REQUIRE(rows[1].integers().data() == base + 3);
  matrix = pluss::Note::undefined();
  REQUIRE(rows[1].integers()[2] == 22);

  const std::array<std::int64_t, 3> cube_shape{2, 2, 2};
  auto cube = pluss::note::make(
      std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7}, cube_shape);
  const auto cube_base = cube.reals().data();
  const auto slabs = cube.slabs();
  REQUIRE(slabs[0].reals().data() == cube_base);
  REQUIRE(slabs[1].reals().data() == cube_base + 4);
  REQUIRE(slabs[1].sizes() == std::vector<std::int64_t>{2, 2});

  const std::array<std::int64_t, 2> text_shape{2, 2};
  const auto text = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"}, text_shape);
  const auto text_rows = text.matrix();
  REQUIRE(NoteAccess::impl(text_rows[0])->arena().get() ==
          NoteAccess::impl(text)->arena().get());
}

TEST_CASE("Note host export and rendering are dtype aware",
          "[coden][typed-storage][pass3]") {
  const auto flag = pluss::note::make(false).host();
  REQUIRE(flag.dtype() == pluss::NoteDType::Bool);
  REQUIRE_FALSE(flag.bool_scalar());

  const auto integers = pluss::note::make({1, 2, 3}).host();
  REQUIRE(integers.int_vector() == std::vector<std::int64_t>{1, 2, 3});

  const std::array<std::int64_t, 2> shape{2, 2};
  const auto array = pluss::note::make(
      std::vector<double>{0.5, 1.5, 2.5, 3.5}, shape).host();
  REQUIRE(array.is_array());
  REQUIRE(array.array().dtype == pluss::NoteDType::Float64);
  REQUIRE(array.array().float_data[2] == 2.5);

  REQUIRE(pluss::coden::to_string(pluss::note::make({true, false})) ==
          "Note(shape=[2], dim=1, numel=2, dtype=Bool)\n[true, false]");
}

TEST_CASE("typed Note storage preserves dtype through shape/index ops",
          "[coden][typed-storage][pass3]") {
  const std::array<std::int64_t, 2> shape{2, 3};
  const auto ints =
      pluss::note::make(std::vector<std::int64_t>{0, 1, 2, 3, 4, 5}, shape);

  const auto reshaped = ints.reshape({3, 2});
  REQUIRE(reshaped.is_int64());
  REQUIRE(reshaped.sizes() == std::vector<std::int64_t>{3, 2});
  REQUIRE(std::vector<std::int64_t>(reshaped.integers().begin(),
                                    reshaped.integers().end()) ==
          std::vector<std::int64_t>{0, 1, 2, 3, 4, 5});

  const auto transposed = ints.transpose(0, 1);
  REQUIRE(transposed.is_int64());
  REQUIRE(transposed.sizes() == std::vector<std::int64_t>{3, 2});
  REQUIRE(std::vector<std::int64_t>(transposed.integers().begin(),
                                    transposed.integers().end()) ==
          std::vector<std::int64_t>{0, 3, 1, 4, 2, 5});

  const auto row = ints.select(0, 1);
  REQUIRE(row.is_int64());
  REQUIRE(row.sizes() == std::vector<std::int64_t>{3});
  REQUIRE(std::vector<std::int64_t>(row.integers().begin(),
                                    row.integers().end()) ==
          std::vector<std::int64_t>{3, 4, 5});

  const auto narrowed = ints.narrow(1, 1, 2);
  REQUIRE(narrowed.is_int64());
  REQUIRE(narrowed.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(std::vector<std::int64_t>(narrowed.integers().begin(),
                                    narrowed.integers().end()) ==
          std::vector<std::int64_t>{1, 2, 4, 5});

  const std::array<std::int64_t, 1> flags_shape{2};
  const auto flags = pluss::note::make(std::vector<bool>{true, false}, flags_shape);
  const std::array<std::int64_t, 1> reps{3};
  const auto tiled = pluss::coden::ops_internal::repeat(flags, reps);
  REQUIRE(tiled.is_bool());
  REQUIRE(tiled.sizes() == std::vector<std::int64_t>{6});
  REQUIRE(std::vector<std::uint8_t>(tiled.bools().begin(), tiled.bools().end()) ==
          std::vector<std::uint8_t>{1, 0, 1, 0, 1, 0});

  const std::array<std::int64_t, 1> real_shape{2};
  const auto reals =
      pluss::note::make(std::vector<double>{1.5, 2.5}, real_shape);
  const auto interleaved = reals.repeat_interleave(2, 0);
  REQUIRE(interleaved.is_float64());
  REQUIRE(interleaved.sizes() == std::vector<std::int64_t>{4});
  REQUIRE(std::vector<double>(interleaved.reals().begin(),
                              interleaved.reals().end()) ==
          std::vector<double>{1.5, 1.5, 2.5, 2.5});
}

TEST_CASE("Note pack and unpack preserve every closed dtype and shape state",
          "[coden][typed-storage][pass3][serialization]") {
  const std::array<std::int64_t, 2> shape{2, 2};
  const std::string binary{"a\0b", 3};
  const std::vector<pluss::Note> values{
      pluss::Note::undefined(),
      pluss::note::make("scalar"),
      pluss::note::bytes(std::vector<std::string_view>{binary, "", "z", "q"}, shape),
      pluss::note::make(std::vector<bool>{true, false, false, true}, shape),
      pluss::note::make(std::vector<std::int64_t>{-1, 0, 7, 9}, shape),
      pluss::note::make(
          std::vector<double>{-0.0, 1.25,
                              std::numeric_limits<double>::infinity(),
                              std::numeric_limits<double>::quiet_NaN()},
          shape),
      pluss::note::make(std::vector<bool>{}, std::array<std::int64_t, 2>{0, 5}),
  };

  for (const auto& value : values) {
    const auto encoded = value.pack();
    const auto restored = pluss::note::unpack(encoded);
    REQUIRE(restored.defined() == value.defined());
    if (!value.defined()) continue;
    REQUIRE(restored.dtype() == value.dtype());
    REQUIRE(restored.sizes() == value.sizes());
    if (value.is_text() || value.is_bytes()) {
      REQUIRE(restored.vector() == value.vector());
    } else if (value.is_bool()) {
      REQUIRE(std::equal(restored.bools().begin(), restored.bools().end(),
                         value.bools().begin(), value.bools().end()));
    } else if (value.is_int64()) {
      REQUIRE(std::equal(restored.integers().begin(), restored.integers().end(),
                         value.integers().begin(), value.integers().end()));
    } else {
      REQUIRE(restored.reals().size() == value.reals().size());
      for (std::size_t i = 0; i < value.reals().size(); ++i) {
        REQUIRE(std::bit_cast<std::uint64_t>(restored.reals()[i]) ==
                std::bit_cast<std::uint64_t>(value.reals()[i]));
      }
    }
  }
}

TEST_CASE("Note unpack rejects corrupted or non-canonical envelopes",
          "[coden][typed-storage][pass3][serialization]") {
  auto encoded = pluss::note::make({true, false}).pack();

  auto bad_magic = encoded;
  bad_magic[0] = std::byte{0};
  REQUIRE_THROWS(pluss::note::unpack(bad_magic));

  auto bad_version = encoded;
  bad_version[4] = std::byte{99};
  REQUIRE_THROWS(pluss::note::unpack(bad_version));

  auto bad_dtype = encoded;
  bad_dtype[5] = std::byte{99};
  REQUIRE_THROWS(pluss::note::unpack(bad_dtype));

  auto bad_bool = encoded;
  bad_bool.back() = std::byte{2};
  REQUIRE_THROWS(pluss::note::unpack(bad_bool));

  auto trailing = encoded;
  trailing.push_back(std::byte{0});
  REQUIRE_THROWS(pluss::note::unpack(trailing));

  REQUIRE_THROWS(pluss::note::unpack(
      std::span<const std::byte>(encoded.data(), encoded.size() - 1)));
}

TEST_CASE("Note's generic byte-oriented traversal is Text/Bytes only, by design",
          "[coden][typed-storage][pass8]") {
  // Text/Bytes: begin()/end(), copy_flat_to, and for_each_flat all work as
  // before -- unaffected by the new dtype checks.
  const auto text = pluss::note::make({"alpha", "beta", "gamma"});
  std::vector<std::string> collected;
  for (const auto& cell : text) collected.emplace_back(cell);
  REQUIRE(collected == std::vector<std::string>{"alpha", "beta", "gamma"});

  std::vector<std::string_view> copied;
  text.copy_flat_to(std::back_inserter(copied));
  REQUIRE(copied.size() == 3);

  std::size_t visited = 0;
  text.for_each_flat([&](std::string_view) { ++visited; });
  REQUIRE(visited == 3);

  // Bool/Int64/Float64: fail immediately and clearly, rather than iterating
  // partway or misinterpreting lanes as text. The typed span accessors
  // (bools()/integers()/reals()) are the intended traversal path instead --
  // themselves plain std::span<const T>, already range-for- and
  // <algorithm>-compatible, as used throughout this test file.
  const auto ints = pluss::note::make(std::vector<int>{1, 2, 3});
  REQUIRE_THROWS(ints.begin());
  REQUIRE_THROWS(ints.end());
  REQUIRE_THROWS(ints.for_each_flat([](std::string_view) {}));
  REQUIRE_THROWS(ints.copy_flat_to(std::back_inserter(copied)));

  // Iterators retain the arena, so both the iterator and a view obtained from
  // it remain valid after the original Note handle leaves scope.
  auto kept = [] {
    const auto temporary = pluss::note::make({"kept", "alive"});
    return temporary.begin();
  }();
  REQUIRE(*kept == "kept");
  ++kept;
  REQUIRE(*kept == "alive");

  // An undefined Note yields an empty range rather than throwing (matches
  // the pre-existing convention for other accessors on undefined Notes).
  const auto undefined = pluss::Note::undefined();
  REQUIRE(undefined.begin() == undefined.end());
}

TEST_CASE("Note::debug() and operator<< already branch per dtype",
          "[coden][typed-storage][pass8]") {
  const std::array<std::int64_t, 1> shape{3};
  const auto ints = pluss::note::make(std::vector<int>{1, -2, 3}, shape);
  REQUIRE(ints.debug() == "Note(shape=[3], dim=1, numel=3, dtype=Int64, values=[1, -2, 3])");

  const auto reals = pluss::note::make(std::vector<double>{0.5, 1.5}, std::array<std::int64_t, 1>{2});
  REQUIRE(reals.debug() == "Note(shape=[2], dim=1, numel=2, dtype=Float64, values=[0.5, 1.5])");

  const auto bools = pluss::note::make(std::vector<bool>{true, false}, std::array<std::int64_t, 1>{2});
  REQUIRE(bools.debug() == "Note(shape=[2], dim=1, numel=2, dtype=Bool, values=[true, false])");
  REQUIRE(ints.debug(0) == "Note(shape=[3], dim=1, numel=3, dtype=Int64, values=[...])");
  REQUIRE(ints.debug(-1) == "Note(shape=[3], dim=1, numel=3, dtype=Int64, values=[...])");

  const std::string raw{"A\0\xFF", 3};
  const auto bytes = pluss::note::bytes(
      std::vector<std::string>{raw}, std::array<std::int64_t, 1>{1});
  REQUIRE(bytes.debug() ==
          "Note(shape=[1], dim=1, numel=1, encoding=bytes, values=[\"A\\x00\\xFF\"])");

  std::ostringstream oss;
  oss << ints;
  REQUIRE(oss.str() == pluss::coden::to_string(ints));
  REQUIRE(oss.str().find("dtype=Int64") != std::string::npos);
}

TEST_CASE("TokenBatch adopts validated aligned offsets without copying",
          "[coden][typed-storage][pass6]") {
  NoteImpl::Int64Storage lanes{0, 2, 3};
  const auto* lane_data = lanes.data();
  auto offsets = pluss::coden::ops_internal::note_from_int64_storage(
      std::move(lanes), std::array<std::int64_t, 1>{3});
  auto batch = pluss::coden::ops_internal::TokenBatchFactoryAccess::make(
      pluss::note::make({"a", "b", "c"}), std::move(offsets));
  REQUIRE(batch.offsets().integers().data() == lane_data);

  REQUIRE_THROWS(pluss::coden::ops_internal::TokenBatchFactoryAccess::make(
      pluss::note::make({"a"}), pluss::note::make({1, 1})));
  REQUIRE_THROWS(pluss::coden::ops_internal::TokenBatchFactoryAccess::make(
      pluss::note::make({"a", "b"}), pluss::note::make({0, 2, 1})));
  REQUIRE_THROWS(pluss::coden::ops_internal::TokenBatchFactoryAccess::make(
      pluss::note::make({"a", "b"}), pluss::note::make({0, 1})));
  REQUIRE_THROWS(pluss::coden::ops_internal::TokenBatchFactoryAccess::make(
      pluss::note::make({"a"}), pluss::note::make({false, true})));
}

TEST_CASE("closed Bool and Int64 kernels allocate one native output buffer",
          "[coden][typed-storage][pass8][allocation]") {
  const auto text = pluss::note::make({"alpha", "beta", "gamma", "delta"});

  auto mask = pluss::Note::undefined();
  pluss::coden::detail::LaneAllocationSnapshot stats;
  {
    pluss::coden::detail::LaneAllocationScope tracking;
    mask = text.contains("a");
    stats = tracking.snapshot();
  }
  REQUIRE(stats.allocations == 1);
  REQUIRE(stats.bytes ==
          static_cast<std::uint64_t>(text.numel() * sizeof(std::uint8_t)));

  auto lengths = pluss::Note::undefined();
  {
    pluss::coden::detail::LaneAllocationScope tracking;
    lengths = text.lengths_bytes();
    stats = tracking.snapshot();
  }
  REQUIRE(stats.allocations == 1);
  REQUIRE(stats.bytes ==
          static_cast<std::uint64_t>(text.numel() * sizeof(std::int64_t)));

  const auto other = text.starts_with("a");
  auto combined = pluss::Note::undefined();
  {
    pluss::coden::detail::LaneAllocationScope tracking;
    combined = mask | other;
    stats = tracking.snapshot();
  }
  REQUIRE(stats.allocations == 1);
  REQUIRE(stats.bytes ==
          static_cast<std::uint64_t>(text.numel() * sizeof(std::uint8_t)));

  REQUIRE(lengths.is_int64());
  REQUIRE(combined.is_bool());
}
