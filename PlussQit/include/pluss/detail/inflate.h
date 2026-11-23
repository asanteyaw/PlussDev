#pragma once

// CRC-32 and DEFLATE decoding (RFC 1951), with nothing but the standard library: enough to read
// zip members and the pages of the bundled documentation. Errors are std::runtime_error.
//
// Both are written for speed, since they sit on the path of reading a large array file:
// the CRC works eight bytes at a time, and the decoder reads through a 64-bit bit buffer, decodes
// codes of up to ten bits with one table lookup, and copies back-references in bulk.

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pluss::detail {

// The CRC-32 of `bytes`; pass the CRC of what came before to continue over more data.
[[nodiscard]] inline std::uint32_t crc32(std::string_view bytes, std::uint32_t previous = 0) {
  static const auto tables = [] {
    std::array<std::array<std::uint32_t, 256>, 8> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
      std::uint32_t c = i;
      for (int k = 0; k < 8; ++k) c = (c & 1U) != 0 ? 0xEDB88320U ^ (c >> 1) : c >> 1;
      t[0][i] = c;
    }
    for (std::uint32_t i = 0; i < 256; ++i) {
      for (int slice = 1; slice < 8; ++slice) t[slice][i] = (t[slice - 1][i] >> 8) ^ t[0][t[slice - 1][i] & 0xFF];
    }
    return t;
  }();
  std::uint32_t c = previous ^ 0xFFFFFFFFU;
  const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
  std::size_t n = bytes.size();
  while (n >= 8) {
    std::uint32_t lo = 0, hi = 0;
    std::memcpy(&lo, p, 4);
    std::memcpy(&hi, p + 4, 4);
    lo ^= c;
    c = tables[7][lo & 0xFF] ^ tables[6][(lo >> 8) & 0xFF] ^ tables[5][(lo >> 16) & 0xFF] ^ tables[4][lo >> 24] ^
        tables[3][hi & 0xFF] ^ tables[2][(hi >> 8) & 0xFF] ^ tables[1][(hi >> 16) & 0xFF] ^ tables[0][hi >> 24];
    p += 8;
    n -= 8;
  }
  while (n-- > 0) c = tables[0][(c ^ *p++) & 0xFF] ^ (c >> 8);
  return c ^ 0xFFFFFFFFU;
}

class Inflater {
 public:
  // `expected` is the size of the decoded data, which a zip member and a bundled page both record.
  Inflater(std::string_view in, std::size_t expected) : in_(in), expected_(expected) {}

  std::string run() {
    out_.resize(expected_);
    bool last = false;
    while (!last) {
      last = take_(1) != 0;
      const unsigned type = take_(2);
      if (type == 0) {
        stored_();
      } else if (type == 1) {
        fixed_();
        block_();
      } else if (type == 2) {
        dynamic_();
        block_();
      } else {
        throw std::runtime_error("pluss inflate: a compressed member has a bad block type");
      }
    }
    if (written_ != expected_) throw std::runtime_error("pluss inflate: a compressed member is shorter than its recorded size");
    return std::move(out_);
  }

 private:
  static constexpr unsigned kFastBits = 10;

  // A prefix code: codes of up to kFastBits bits decode with one lookup; longer ones fall back to
  // walking the canonical code a bit at a time.
  struct Table {
    std::array<std::uint16_t, 16> count{};
    std::array<std::uint16_t, 288> symbol{};
    std::array<std::uint16_t, 1U << kFastBits> fast{};  // (symbol << 4) | length, 0 when longer than kFastBits
  };

  void refill_() {
    while (bit_count_ <= 56 && at_ < in_.size()) {
      bit_buffer_ |= static_cast<std::uint64_t>(static_cast<unsigned char>(in_[at_++])) << bit_count_;
      bit_count_ += 8;
    }
  }

  [[nodiscard]] unsigned take_(unsigned need) {
    if (need == 0) return 0;
    if (bit_count_ < need) {
      refill_();
      if (bit_count_ < need) throw std::runtime_error("pluss inflate: a compressed member is cut off");
    }
    const unsigned value = static_cast<unsigned>(bit_buffer_ & ((std::uint64_t{1} << need) - 1));
    bit_buffer_ >>= need;
    bit_count_ -= need;
    return value;
  }

  void stored_() {
    // Whole bytes still in the bit buffer go back to the input; the partial byte is dropped.
    const unsigned partial = bit_count_ % 8;
    bit_buffer_ >>= partial;
    bit_count_ -= partial;
    at_ -= bit_count_ / 8;
    bit_buffer_ = 0;
    bit_count_ = 0;
    if (at_ + 4 > in_.size()) throw std::runtime_error("pluss inflate: a stored block is cut off");
    const std::size_t length = static_cast<unsigned char>(in_[at_]) | (static_cast<std::size_t>(static_cast<unsigned char>(in_[at_ + 1])) << 8);
    at_ += 4;
    if (at_ + length > in_.size() || written_ + length > expected_) throw std::runtime_error("pluss inflate: a stored block is cut off");
    std::memcpy(out_.data() + written_, in_.data() + at_, length);
    written_ += length;
    at_ += length;
  }

