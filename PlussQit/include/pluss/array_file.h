#pragma once

// Array files: one Series in a .npy file, several named Series in a .npz file.
//
//   pluss::io::save_npy(series, "x.npy");
//   pluss::Series x = pluss::io::load_npy("x.npy");
//   pluss::io::save_npz({{"x", x}, {"y", y}}, "xy.npz");
//   auto arrays = pluss::io::load_npz("xy.npz");   // [("x", Series), ("y", Series)]
//
// The formats are the common array-exchange ones, so these files open in other numerical tools.
// Every dtype that has a counterpart is supported (bool, 8 to 64 bit integers, 16 to 64 bit
// floats, complex64 and complex128); bfloat16 and the 8-bit floats have none, so saving one is an
// error that says to convert first. Reading accepts either byte order and column-major data, and a
// .npz may be stored or deflated. Nothing here needs another library.

#include "pluss/codem/series_factory.h"
#include "pluss/exception.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::io {
namespace array_detail {

static_assert(std::endian::native == std::endian::little, "array files are written little-endian from a little-endian host");

// ---- the descriptor of a dtype -----------------------------------------------------------------

struct Kind {
  const char* descr;  // as written in a header, little-endian
  pluss::codem::ScalarType type;
  std::size_t size;
};

inline constexpr std::array<Kind, 14> kKinds{{
    {"|b1", pluss::codem::ScalarType::Bool, 1},
    {"|u1", pluss::codem::ScalarType::UInt8, 1},
    {"<u2", pluss::codem::ScalarType::UInt16, 2},
    {"<u4", pluss::codem::ScalarType::UInt32, 4},
    {"<u8", pluss::codem::ScalarType::UInt64, 8},
    {"|i1", pluss::codem::ScalarType::Int8, 1},
    {"<i2", pluss::codem::ScalarType::Int16, 2},
    {"<i4", pluss::codem::ScalarType::Int32, 4},
    {"<i8", pluss::codem::ScalarType::Int64, 8},
    {"<f2", pluss::codem::ScalarType::Float16, 2},
    {"<f4", pluss::codem::ScalarType::Float32, 4},
    {"<f8", pluss::codem::ScalarType::Float64, 8},
    {"<c8", pluss::codem::ScalarType::Complex64, 8},
    {"<c16", pluss::codem::ScalarType::Complex128, 16},
}};

[[nodiscard]] inline const Kind& kind_of(pluss::codem::ScalarType type) {
  for (const auto& kind : kKinds) {
    if (kind.type == type) return kind;
  }
  PLUSS_FAIL("pluss::io::save_npy: this dtype has no array-file form; convert it first (astype to Float32 or Float64)");
}

// ---- reading and writing a .npy in memory -------------------------------------------------------

[[nodiscard]] inline std::string npy_bytes(const pluss::Series& value) {
  PLUSS_CHECK(value.defined(), "pluss::io::save_npy: the Series is undefined");
  pluss::Series cpu = value.is_cpu() ? value : value.to(pluss::codem::Platform::CPU);
  cpu = cpu.contiguous();
  const Kind& kind = kind_of(cpu.scalar_type());

  std::string shape = "(";
  const auto sizes = cpu.sizes();
  std::size_t count = 1;
  std::size_t rank = 0;
  for (const auto extent : sizes) {
    if (rank > 0) shape += ", ";
    shape += std::to_string(extent);
    count *= static_cast<std::size_t>(extent);
    ++rank;
  }
  if (rank == 1) shape += ",";
  shape += ")";

  std::string header = std::string("{'descr': '") + kind.descr + "', 'fortran_order': False, 'shape': " + shape + ", }";
  // The header ends in a newline and the whole preamble is a multiple of 64 bytes.
  const std::size_t preamble = 10;
  std::size_t total = preamble + header.size() + 1;
  const std::size_t padding = (64 - total % 64) % 64;
  header.append(padding, ' ');
  header += '\n';
  PLUSS_CHECK(header.size() <= 65535, "pluss::io::save_npy: the header is too long for a version 1 file");

  std::string out;
  out.reserve(preamble + header.size() + count * kind.size);
  out += "\x93NUMPY";
  out += '\x01';
  out += '\x00';
  out += static_cast<char>(header.size() & 0xFF);
  out += static_cast<char>((header.size() >> 8) & 0xFF);
  out += header;
  if (count > 0) out.append(reinterpret_cast<const char*>(cpu.raw_data()), count * kind.size);
  return out;
}

struct Header {
  std::string descr;
  bool fortran{false};
  std::vector<std::int64_t> shape;
  std::size_t data_offset{0};
};

[[nodiscard]] inline Header parse_header(std::string_view bytes, std::string_view where) {
  const auto fail = [&](const std::string& why) { PLUSS_FAIL(std::string("pluss::io::load_npy: ") + std::string(where) + ": " + why); };
  if (bytes.size() < 10 || bytes.substr(0, 6) != std::string_view("\x93NUMPY", 6)) fail("not an array file (the magic bytes are missing)");
  const int major = static_cast<unsigned char>(bytes[6]);
  std::size_t length = 0, start = 0;
  if (major == 1) {
    length = static_cast<unsigned char>(bytes[8]) | (static_cast<std::size_t>(static_cast<unsigned char>(bytes[9])) << 8);
    start = 10;
  } else if (major == 2 || major == 3) {
    if (bytes.size() < 12) fail("the header is cut off");
    length = 0;
    for (int i = 0; i < 4; ++i) length |= static_cast<std::size_t>(static_cast<unsigned char>(bytes[8 + i])) << (8 * i);
    start = 12;
  } else {
    fail("unsupported format version " + std::to_string(major));
  }
  if (start + length > bytes.size()) fail("the header is cut off");
  const std::string_view dict = bytes.substr(start, length);

  Header out;
  out.data_offset = start + length;

  const auto value_after = [&](std::string_view key) -> std::string_view {
    const std::string quoted = "'" + std::string(key) + "'";
    const auto at = dict.find(quoted);
    if (at == std::string_view::npos) fail("the header has no " + quoted);
    auto from = dict.find(':', at + quoted.size());
    if (from == std::string_view::npos) fail("the header is malformed near " + quoted);
    ++from;
    while (from < dict.size() && dict[from] == ' ') ++from;
    return dict.substr(from);
  };

  const std::string_view descr = value_after("descr");
  if (descr.empty() || descr[0] != '\'') fail("structured dtypes are not supported");
  const auto end = descr.find('\'', 1);
  if (end == std::string_view::npos) fail("the dtype is not closed");
  out.descr = std::string(descr.substr(1, end - 1));

  const std::string_view fortran = value_after("fortran_order");
  out.fortran = fortran.substr(0, 4) == "True";

  std::string_view shape = value_after("shape");
  if (shape.empty() || shape[0] != '(') fail("the shape is malformed");
  const auto close = shape.find(')');
  if (close == std::string_view::npos) fail("the shape is not closed");
  shape = shape.substr(1, close - 1);
  std::size_t at = 0;
  while (at < shape.size()) {
    while (at < shape.size() && (shape[at] == ' ' || shape[at] == ',')) ++at;
    if (at >= shape.size()) break;
    std::size_t used = 0;
    const std::string piece(shape.substr(at));
    long long extent = 0;
    try {
      extent = std::stoll(piece, &used);
    } catch (const std::exception&) {
      fail("the shape is malformed");
    }
    if (extent < 0) fail("the shape has a negative extent");
    out.shape.push_back(extent);
    at += used;
  }
  return out;
}

[[nodiscard]] inline const Kind& kind_from(std::string_view descr, bool& swap, std::string_view where) {
  const auto fail = [&](const std::string& why) { PLUSS_FAIL(std::string("pluss::io::load_npy: ") + std::string(where) + ": " + why); };
  if (descr.size() < 3) fail("unreadable dtype '" + std::string(descr) + "'");
  const char order = descr[0];
  if (order != '<' && order != '>' && order != '|' && order != '=') fail("unreadable dtype '" + std::string(descr) + "'");
  const std::string tail(descr.substr(1));
  swap = order == '>';
  for (const auto& kind : kKinds) {
    if (std::string_view(kind.descr).substr(1) == tail) return kind;
  }
  fail("the dtype '" + std::string(descr) + "' has no counterpart (text, objects and half-complex types are not supported)");
  return kKinds[0];
}

[[nodiscard]] inline pluss::Series series_from_npy(std::string_view bytes, std::string_view where = "array") {
  const Header header = parse_header(bytes, where);
  bool swap = false;
  const Kind& kind = kind_from(header.descr, swap, where);

  std::uint64_t count = 1;
  for (const auto extent : header.shape) count *= static_cast<std::uint64_t>(extent);
  const std::uint64_t nbytes = count * kind.size;
  PLUSS_CHECK(bytes.size() >= header.data_offset + nbytes,
              std::string("pluss::io::load_npy: ") + std::string(where) + ": the data is shorter than the shape says");

  const std::size_t alloc = nbytes == 0 ? 1 : static_cast<std::size_t>(nbytes);
  auto owned = std::make_unique<std::byte[]>(alloc);
  const char* source = bytes.data() + header.data_offset;

  // Column-major data is gathered into row-major order: the element at (i0, i1, ...) of the file
  // sits at i0 + n0 * (i1 + n1 * (...)).
  if (header.fortran && header.shape.size() > 1 && count > 0) {
    const std::size_t rank = header.shape.size();
    std::vector<std::int64_t> index(rank, 0);
    for (std::uint64_t out = 0; out < count; ++out) {
      std::uint64_t from = 0, stride = 1;
      for (std::size_t d = 0; d < rank; ++d) {
        from += static_cast<std::uint64_t>(index[d]) * stride;
        stride *= static_cast<std::uint64_t>(header.shape[d]);
      }
      std::memcpy(owned.get() + out * kind.size, source + from * kind.size, kind.size);
      for (std::size_t d = rank; d-- > 0;) {
        if (++index[d] < header.shape[d]) break;
        index[d] = 0;
      }
    }
  } else if (nbytes > 0) {
    std::memcpy(owned.get(), source, static_cast<std::size_t>(nbytes));
  }

  if (swap && kind.size > 1 && nbytes > 0) {
    // A complex number is two floats, each swapped on its own.
    const std::size_t unit = (kind.type == pluss::codem::ScalarType::Complex64) ? 4
                             : (kind.type == pluss::codem::ScalarType::Complex128) ? 8 : kind.size;
    auto* p = reinterpret_cast<unsigned char*>(owned.get());
    for (std::uint64_t i = 0; i < nbytes / unit; ++i) std::reverse(p + i * unit, p + (i + 1) * unit);
  }

  std::vector<std::int64_t> strides(header.shape.size(), 1);
  for (std::size_t d = header.shape.size(); d-- > 1;) strides[d - 1] = strides[d] * std::max<std::int64_t>(header.shape[d], 1);
  auto* raw = owned.release();
  pluss::codem::BlobDeleter deleter = [](void* p) { delete[] static_cast<std::byte*>(p); };
  try {
    return pluss::series::from_host(raw, header.shape, strides, deleter, kind.type);
  } catch (...) {
    delete[] raw;
    throw;
  }
}

// ---- files -------------------------------------------------------------------------------------

[[nodiscard]] inline std::string read_file(const std::filesystem::path& path, std::string_view who) {
  std::ifstream in(path, std::ios::binary);
  PLUSS_CHECK(in.good(), std::string(who) + ": cannot open '" + path.string() + "'");
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

inline void write_file(const std::filesystem::path& path, std::string_view bytes, std::string_view who) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  PLUSS_CHECK(out.good(), std::string(who) + ": cannot write '" + path.string() + "'");
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  PLUSS_CHECK(out.good(), std::string(who) + ": writing '" + path.string() + "' failed");
}

// ---- CRC-32 and DEFLATE (reading only) ----------------------------------------------------------

[[nodiscard]] inline std::uint32_t crc32(std::string_view bytes) {
  static const auto table = [] {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t c = i;
      for (int k = 0; k < 8; ++k) c = (c & 1U) != 0 ? 0xEDB88320U ^ (c >> 1) : c >> 1;
      t[i] = c;
    }
    return t;
  }();
  std::uint32_t c = 0xFFFFFFFFU;
  for (const char ch : bytes) c = table[(c ^ static_cast<unsigned char>(ch)) & 0xFF] ^ (c >> 8);
  return c ^ 0xFFFFFFFFU;
}

