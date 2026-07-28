#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/codem/stack_array.h"
#include "pluss/coden/note_factory.h"
#include "pluss/codeh/timeline_factory.h"
#include "pluss/codeh/timeindex_factory.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <list>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::io {

enum class Compression : std::uint8_t {
  None = 0,
  Zstd = 1,
  Lz4 = 2,
};

struct SaveOptions {
  Compression compression = Compression::None;
  std::int32_t compression_level = 0;
  bool create_parent_dirs = false;
  bool include_platform_hint = true;
  bool canonicalize_series_storage = true;
  bool warn_on_platform_downgrade = true;
};

struct LoadOptions {
  bool preserve_saved_platform = false;
  std::optional<pluss::codem::Platform> target_platform{};
  bool strict_platform = false;
};

namespace detail {

enum class Tag : std::uint8_t {
  Series = 1,
  Note = 2,
  Timeline = 3,
  TimeIndex = 4,
  Vector = 20,
  List = 21,
  Deque = 22,
  Array = 23,
  StackArraySeries = 24,
};

inline constexpr std::array<char, 8> k_magic{{'P', 'L', 'I', 'O', 'B', 'I', 'N', '\0'}};
inline constexpr std::uint16_t k_schema_major = 1;
inline constexpr std::uint16_t k_schema_minor = 0;

class BinaryWriter final {
public:
  explicit BinaryWriter(std::ostream& out) : out_(out) {}

  void write_bytes(const void* data, std::size_t size, std::string_view where) {
    if (size == 0) return;
    PLUSS_CHECK(data != nullptr, std::string("pluss::io::save: null write buffer at ") + std::string(where));
    out_.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    PLUSS_CHECK(out_.good(), std::string("pluss::io::save: stream write failed at ") + std::string(where));
  }

  void write_u8(std::uint8_t value, std::string_view where) { write_bytes(&value, sizeof(value), where); }
  void write_u16(std::uint16_t value, std::string_view where) { write_bytes(&value, sizeof(value), where); }
  void write_u32(std::uint32_t value, std::string_view where) { write_bytes(&value, sizeof(value), where); }
  void write_u64(std::uint64_t value, std::string_view where) { write_bytes(&value, sizeof(value), where); }
  void write_i64(std::int64_t value, std::string_view where) { write_bytes(&value, sizeof(value), where); }

  void write_string(std::string_view s, std::string_view where) {
    write_u64(static_cast<std::uint64_t>(s.size()), where);
    write_bytes(s.data(), s.size(), where);
  }

private:
  std::ostream& out_;
};

class BinaryReader final {
public:
  explicit BinaryReader(std::istream& in) : in_(in) {}

  void read_bytes(void* dst, std::size_t size, std::string_view where) {
    if (size == 0) return;
    PLUSS_CHECK(dst != nullptr, std::string("pluss::io::load: null read buffer at ") + std::string(where));
    in_.read(static_cast<char*>(dst), static_cast<std::streamsize>(size));
    PLUSS_CHECK(
        in_.good(),
        std::string("pluss::io::load: stream read failed at ") + std::string(where) +
            ". requested=" + std::to_string(size) + " bytes");
  }

  std::uint8_t read_u8(std::string_view where) {
    std::uint8_t out = 0;
    read_bytes(&out, sizeof(out), where);
    return out;
  }

  std::uint16_t read_u16(std::string_view where) {
    std::uint16_t out = 0;
    read_bytes(&out, sizeof(out), where);
    return out;
  }

  std::uint32_t read_u32(std::string_view where) {
    std::uint32_t out = 0;
    read_bytes(&out, sizeof(out), where);
    return out;
  }

  std::uint64_t read_u64(std::string_view where) {
    std::uint64_t out = 0;
    read_bytes(&out, sizeof(out), where);
    return out;
  }

  std::int64_t read_i64(std::string_view where) {
    std::int64_t out = 0;
    read_bytes(&out, sizeof(out), where);
    return out;
  }

  std::string read_string(std::string_view where) {
    const auto n = read_u64(where);
    PLUSS_CHECK(
        n <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),
        std::string("pluss::io::load: oversized string at ") + std::string(where));
    std::string out(static_cast<std::size_t>(n), '\0');
    if (!out.empty()) {
      read_bytes(out.data(), out.size(), where);
    }
    return out;
  }

private:
  std::istream& in_;
};

