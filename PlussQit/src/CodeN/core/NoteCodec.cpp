#include "CodeN/core/Note.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CodeN/core/OpsInternal.h"

namespace {

using Byte = ::std::byte;

constexpr ::std::array<Byte, 4> kMagic{
    Byte{'P'}, Byte{'N'}, Byte{'O'}, Byte{'T'}};
constexpr ::std::uint8_t kVersion = 1;
constexpr ::std::uint8_t kUndefined = 0xff;

void append_u8(::std::vector<Byte>& out, ::std::uint8_t value) {
  out.push_back(static_cast<Byte>(value));
}

void append_u64(::std::vector<Byte>& out, ::std::uint64_t value) {
  for (unsigned shift = 0; shift < 64; shift += 8) {
    append_u8(out, static_cast<::std::uint8_t>(value >> shift));
  }
}

void append_bytes(::std::vector<Byte>& out, const void* data, ::std::size_t size) {
  if (size == 0) return;
  const auto* first = static_cast<const Byte*>(data);
  out.insert(out.end(), first, first + size);
}

class Reader {
public:
  explicit Reader(::std::span<const Byte> input) : input_(input) {}

  ::std::uint8_t u8() {
    require(1);
    return static_cast<::std::uint8_t>(input_[at_++]);
  }

  ::std::uint64_t u64() {
    require(8);
    ::std::uint64_t value = 0;
    for (unsigned shift = 0; shift < 64; shift += 8) {
      value |= static_cast<::std::uint64_t>(u8()) << shift;
    }
    return value;
  }

  ::std::span<const Byte> bytes(::std::size_t size) {
    require(size);
    const auto result = input_.subspan(at_, size);
    at_ += size;
    return result;
  }

  bool done() const noexcept { return at_ == input_.size(); }
  ::std::size_t remaining() const noexcept { return input_.size() - at_; }

private:
  void require(::std::size_t size) const {
    PLUSS_CHECK(size <= input_.size() - at_,
                "pluss::note::unpack: truncated input");
  }

  ::std::span<const Byte> input_;
  ::std::size_t at_{0};
};

::std::int64_t checked_i64(::std::uint64_t value, ::std::string_view field) {
  PLUSS_CHECK(value <= static_cast<::std::uint64_t>(
                           ::std::numeric_limits<::std::int64_t>::max()),
              "pluss::note::unpack: " + ::std::string(field) +
                  " exceeds int64");
  return static_cast<::std::int64_t>(value);
}

::std::size_t checked_size(::std::uint64_t value, ::std::string_view field) {
  PLUSS_CHECK(value <= static_cast<::std::uint64_t>(
                           ::std::numeric_limits<::std::size_t>::max()),
              "pluss::note::unpack: " + ::std::string(field) +
                  " exceeds addressable memory");
  return static_cast<::std::size_t>(value);
}

} // namespace

namespace pluss::coden {

::std::vector<::std::byte> Note::pack() const {
  ::std::vector<Byte> out;
  out.reserve(40 + (defined() ? static_cast<::std::size_t>(numel()) * 8 : 0));
  out.insert(out.end(), kMagic.begin(), kMagic.end());
  append_u8(out, kVersion);
  append_u8(out, defined() ? static_cast<::std::uint8_t>(dtype()) : kUndefined);
  append_u8(out, defined() ? static_cast<::std::uint8_t>(dim()) : 0);
  append_u8(out, 0);
  append_u64(out, defined() ? static_cast<::std::uint64_t>(numel()) : 0);

  const auto shape = defined() ? sizes_view() : ::std::span<const ::std::int64_t>{};
  for (::std::size_t axis = 0; axis < 3; ++axis) {
    append_u64(out, axis < shape.size()
                        ? static_cast<::std::uint64_t>(shape[axis])
                        : 0);
  }
  if (!defined()) return out;

  switch (dtype()) {
    case NoteDType::Text:
    case NoteDType::Bytes:
      for (::std::int64_t i = 0; i < numel(); ++i) {
        const auto value = at(i);
        append_u64(out, static_cast<::std::uint64_t>(value.size()));
        append_bytes(out, value.data(), value.size());
      }
      break;
    case NoteDType::Bool:
      append_bytes(out, bools().data(), bools().size());
      break;
    case NoteDType::Int64:
      for (const auto value : integers()) {
        append_u64(out, ::std::bit_cast<::std::uint64_t>(value));
      }
      break;
    case NoteDType::Float64:
      for (const auto value : reals()) {
        append_u64(out, ::std::bit_cast<::std::uint64_t>(value));
      }
      break;
  }
  return out;
}

} // namespace pluss::coden