// RFC 1951, as zip files use it: stored, fixed-Huffman and dynamic-Huffman blocks.
class Inflater {
 public:
  Inflater(std::string_view in, std::size_t expected) : in_(in) { out_.reserve(expected); }

  std::string run() {
    bool last = false;
    while (!last) {
      last = bits_(1) != 0;
      const unsigned type = bits_(2);
      if (type == 0) {
        stored_();
      } else if (type == 1) {
        fixed_();
        block_(literal_, distance_);
      } else if (type == 2) {
        dynamic_();
        block_(literal_, distance_);
      } else {
        PLUSS_FAIL("pluss::io::load_npz: a compressed member has a bad block type");
      }
    }
    return std::move(out_);
  }

 private:
  struct Table {
    std::array<std::uint16_t, 16> count{};
    std::array<std::uint16_t, 288> symbol{};
  };

  [[nodiscard]] unsigned bits_(unsigned need) {
    unsigned value = 0;
    for (unsigned i = 0; i < need; ++i) {
      if (bit_count_ == 0) {
        PLUSS_CHECK(at_ < in_.size(), "pluss::io::load_npz: a compressed member is cut off");
        bit_buffer_ = static_cast<unsigned char>(in_[at_++]);
        bit_count_ = 8;
      }
      value |= (bit_buffer_ & 1U) << i;
      bit_buffer_ >>= 1;
      --bit_count_;
    }
    return value;
  }