template <class T>
struct dependent_false : std::false_type {};

template <class T>
struct is_std_vector : std::false_type {};

template <class T, class Alloc>
struct is_std_vector<std::vector<T, Alloc>> : std::true_type {};

template <class T>
struct is_std_list : std::false_type {};

template <class T, class Alloc>
struct is_std_list<std::list<T, Alloc>> : std::true_type {};

template <class T>
struct is_std_deque : std::false_type {};

template <class T, class Alloc>
struct is_std_deque<std::deque<T, Alloc>> : std::true_type {};

template <class T>
struct is_std_array : std::false_type {};

template <class T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <class T>
struct array_traits;

template <class T, std::size_t N>
struct array_traits<std::array<T, N>> {
  using value_type = T;
  static constexpr std::size_t k_size = N;
};

template <class T>
struct is_stack_array_series : std::false_type {};

template <>
struct is_stack_array_series<pluss::StackArray<pluss::Series>> : std::true_type {};

template <class T>
struct serializable_value : std::false_type {};

template <>
struct serializable_value<pluss::Series> : std::true_type {};

template <>
struct serializable_value<pluss::Note> : std::true_type {};

template <>
struct serializable_value<pluss::Timeline> : std::true_type {};

template <>
struct serializable_value<pluss::TimeIndex> : std::true_type {};

template <class T, class Alloc>
struct serializable_value<std::vector<T, Alloc>>
    : std::bool_constant<serializable_value<std::remove_cv_t<T>>::value> {};

template <class T, class Alloc>
struct serializable_value<std::list<T, Alloc>>
    : std::bool_constant<serializable_value<std::remove_cv_t<T>>::value> {};

template <class T, class Alloc>
struct serializable_value<std::deque<T, Alloc>>
    : std::bool_constant<serializable_value<std::remove_cv_t<T>>::value> {};

template <class T, std::size_t N>
struct serializable_value<std::array<T, N>>
    : std::bool_constant<serializable_value<std::remove_cv_t<T>>::value> {};

template <>
struct serializable_value<pluss::StackArray<pluss::Series>> : std::true_type {};

template <class T>
inline constexpr bool serializable_value_v = serializable_value<std::remove_cv_t<T>>::value;

template <class T>
using no_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

inline std::vector<std::int64_t> contiguous_strides_(std::span<const std::int64_t> sizes) {
  std::vector<std::int64_t> strides(sizes.size(), 1);
  if (sizes.empty()) return strides;
  std::int64_t step = 1;
  for (std::size_t i = sizes.size(); i > 0; --i) {
    const std::size_t index = i - 1;
    strides[index] = step;
    step *= std::max<std::int64_t>(sizes[index], 1);
  }
  return strides;
}

inline std::vector<std::int64_t> to_sizes_(pluss::msharp::RangeView::Int view) {
  return std::vector<std::int64_t>(view.begin(), view.end());
}

inline std::uint64_t checked_numel_(std::span<const std::int64_t> sizes, std::string_view where) {
  if (sizes.empty()) return 1;
  std::uint64_t n = 1;
  for (const auto s : sizes) {
    PLUSS_CHECK(s >= 0, std::string(where) + ": shape contains negative extent");
    const auto us = static_cast<std::uint64_t>(s);
    if (us == 0) return 0;
    PLUSS_CHECK(
        n <= std::numeric_limits<std::uint64_t>::max() / us,
        std::string(where) + ": shape product overflow");
    n *= us;
  }
  return n;
}

inline void maybe_warn_unimplemented_compression_(const SaveOptions& options) {
  if (options.compression == Compression::None) return;
  PLUSS_WARN(
      "pluss::io::save: compression policy requested ("
      + std::to_string(static_cast<std::uint8_t>(options.compression))
      + ") but this build currently writes uncompressed payloads. Proceeding with Compression::None.");
}