namespace pluss::note {

CODEN_API pluss::coden::Note unpack(::std::span<const ::std::byte> encoded) {
  Reader reader(encoded);
  for (const auto expected : kMagic) {
    PLUSS_CHECK(reader.u8() == static_cast<::std::uint8_t>(expected),
                "pluss::note::unpack: invalid Note envelope");
  }
  PLUSS_CHECK(reader.u8() == kVersion,
              "pluss::note::unpack: unsupported format version");
  const auto dtype_tag = reader.u8();
  const auto rank = reader.u8();
  PLUSS_CHECK(reader.u8() == 0,
              "pluss::note::unpack: unsupported envelope flags");
  const auto count = checked_i64(reader.u64(), "element count");

  ::std::array<::std::int64_t, 3> extents{};
  for (auto& extent : extents) extent = checked_i64(reader.u64(), "extent");

  if (dtype_tag == kUndefined) {
    PLUSS_CHECK(rank == 0 && count == 0 && extents == decltype(extents){} &&
                    reader.done(),
                "pluss::note::unpack: malformed undefined Note");
    return pluss::coden::Note::undefined();
  }

  PLUSS_CHECK(rank <= 3, "pluss::note::unpack: rank must be at most 3");
  for (::std::size_t axis = rank; axis < 3; ++axis) {
    PLUSS_CHECK(extents[axis] == 0,
                "pluss::note::unpack: unused extents must be zero");
  }
  const auto shape = ::std::span<const ::std::int64_t>(extents.data(), rank);
  ::std::int64_t expected_count = 1;
  for (const auto extent : shape) {
    PLUSS_CHECK(expected_count == 0 ||
                    extent <= ::std::numeric_limits<::std::int64_t>::max() /
                                  expected_count,
                "pluss::note::unpack: shape cardinality overflows int64");
    expected_count *= extent;
  }
  PLUSS_CHECK(count == expected_count,
              "pluss::note::unpack: element count does not match shape");

  pluss::coden::Note result = pluss::coden::Note::undefined();
  const auto dtype = static_cast<pluss::coden::NoteDType>(dtype_tag);
  switch (dtype) {
    case pluss::coden::NoteDType::Text:
    case pluss::coden::NoteDType::Bytes: {
      PLUSS_CHECK(static_cast<::std::uint64_t>(count) <= reader.remaining() / 8,
                  "pluss::note::unpack: truncated text length table");
      ::std::vector<::std::string_view> values;
      values.reserve(static_cast<::std::size_t>(count));
      for (::std::int64_t i = 0; i < count; ++i) {
        const auto bytes = reader.bytes(checked_size(reader.u64(), "cell length"));
        values.emplace_back(reinterpret_cast<const char*>(bytes.data()), bytes.size());
      }
      result = dtype == pluss::coden::NoteDType::Text
                   ? pluss::coden::ops_internal::note_from_utf8(values, shape)
                   : pluss::coden::ops_internal::note_from_bytes(values, shape);
      break;
    }
    case pluss::coden::NoteDType::Bool: {
      const auto bytes = reader.bytes(static_cast<::std::size_t>(count));
      result = pluss::coden::ops_internal::note_from_bool(
          {reinterpret_cast<const ::std::uint8_t*>(bytes.data()), bytes.size()},
          shape);
      break;
    }
    case pluss::coden::NoteDType::Int64: {
      PLUSS_CHECK(static_cast<::std::uint64_t>(count) <= reader.remaining() / 8,
                  "pluss::note::unpack: truncated Int64 payload");
      pluss::coden::NoteImpl::Int64Storage values(
          static_cast<::std::size_t>(count));
      for (auto& value : values) {
        value = ::std::bit_cast<::std::int64_t>(reader.u64());
      }
      result = pluss::coden::ops_internal::note_from_int64_storage(
          ::std::move(values), shape);
      break;
    }
    case pluss::coden::NoteDType::Float64: {
      PLUSS_CHECK(static_cast<::std::uint64_t>(count) <= reader.remaining() / 8,
                  "pluss::note::unpack: truncated Float64 payload");
      pluss::coden::NoteImpl::Float64Storage values(
          static_cast<::std::size_t>(count));
      for (auto& value : values) {
        value = ::std::bit_cast<double>(reader.u64());
      }
      result = pluss::coden::ops_internal::note_from_float64_storage(
          ::std::move(values), shape);
      break;
    }
    default:
      PLUSS_FAIL("pluss::note::unpack: unknown dtype tag");
  }
  PLUSS_CHECK(reader.done(), "pluss::note::unpack: trailing bytes");
  return result;
}

} // namespace pluss::note
