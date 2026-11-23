#pragma once

// CRC-32 and DEFLATE decoding (RFC 1951), with nothing but the standard library: enough to read
// zip members and the pages of the bundled documentation. Errors are std::runtime_error.

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pluss::detail {

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
        throw std::runtime_error("pluss inflate: a compressed member has a bad block type");
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
        if (!(at_ < in_.size())) throw std::runtime_error("pluss inflate: a compressed member is cut off");
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
    if (!(at_ + 4 <= in_.size())) throw std::runtime_error("pluss inflate: a stored block is cut off");
    const unsigned length = static_cast<unsigned char>(in_[at_]) | (static_cast<unsigned>(static_cast<unsigned char>(in_[at_ + 1])) << 8);
    at_ += 4;
    if (!(at_ + length <= in_.size())) throw std::runtime_error("pluss inflate: a stored block is cut off");
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
    throw std::runtime_error("pluss inflate: a compressed member has an invalid code");
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
    if (!(nlen <= 286 && ndist <= 30)) throw std::runtime_error("pluss inflate: a compressed member has bad table sizes");
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
          if (!(index > 0)) throw std::runtime_error("pluss inflate: a compressed member repeats nothing");
          previous = all[index - 1];
          repeat = 3 + bits_(2);
        } else if (symbol == 17) {
          repeat = 3 + bits_(3);
        } else {
          repeat = 11 + bits_(7);
        }
        if (!(index + repeat <= nlen + ndist)) throw std::runtime_error("pluss inflate: a compressed member repeats too far");
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
        if (!(symbol < 29)) throw std::runtime_error("pluss inflate: a compressed member has a bad length code");
        const unsigned length = base[symbol] + bits_(extra[symbol]);
        const unsigned ds = decode_(distance);
        if (!(ds < 30)) throw std::runtime_error("pluss inflate: a compressed member has a bad distance code");
        const unsigned dist = dbase[ds] + bits_(dextra[ds]);
        if (!(dist <= out_.size())) throw std::runtime_error("pluss inflate: a compressed member points before its start");
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

}  // namespace pluss::detail