inline void write_header_(BinaryWriter& writer, const SaveOptions& options) {
  PLUSS_CHECK(
      std::endian::native == std::endian::little,
      "pluss::io::save currently supports little-endian hosts only");

  maybe_warn_unimplemented_compression_(options);

  writer.write_bytes(k_magic.data(), k_magic.size(), "header.magic");
  writer.write_u16(k_schema_major, "header.schema_major");
  writer.write_u16(k_schema_minor, "header.schema_minor");
  writer.write_u8(static_cast<std::uint8_t>(Compression::None), "header.compression");
  writer.write_u8(0, "header.flags");
}

inline void read_header_(BinaryReader& reader) {
  PLUSS_CHECK(
      std::endian::native == std::endian::little,
      "pluss::io::load currently supports little-endian hosts only");

  std::array<char, k_magic.size()> magic{};
  reader.read_bytes(magic.data(), magic.size(), "header.magic");
  PLUSS_CHECK(magic == k_magic, "pluss::io::load: invalid file magic (expected PLIOBIN)");

  const auto major = reader.read_u16("header.schema_major");
  const auto minor = reader.read_u16("header.schema_minor");
  PLUSS_CHECK(
      major == k_schema_major,
      "pluss::io::load: schema major mismatch. file=" + std::to_string(major)
          + ", runtime=" + std::to_string(k_schema_major));
  PLUSS_CHECK(
      minor <= k_schema_minor,
      "pluss::io::load: schema minor is newer than runtime. file=" + std::to_string(minor)
          + ", runtime=" + std::to_string(k_schema_minor));

  const auto compression = static_cast<Compression>(reader.read_u8("header.compression"));
  (void)reader.read_u8("header.flags");

  PLUSS_CHECK(
      compression == Compression::None,
      "pluss::io::load: file uses unsupported compression policy. expected Compression::None");
}

inline void write_tag_(BinaryWriter& writer, Tag tag) {
  writer.write_u8(static_cast<std::uint8_t>(tag), "tag");
}

inline Tag read_tag_(BinaryReader& reader) {
  return static_cast<Tag>(reader.read_u8("tag"));
}

inline void expect_tag_(Tag got, Tag expected, std::string_view where) {
  PLUSS_CHECK(
      got == expected,
      std::string("pluss::io::load: tag mismatch at ") + std::string(where)
          + ". expected=" + std::to_string(static_cast<std::uint8_t>(expected))
          + ", got=" + std::to_string(static_cast<std::uint8_t>(got)));
}

inline bool is_non_decreasing_(std::span<const std::int64_t> values) {
  if (values.size() < 2) return true;
  for (std::size_t i = 1; i < values.size(); ++i) {
    if (values[i] < values[i - 1]) return false;
  }
  return true;
}

inline pluss::codem::Platform decode_platform_(std::uint8_t raw, std::string_view where) {
  PLUSS_CHECK(raw <= static_cast<std::uint8_t>(pluss::codem::Platform::MPS),
              std::string("pluss::io::load: invalid platform enum at ") + std::string(where)
                  + ": raw=" + std::to_string(raw));
  return static_cast<pluss::codem::Platform>(raw);
}

inline pluss::codem::ScalarType decode_scalar_type_(std::uint8_t raw, std::string_view where) {
  PLUSS_CHECK(raw <= static_cast<std::uint8_t>(pluss::codem::ScalarType::Complex128),
              std::string("pluss::io::load: invalid ScalarType enum at ") + std::string(where)
                  + ": raw=" + std::to_string(raw));
  return static_cast<pluss::codem::ScalarType>(raw);
}

inline pluss::codem::Platform resolve_target_platform_(
    pluss::codem::Platform saved_platform,
    const LoadOptions& options) {
  if (options.target_platform.has_value()) return *options.target_platform;
  if (options.preserve_saved_platform) return saved_platform;
  return pluss::codem::Platform::CPU;
}