  void stored_() {
    bit_count_ = 0;
    PLUSS_CHECK(at_ + 4 <= in_.size(), "pluss::io::load_npz: a stored block is cut off");
    const unsigned length = static_cast<unsigned char>(in_[at_]) | (static_cast<unsigned>(static_cast<unsigned char>(in_[at_ + 1])) << 8);
    at_ += 4;
    PLUSS_CHECK(at_ + length <= in_.size(), "pluss::io::load_npz: a stored block is cut off");
    out_.append(in_.substr(at_, length));
    at_ += length;
  }

  static void build_(Table& table, const std::uint8_t* lengths, std::size_t n) {
    table.count.fill(0);
    for (std::size_t i = 0; i < n; ++i) ++table.count[lengths[i]];
    table.count[0] = 0;
    std::array<std::uint16_t, 16> offsets{};
    for (int len = 1; len < 15; ++len) offsets[len + 1] = static_cast<std::uint16_t>(offsets[len] + table.count[len]);
    for (std::size_t i = 0; i < n; ++i) {
      if (lengths[i] != 0) table.symbol[offsets[lengths[i]]++] = static_cast<std::uint16_t>(i);
    }
  }

  [[nodiscard]] unsigned decode_(const Table& table) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; ++len) {
      code |= static_cast<int>(bits_(1));
      const int count = table.count[len];
      if (code - count < first) return table.symbol[index + (code - first)];
      index += count;
      first += count;
      first <<= 1;
      code <<= 1;
    }
    PLUSS_FAIL("pluss::io::load_npz: a compressed member has an invalid code");
  }

  void fixed_() {
    std::uint8_t lengths[288];
    for (int i = 0; i < 144; ++i) lengths[i] = 8;
    for (int i = 144; i < 256; ++i) lengths[i] = 9;
    for (int i = 256; i < 280; ++i) lengths[i] = 7;
    for (int i = 280; i < 288; ++i) lengths[i] = 8;
    build_(literal_, lengths, 288);
    std::uint8_t d[30];
    for (auto& x : d) x = 5;
    build_(distance_, d, 30);
  }

  void dynamic_() {
    static constexpr std::array<std::uint8_t, 19> order{16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    const unsigned nlen = bits_(5) + 257, ndist = bits_(5) + 1, ncode = bits_(4) + 4;
    PLUSS_CHECK(nlen <= 286 && ndist <= 30, "pluss::io::load_npz: a compressed member has bad table sizes");
    std::uint8_t lengths[320] = {};
    for (unsigned i = 0; i < ncode; ++i) lengths[order[i]] = static_cast<std::uint8_t>(bits_(3));
    Table code;
    build_(code, lengths, 19);
    std::uint8_t all[320] = {};
    unsigned index = 0;
    while (index < nlen + ndist) {
      const unsigned symbol = decode_(code);
      if (symbol < 16) {
        all[index++] = static_cast<std::uint8_t>(symbol);
      } else {
        std::uint8_t previous = 0;
        unsigned repeat = 0;
        if (symbol == 16) {
          PLUSS_CHECK(index > 0, "pluss::io::load_npz: a compressed member repeats nothing");
          previous = all[index - 1];
          repeat = 3 + bits_(2);
        } else if (symbol == 17) {
          repeat = 3 + bits_(3);
        } else {
          repeat = 11 + bits_(7);
        }
        PLUSS_CHECK(index + repeat <= nlen + ndist, "pluss::io::load_npz: a compressed member repeats too far");
        while (repeat-- > 0) all[index++] = previous;
      }
    }
    build_(literal_, all, nlen);
    build_(distance_, all + nlen, ndist);
  }

  void block_(const Table& literal, const Table& distance) {
    static constexpr std::array<std::uint16_t, 29> base{3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static constexpr std::array<std::uint8_t, 29> extra{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static constexpr std::array<std::uint16_t, 30> dbase{1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static constexpr std::array<std::uint8_t, 30> dextra{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    for (;;) {
      unsigned symbol = decode_(literal);
      if (symbol < 256) {
        out_ += static_cast<char>(symbol);
      } else if (symbol == 256) {
        return;
      } else {
        symbol -= 257;
        PLUSS_CHECK(symbol < 29, "pluss::io::load_npz: a compressed member has a bad length code");
        const unsigned length = base[symbol] + bits_(extra[symbol]);
        const unsigned ds = decode_(distance);
        PLUSS_CHECK(ds < 30, "pluss::io::load_npz: a compressed member has a bad distance code");
        const unsigned dist = dbase[ds] + bits_(dextra[ds]);
        PLUSS_CHECK(dist <= out_.size(), "pluss::io::load_npz: a compressed member points before its start");
        for (unsigned i = 0; i < length; ++i) out_ += out_[out_.size() - dist];
      }
    }
  }

  std::string_view in_;
  std::string out_;
  std::size_t at_{0};
  unsigned bit_buffer_{0};
  unsigned bit_count_{0};
  Table literal_{};
  Table distance_{};
};

[[nodiscard]] inline std::uint32_t u32_at(std::string_view s, std::size_t at) {
  PLUSS_CHECK(at + 4 <= s.size(), "pluss::io::load_npz: the file is cut off");
  return static_cast<unsigned char>(s[at]) | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[at + 1])) << 8) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(s[at + 2])) << 16) |
         (static_cast<std::uint32_t>(static_cast<unsigned char>(s[at + 3])) << 24);
}