  static void build_(Table& table, const std::uint8_t* lengths, std::size_t n) {
    table.count.fill(0);
    table.fast.fill(0);
    for (std::size_t i = 0; i < n; ++i) ++table.count[lengths[i]];
    table.count[0] = 0;
    std::array<std::uint16_t, 16> offsets{};
    for (int len = 1; len < 15; ++len) offsets[len + 1] = static_cast<std::uint16_t>(offsets[len] + table.count[len]);
    for (std::size_t i = 0; i < n; ++i) {
      if (lengths[i] != 0) table.symbol[offsets[lengths[i]]++] = static_cast<std::uint16_t>(i);
    }
    // The fast table: assign canonical codes, and for each code of at most kFastBits bits fill every
    // table slot whose low bits (deflate sends codes most significant bit first) spell it.
    unsigned code = 0;
    std::size_t index = 0;
    for (unsigned len = 1; len <= 15; ++len) {
      for (unsigned k = 0; k < table.count[len]; ++k, ++index) {
        const unsigned symbol = table.symbol[index];
        if (len <= kFastBits) {
          unsigned reversed = 0;
          for (unsigned b = 0; b < len; ++b) reversed |= ((code >> b) & 1U) << (len - 1 - b);
          for (unsigned fill = reversed; fill < (1U << kFastBits); fill += 1U << len) {
            table.fast[fill] = static_cast<std::uint16_t>((symbol << 4) | len);
          }
        }
        ++code;
      }
      code <<= 1;
    }
  }

  [[nodiscard]] unsigned decode_(const Table& table) {
    if (bit_count_ < 15) refill_();
    const std::uint16_t entry = table.fast[bit_buffer_ & ((1U << kFastBits) - 1)];
    if (entry != 0) {
      const unsigned len = entry & 15U;
      if (len > bit_count_) throw std::runtime_error("pluss inflate: a compressed member is cut off");
      bit_buffer_ >>= len;
      bit_count_ -= len;
      return entry >> 4;
    }
    // A code longer than kFastBits bits: walk the canonical code.
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; ++len) {
      if (bit_count_ == 0) throw std::runtime_error("pluss inflate: a compressed member is cut off");
      code |= static_cast<int>(bit_buffer_ & 1U);
      bit_buffer_ >>= 1;
      --bit_count_;
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
    const unsigned nlen = take_(5) + 257, ndist = take_(5) + 1, ncode = take_(4) + 4;
    if (!(nlen <= 286 && ndist <= 30)) throw std::runtime_error("pluss inflate: a compressed member has bad table sizes");
    std::uint8_t lengths[320] = {};
    for (unsigned i = 0; i < ncode; ++i) lengths[order[i]] = static_cast<std::uint8_t>(take_(3));
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
          repeat = 3 + take_(2);
        } else if (symbol == 17) {
          repeat = 3 + take_(3);
        } else {
          repeat = 11 + take_(7);
        }
        if (!(index + repeat <= nlen + ndist)) throw std::runtime_error("pluss inflate: a compressed member repeats too far");
        while (repeat-- > 0) all[index++] = previous;
      }
    }
    build_(literal_, all, nlen);
    build_(distance_, all + nlen, ndist);
  }

  void block_() {
    static constexpr std::array<std::uint16_t, 29> base{3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static constexpr std::array<std::uint8_t, 29> extra{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static constexpr std::array<std::uint16_t, 30> dbase{1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static constexpr std::array<std::uint8_t, 30> dextra{0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    char* const out = out_.data();
    for (;;) {
      unsigned symbol = decode_(literal_);
      if (symbol < 256) {
        if (written_ >= expected_) throw std::runtime_error("pluss inflate: a compressed member is longer than its recorded size");
        out[written_++] = static_cast<char>(symbol);
      } else if (symbol == 256) {
        return;
      } else {
        symbol -= 257;
        if (!(symbol < 29)) throw std::runtime_error("pluss inflate: a compressed member has a bad length code");
        const unsigned length = base[symbol] + take_(extra[symbol]);
        const unsigned ds = decode_(distance_);
        if (!(ds < 30)) throw std::runtime_error("pluss inflate: a compressed member has a bad distance code");
        const std::size_t dist = dbase[ds] + take_(dextra[ds]);
        if (dist > written_) throw std::runtime_error("pluss inflate: a compressed member points before its start");
        if (written_ + length > expected_) throw std::runtime_error("pluss inflate: a compressed member is longer than its recorded size");
        char* to = out + written_;
        const char* from = to - dist;
        if (dist >= length) {
          std::memcpy(to, from, length);  // the pieces do not overlap
        } else if (dist == 1) {
          std::memset(to, static_cast<unsigned char>(*from), length);
        } else {
          for (unsigned i = 0; i < length; ++i) to[i] = from[i];  // an overlapping copy repeats the pattern
        }
        written_ += length;
      }
    }
  }

  std::string_view in_;
  std::size_t expected_;
  std::string out_;
  std::size_t written_{0};
  std::size_t at_{0};
  std::uint64_t bit_buffer_{0};
  unsigned bit_count_{0};
  Table literal_{};
  Table distance_{};
};

}  // namespace pluss::detail