inline void write_series_payload_(
    BinaryWriter& writer,
    const pluss::Series& value,
    const SaveOptions& options) {
  writer.write_u8(value.defined() ? 1U : 0U, "series.defined");
  if (!value.defined()) return;

  const auto saved_platform = value.platform();

  pluss::Series cpu = value;
  if (!cpu.is_cpu()) {
    if (options.warn_on_platform_downgrade) {
      PLUSS_WARN(
          "pluss::io::save(Series): serializing non-CPU Series from platform="
          + std::to_string(static_cast<std::uint8_t>(saved_platform))
          + " as CPU payload for portability");
    }
    cpu = cpu.to(pluss::codem::Platform::CPU);
  }

  if (!options.canonicalize_series_storage) {
    PLUSS_WARN(
        "pluss::io::save(Series): canonicalize_series_storage=false is not yet implemented. "
        "Proceeding with canonical contiguous CPU storage.");
  }

  cpu = cpu.contiguous();

  const auto sizes = to_sizes_(cpu.sizes());
  const auto dtype = cpu.scalar_type();
  const auto itemsize = static_cast<std::uint64_t>(cpu.itemsize());
  const auto numel = checked_numel_(sizes, "pluss::io::save(Series)");

  PLUSS_CHECK(
      numel <= std::numeric_limits<std::uint64_t>::max() / std::max<std::uint64_t>(itemsize, 1),
      "pluss::io::save(Series): nbytes overflow");
  const auto nbytes = numel * itemsize;

  writer.write_u8(
      options.include_platform_hint ? static_cast<std::uint8_t>(saved_platform)
                                    : static_cast<std::uint8_t>(pluss::codem::Platform::CPU),
      "series.saved_platform");
  writer.write_u8(static_cast<std::uint8_t>(pluss::codem::Platform::CPU), "series.stored_platform");
  writer.write_u8(static_cast<std::uint8_t>(dtype), "series.scalar_type");
  writer.write_u32(static_cast<std::uint32_t>(sizes.size()), "series.rank");
  for (const auto extent : sizes) {
    writer.write_i64(extent, "series.size");
  }
  writer.write_u64(nbytes, "series.nbytes");

  if (nbytes > 0) {
    writer.write_bytes(cpu.raw_data(), static_cast<std::size_t>(nbytes), "series.bytes");
  }
}

inline pluss::Series read_series_payload_(BinaryReader& reader, const LoadOptions& options) {
  const bool defined = reader.read_u8("series.defined") != 0;
  if (!defined) return pluss::Series::undefined();

  const auto saved_platform = decode_platform_(reader.read_u8("series.saved_platform"), "series.saved_platform");
  const auto stored_platform = decode_platform_(reader.read_u8("series.stored_platform"), "series.stored_platform");
  const auto dtype = decode_scalar_type_(reader.read_u8("series.scalar_type"), "series.scalar_type");

  PLUSS_CHECK(
      stored_platform == pluss::codem::Platform::CPU,
      "pluss::io::load(Series): unsupported stored platform in file (expected CPU payload)");

  const auto rank = reader.read_u32("series.rank");
  std::vector<std::int64_t> sizes;
  sizes.reserve(rank);
  for (std::uint32_t i = 0; i < rank; ++i) {
    sizes.push_back(reader.read_i64("series.size"));
  }

  const auto nbytes = reader.read_u64("series.nbytes");
  const auto numel = checked_numel_(sizes, "pluss::io::load(Series)");
  const auto itemsize = static_cast<std::uint64_t>(pluss::codem::dtype_of(dtype).itemsize);
  const auto expected = numel * itemsize;
  PLUSS_CHECK(
      nbytes == expected,
      "pluss::io::load(Series): payload byte size mismatch. file=" + std::to_string(nbytes)
          + ", expected=" + std::to_string(expected));

  const std::size_t alloc_bytes = (nbytes == 0) ? std::size_t{1} : static_cast<std::size_t>(nbytes);
  auto owned = std::make_unique<std::byte[]>(alloc_bytes);
  if (nbytes > 0) {
    reader.read_bytes(owned.get(), static_cast<std::size_t>(nbytes), "series.bytes");
  }

  auto* raw = owned.release();
  pluss::codem::BlobDeleter deleter = [](void* p) {
    delete[] static_cast<std::byte*>(p);
  };

  const auto strides = contiguous_strides_(sizes);
  pluss::Series out = pluss::Series::undefined();
  try {
    out = pluss::from_host(raw, sizes, strides, deleter, dtype);
  } catch (...) {
    delete[] raw;
    throw;
  }

  const auto target = resolve_target_platform_(saved_platform, options);
  if (target != pluss::codem::Platform::CPU) {
    if (!pluss::codem::platform_available(target)) {
      const std::string msg =
          "pluss::io::load(Series): requested platform is unavailable at runtime. requested="
          + std::to_string(static_cast<std::uint8_t>(target));
      if (options.strict_platform) {
        PLUSS_FAIL(msg);
      } else {
        PLUSS_WARN(msg + ". Keeping CPU payload.");
      }
    } else {
      out = out.to(target, dtype);
    }
  }

  return out;
}