[[nodiscard]] inline std::uint32_t u16_at(std::string_view s, std::size_t at) {
  PLUSS_CHECK(at + 2 <= s.size(), "pluss::io::load_npz: the file is cut off");
  return static_cast<unsigned char>(s[at]) | (static_cast<std::uint32_t>(static_cast<unsigned char>(s[at + 1])) << 8);
}

inline void put_u16(std::string& out, std::uint32_t v) {
  out += static_cast<char>(v & 0xFF);
  out += static_cast<char>((v >> 8) & 0xFF);
}

inline void put_u32(std::string& out, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) out += static_cast<char>((v >> (8 * i)) & 0xFF);
}

}  // namespace array_detail

// One array, one file.
inline void save_npy(const pluss::Series& value, const std::filesystem::path& path) {
  array_detail::write_file(path, array_detail::npy_bytes(value), "pluss::io::save_npy");
}

[[nodiscard]] inline pluss::Series load_npy(const std::filesystem::path& path) {
  const std::string bytes = array_detail::read_file(path, "pluss::io::load_npy");
  return array_detail::series_from_npy(bytes, path.string());
}

// Several named arrays in one file. The names become the members' names ("x" is stored as x.npy).
inline void save_npz(const std::vector<std::pair<std::string, pluss::Series>>& arrays, const std::filesystem::path& path) {
  using namespace array_detail;
  PLUSS_CHECK(arrays.size() < 65535, "pluss::io::save_npz: too many arrays for a zip file without zip64");
  std::string body, central;
  for (const auto& [name, value] : arrays) {
    PLUSS_CHECK(!name.empty(), "pluss::io::save_npz: an array needs a name");
    const std::string member = name + ".npy";
    const std::string data = npy_bytes(value);
    PLUSS_CHECK(data.size() < 0xFFFFFFFFULL && body.size() < 0xFFFFFFFFULL, "pluss::io::save_npz: an array is too large for a zip file without zip64");
    const std::uint32_t crc = crc32(data);
    const std::uint32_t offset = static_cast<std::uint32_t>(body.size());

    put_u32(body, 0x04034b50);
    put_u16(body, 20);  // version needed
    put_u16(body, 0);   // flags
    put_u16(body, 0);   // stored
    put_u16(body, 0);   // time
    put_u16(body, 0x21);  // date: 1980-01-01
    put_u32(body, crc);
    put_u32(body, static_cast<std::uint32_t>(data.size()));
    put_u32(body, static_cast<std::uint32_t>(data.size()));
    put_u16(body, static_cast<std::uint32_t>(member.size()));
    put_u16(body, 0);
    body += member;
    body += data;

    put_u32(central, 0x02014b50);
    put_u16(central, 20);
    put_u16(central, 20);
    put_u16(central, 0);
    put_u16(central, 0);
    put_u16(central, 0);
    put_u16(central, 0x21);
    put_u32(central, crc);
    put_u32(central, static_cast<std::uint32_t>(data.size()));
    put_u32(central, static_cast<std::uint32_t>(data.size()));
    put_u16(central, static_cast<std::uint32_t>(member.size()));
    put_u16(central, 0);  // extra
    put_u16(central, 0);  // comment
    put_u16(central, 0);  // disk
    put_u16(central, 0);  // internal attributes
    put_u32(central, 0);  // external attributes
    put_u32(central, offset);
    central += member;
  }
  std::string file = body;
  const std::uint32_t central_at = static_cast<std::uint32_t>(file.size());
  file += central;
  put_u32(file, 0x06054b50);
  put_u16(file, 0);
  put_u16(file, 0);
  put_u16(file, static_cast<std::uint32_t>(arrays.size()));
  put_u16(file, static_cast<std::uint32_t>(arrays.size()));
  put_u32(file, static_cast<std::uint32_t>(central.size()));
  put_u32(file, central_at);
  put_u16(file, 0);
  write_file(path, file, "pluss::io::save_npz");
}