inline void write_note_payload_(
    BinaryWriter& writer,
    const pluss::Note& value,
    const SaveOptions& /*options*/) {
  writer.write_u8(value.defined() ? 1U : 0U, "note.defined");
  if (!value.defined()) return;

  const auto sizes = value.sizes();
  const auto numel = static_cast<std::uint64_t>(value.numel());

  const bool note_is_bytes = static_cast<const pluss::coden::NoteBase&>(value).is_bytes();
  writer.write_u8(note_is_bytes ? 1U : 0U, "note.is_bytes");
  writer.write_u32(static_cast<std::uint32_t>(sizes.size()), "note.rank");
  for (const auto extent : sizes) {
    writer.write_i64(extent, "note.size");
  }
  writer.write_u64(numel, "note.numel");

  value.for_each_flat([&](std::string_view bytes) {
    writer.write_u64(static_cast<std::uint64_t>(bytes.size()), "note.elem_size");
    writer.write_bytes(bytes.data(), bytes.size(), "note.elem_bytes");
  });
}

inline pluss::Note read_note_payload_(BinaryReader& reader, const LoadOptions& /*options*/) {
  const bool defined = reader.read_u8("note.defined") != 0;
  if (!defined) return pluss::Note::undefined();

  const bool is_bytes = reader.read_u8("note.is_bytes") != 0;
  const auto rank = reader.read_u32("note.rank");
  PLUSS_CHECK(rank <= 3, "pluss::io::load(Note): rank > 3 is not supported");

  std::vector<std::int64_t> sizes;
  sizes.reserve(rank);
  for (std::uint32_t i = 0; i < rank; ++i) {
    sizes.push_back(reader.read_i64("note.size"));
  }

  const auto numel = reader.read_u64("note.numel");
  const auto expected_numel = checked_numel_(sizes, "pluss::io::load(Note)");
  PLUSS_CHECK(
      numel == expected_numel,
      "pluss::io::load(Note): numel mismatch. file=" + std::to_string(numel)
          + ", expected=" + std::to_string(expected_numel));

  std::vector<std::string> storage;
  storage.resize(static_cast<std::size_t>(numel));
  for (std::uint64_t i = 0; i < numel; ++i) {
    const auto len = reader.read_u64("note.elem_size");
    PLUSS_CHECK(
        len <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),
        "pluss::io::load(Note): note element too large");
    auto& out = storage[static_cast<std::size_t>(i)];
    out.resize(static_cast<std::size_t>(len));
    if (!out.empty()) {
      reader.read_bytes(out.data(), out.size(), "note.elem_bytes");
    }
  }

  std::vector<std::string_view> views;
  views.reserve(storage.size());
  for (const auto& item : storage) {
    views.emplace_back(item.data(), item.size());
  }

  if (is_bytes) {
    return pluss::make_bytes_note(std::span<const std::string_view>(views.data(), views.size()),
                                  std::span<const std::int64_t>(sizes.data(), sizes.size()));
  }

  return pluss::make_note(std::span<const std::string_view>(views.data(), views.size()),
                          std::span<const std::int64_t>(sizes.data(), sizes.size()));
}

inline void write_timeline_payload_(
    BinaryWriter& writer,
    const pluss::Timeline& value,
    const SaveOptions& /*options*/) {
  writer.write_u8(value.defined() ? 1U : 0U, "timeline.defined");
  if (!value.defined()) return;

  const auto sizes = to_sizes_(value.sizes());
  const auto flat = value.to_epoch_nanos_vector();
  const auto numel = checked_numel_(sizes, "pluss::io::save(Timeline)");

  PLUSS_CHECK(
      numel == static_cast<std::uint64_t>(flat.size()),
      "pluss::io::save(Timeline): shape/numel mismatch before serialization");

  writer.write_string(value.tz(), "timeline.tz");
  writer.write_u32(static_cast<std::uint32_t>(sizes.size()), "timeline.rank");
  for (const auto extent : sizes) {
    writer.write_i64(extent, "timeline.size");
  }
  writer.write_u64(numel, "timeline.numel");

  if (!flat.empty()) {
    writer.write_bytes(flat.data(), flat.size() * sizeof(std::int64_t), "timeline.epoch_nanos");
  }
}

inline pluss::Timeline read_timeline_payload_(BinaryReader& reader, const LoadOptions& /*options*/) {
  const bool defined = reader.read_u8("timeline.defined") != 0;
  PLUSS_CHECK(defined, "pluss::io::load(Timeline): undefined timeline payload is not supported");

  const auto tz = reader.read_string("timeline.tz");
  const auto rank = reader.read_u32("timeline.rank");
  PLUSS_CHECK(rank <= 3, "pluss::io::load(Timeline): rank > 3 is not supported");

  std::vector<std::int64_t> sizes;
  sizes.reserve(rank);
  for (std::uint32_t i = 0; i < rank; ++i) {
    sizes.push_back(reader.read_i64("timeline.size"));
  }

  const auto numel = reader.read_u64("timeline.numel");
  const auto expected_numel = checked_numel_(sizes, "pluss::io::load(Timeline)");
  PLUSS_CHECK(
      numel == expected_numel,
      "pluss::io::load(Timeline): numel mismatch. file=" + std::to_string(numel)
          + ", expected=" + std::to_string(expected_numel));

  std::vector<std::int64_t> flat(static_cast<std::size_t>(numel), 0);
  if (!flat.empty()) {
    reader.read_bytes(flat.data(), flat.size() * sizeof(std::int64_t), "timeline.epoch_nanos");
  }

  const std::size_t alloc_count = flat.empty() ? std::size_t{1} : flat.size();
  auto owned = std::make_unique<std::int64_t[]>(alloc_count);
  if (!flat.empty()) {
    std::copy(flat.begin(), flat.end(), owned.get());
  }

  const auto strides = contiguous_strides_(sizes);
  auto* raw = owned.release();
  pluss::codem::BlobDeleter deleter = [](void* p) {
    delete[] static_cast<std::int64_t*>(p);
  };

  try {
    return pluss::codeh::Timeline::from_buffer_epoch_nanos(
        raw,
        std::span<const std::int64_t>(sizes.data(), sizes.size()),
        std::span<const std::int64_t>(strides.data(), strides.size()),
        deleter,
        tz.empty() ? "UTC" : std::string_view(tz));
  } catch (...) {
    delete[] raw;
    throw;
  }
}

inline void write_timeindex_payload_(
    BinaryWriter& writer,
    const pluss::TimeIndex& value,
    const SaveOptions& options);

inline pluss::TimeIndex read_timeindex_payload_(BinaryReader& reader, const LoadOptions& options);

template <class T>
void write_value_(BinaryWriter& writer, const T& value, const SaveOptions& options);

template <class T>
T read_value_(BinaryReader& reader, const LoadOptions& options);

inline void write_timeindex_payload_(
    BinaryWriter& writer,
    const pluss::TimeIndex& value,
    const SaveOptions& options) {
  const auto ticks = value.timeline().to_epoch_nanos_vector();
  const bool require_sorted = is_non_decreasing_(std::span<const std::int64_t>(ticks.data(), ticks.size()));
  writer.write_u8(require_sorted ? 1U : 0U, "timeindex.require_sorted");
  write_value_(writer, value.timeline(), options);
}

inline pluss::TimeIndex read_timeindex_payload_(BinaryReader& reader, const LoadOptions& options) {
  const bool require_sorted = reader.read_u8("timeindex.require_sorted") != 0;
  const auto timeline = read_value_<pluss::Timeline>(reader, options);
  return pluss::make_timeindex(timeline, require_sorted);
}

template <class T>
void write_vector_payload_(BinaryWriter& writer, const T& value, const SaveOptions& options) {
  writer.write_u64(static_cast<std::uint64_t>(value.size()), "container.size");
  for (const auto& item : value) {
    write_value_(writer, item, options);
  }
}