[[nodiscard]] inline std::vector<std::pair<std::string, pluss::Series>> load_npz(const std::filesystem::path& path) {
  using namespace array_detail;
  const std::string file = read_file(path, "pluss::io::load_npz");
  // The end-of-directory record is the last 22 bytes unless the file ends in a comment.
  std::size_t eocd = std::string::npos;
  for (std::size_t back = 22; back <= std::min<std::size_t>(file.size(), 22 + 65535); ++back) {
    if (u32_at(file, file.size() - back) == 0x06054b50) {
      eocd = file.size() - back;
      break;
    }
  }
  PLUSS_CHECK(eocd != std::string::npos, "pluss::io::load_npz: '" + path.string() + "' is not a zip file");
  const std::size_t entries = u16_at(file, eocd + 10);
  std::size_t at = u32_at(file, eocd + 16);

  std::vector<std::pair<std::string, pluss::Series>> out;
  for (std::size_t i = 0; i < entries; ++i) {
    PLUSS_CHECK(u32_at(file, at) == 0x02014b50, "pluss::io::load_npz: the zip directory is damaged");
    const std::uint32_t method = u16_at(file, at + 10);
    const std::uint32_t crc = u32_at(file, at + 16);
    const std::size_t csize = u32_at(file, at + 20);
    const std::size_t usize = u32_at(file, at + 24);
    const std::size_t name_len = u16_at(file, at + 28), extra_len = u16_at(file, at + 30), comment_len = u16_at(file, at + 32);
    const std::size_t local = u32_at(file, at + 42);
    PLUSS_CHECK(at + 46 + name_len <= file.size(), "pluss::io::load_npz: the zip directory is cut off");
    std::string name = file.substr(at + 46, name_len);
    at += 46 + name_len + extra_len + comment_len;
    PLUSS_CHECK(csize != 0xFFFFFFFFU && usize != 0xFFFFFFFFU && local != 0xFFFFFFFFU,
                "pluss::io::load_npz: zip64 files are not supported");

    PLUSS_CHECK(u32_at(file, local) == 0x04034b50, "pluss::io::load_npz: a zip member is damaged");
    const std::size_t data_at = local + 30 + u16_at(file, local + 26) + u16_at(file, local + 28);
    PLUSS_CHECK(data_at + csize <= file.size(), "pluss::io::load_npz: a zip member is cut off");
    const std::string_view packed(file.data() + data_at, csize);

    std::string data;
    if (method == 0) {
      data.assign(packed);
    } else if (method == 8) {
      data = Inflater(packed, usize).run();
    } else {
      PLUSS_FAIL("pluss::io::load_npz: member '" + name + "' uses compression method " + std::to_string(method) + ", which is not supported");
    }
    PLUSS_CHECK(data.size() == usize && crc32(data) == crc, "pluss::io::load_npz: member '" + name + "' does not match its checksum");

    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".npy") == 0) {
      name.resize(name.size() - 4);
      out.emplace_back(name, series_from_npy(data, path.string() + ":" + name));
    }
  }
  return out;
}

}  // namespace pluss::io