template <class T>
T read_vector_payload_(BinaryReader& reader, const LoadOptions& options) {
  using Elem = typename T::value_type;
  const auto n = reader.read_u64("container.size");
  T out{};
  if constexpr (requires(T t, std::size_t cap) { t.reserve(cap); }) {
    PLUSS_CHECK(
        n <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),
        "pluss::io::load(container): size does not fit into size_t");
    out.reserve(static_cast<std::size_t>(n));
  }
  for (std::uint64_t i = 0; i < n; ++i) {
    if constexpr (requires(T t, Elem&& v) { t.push_back(std::forward<Elem>(v)); }) {
      out.push_back(read_value_<Elem>(reader, options));
    } else {
      static_assert(dependent_false<T>::value, "pluss::io::load(container): unsupported push_back contract");
    }
  }
  return out;
}

template <class T>
void write_array_payload_(BinaryWriter& writer, const T& value, const SaveOptions& options) {
  writer.write_u64(static_cast<std::uint64_t>(value.size()), "array.size");
  for (const auto& item : value) {
    write_value_(writer, item, options);
  }
}

template <class T>
T read_array_payload_(BinaryReader& reader, const LoadOptions& options) {
  using Elem = typename array_traits<T>::value_type;
  constexpr std::size_t N = array_traits<T>::k_size;

  const auto n = reader.read_u64("array.size");
  PLUSS_CHECK(
      n == static_cast<std::uint64_t>(N),
      "pluss::io::load(array): length mismatch. file=" + std::to_string(n)
          + ", expected=" + std::to_string(N));

  T out{};
  for (std::size_t i = 0; i < N; ++i) {
    out[i] = read_value_<Elem>(reader, options);
  }
  return out;
}

template <class T>
void write_value_(BinaryWriter& writer, const T& value, const SaveOptions& options) {
  using U = no_cvref_t<T>;

  if constexpr (std::is_same_v<U, pluss::Series>) {
    write_tag_(writer, Tag::Series);
    write_series_payload_(writer, value, options);
  } else if constexpr (std::is_same_v<U, pluss::Note>) {
    write_tag_(writer, Tag::Note);
    write_note_payload_(writer, value, options);
  } else if constexpr (std::is_same_v<U, pluss::Timeline>) {
    write_tag_(writer, Tag::Timeline);
    write_timeline_payload_(writer, value, options);
  } else if constexpr (std::is_same_v<U, pluss::TimeIndex>) {
    write_tag_(writer, Tag::TimeIndex);
    write_timeindex_payload_(writer, value, options);
  } else if constexpr (is_std_vector<U>::value) {
    write_tag_(writer, Tag::Vector);
    write_vector_payload_(writer, value, options);
  } else if constexpr (is_std_list<U>::value) {
    write_tag_(writer, Tag::List);
    write_vector_payload_(writer, value, options);
  } else if constexpr (is_std_deque<U>::value) {
    write_tag_(writer, Tag::Deque);
    write_vector_payload_(writer, value, options);
  } else if constexpr (is_std_array<U>::value) {
    write_tag_(writer, Tag::Array);
    write_array_payload_(writer, value, options);
  } else if constexpr (is_stack_array_series<U>::value) {
    write_tag_(writer, Tag::StackArraySeries);
    writer.write_u64(static_cast<std::uint64_t>(value.size()), "stackarray.size");
    for (const auto& item : value) {
      write_value_(writer, item, options);
    }
  } else {
    static_assert(dependent_false<U>::value, "pluss::io::save: unsupported type");
  }
}

template <class T>
T read_value_(BinaryReader& reader, const LoadOptions& options) {
  using U = no_cvref_t<T>;
  const auto tag = read_tag_(reader);

  if constexpr (std::is_same_v<U, pluss::Series>) {
    expect_tag_(tag, Tag::Series, "Series");
    return read_series_payload_(reader, options);
  } else if constexpr (std::is_same_v<U, pluss::Note>) {
    expect_tag_(tag, Tag::Note, "Note");
    return read_note_payload_(reader, options);
  } else if constexpr (std::is_same_v<U, pluss::Timeline>) {
    expect_tag_(tag, Tag::Timeline, "Timeline");
    return read_timeline_payload_(reader, options);
  } else if constexpr (std::is_same_v<U, pluss::TimeIndex>) {
    expect_tag_(tag, Tag::TimeIndex, "TimeIndex");
    return read_timeindex_payload_(reader, options);
  } else if constexpr (is_std_vector<U>::value) {
    expect_tag_(tag, Tag::Vector, "std::vector");
    return read_vector_payload_<U>(reader, options);
  } else if constexpr (is_std_list<U>::value) {
    expect_tag_(tag, Tag::List, "std::list");
    return read_vector_payload_<U>(reader, options);
  } else if constexpr (is_std_deque<U>::value) {
    expect_tag_(tag, Tag::Deque, "std::deque");
    return read_vector_payload_<U>(reader, options);
  } else if constexpr (is_std_array<U>::value) {
    expect_tag_(tag, Tag::Array, "std::array");
    return read_array_payload_<U>(reader, options);
  } else if constexpr (is_stack_array_series<U>::value) {
    expect_tag_(tag, Tag::StackArraySeries, "StackArray<Series>");
    const auto n = reader.read_u64("stackarray.size");
    PLUSS_CHECK(
        n <= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),
        "pluss::io::load(StackArray<Series>): size does not fit into size_t");
    std::vector<pluss::Series> values;
    values.reserve(static_cast<std::size_t>(n));
    for (std::uint64_t i = 0; i < n; ++i) {
      values.push_back(read_value_<pluss::Series>(reader, options));
    }
    return pluss::StackArray<pluss::Series>(std::move(values));
  } else {
    static_assert(dependent_false<U>::value, "pluss::io::load: unsupported type");
  }
}

} // namespace detail

template <class T>
concept SerializableValue = detail::serializable_value_v<detail::no_cvref_t<T>>;

template <SerializableValue T>
inline void save(const T& value, std::ostream& out, const SaveOptions& options = {}) {
  detail::BinaryWriter writer(out);
  detail::write_header_(writer, options);
  detail::write_value_(writer, value, options);
}

template <SerializableValue T>
inline void save(const T& value, std::string_view path, const SaveOptions& options = {}) {
  const std::filesystem::path p(path);
  PLUSS_CHECK(!p.empty(), "pluss::io::save: output path cannot be empty");

  if (options.create_parent_dirs) {
    const auto parent = p.parent_path();
    if (!parent.empty()) {
      std::error_code ec;
      std::filesystem::create_directories(parent, ec);
      PLUSS_CHECK(
          !ec,
          "pluss::io::save: failed to create parent directory '" + parent.string() + "': " + ec.message());
    }
  }

  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  PLUSS_CHECK(
      out.is_open(),
      "pluss::io::save: failed to open output file '" + p.string() + "'");

  save(value, out, options);
  out.flush();
  PLUSS_CHECK(
      out.good(),
      "pluss::io::save: flush failed for '" + p.string() + "'");
}

template <SerializableValue T>
inline T load(std::istream& in, const LoadOptions& options = {}) {
  detail::BinaryReader reader(in);
  detail::read_header_(reader);
  return detail::read_value_<detail::no_cvref_t<T>>(reader, options);
}

template <SerializableValue T>
inline T load(std::string_view path, const LoadOptions& options = {}) {
  const std::filesystem::path p(path);
  PLUSS_CHECK(!p.empty(), "pluss::io::load: input path cannot be empty");

  std::ifstream in(p, std::ios::binary);
  PLUSS_CHECK(
      in.is_open(),
      "pluss::io::load: failed to open input file '" + p.string() + "'");

  return load<T>(in, options);
}

template <SerializableValue T>
inline void load(T& out, std::istream& in, const LoadOptions& options = {}) {
  out = load<T>(in, options);
}

template <SerializableValue T>
inline void load(T& out, std::string_view path, const LoadOptions& options = {}) {
  out = load<T>(path, options);
}

} // namespace pluss::io

namespace pluss {
using io::Compression;
using io::LoadOptions;
using io::SaveOptions;
using io::load;
using io::save;
} // namespace pluss
