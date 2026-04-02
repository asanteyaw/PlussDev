#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Flame/Flame.h"

namespace pluss::coden {

// Arena-backed storage for UTF-8 bytes.
//
// This is a foundational building block for cheap-copy text views (Chord) and
// batched text tensors (Note). Text elements are stored in a single contiguous
// byte buffer, referenced by (offset, length) pairs.
class TextArena final : public ::pluss::flame::RefCounted {
public:
  TextArena() = default;

  // Append bytes to the arena and return (offset, length).
  ::std::pair<::std::size_t, ::std::size_t> append(::std::string_view bytes) {
    const ::std::size_t off = data_.size();
    data_.insert(data_.end(), bytes.begin(), bytes.end());
    return {off, bytes.size()};
  }

  ::std::string_view view(::std::size_t off, ::std::size_t len) const {
    return ::std::string_view(data_.data() + off, len);
  }

  const char* data() const noexcept { return data_.data(); }
  ::std::size_t size() const noexcept { return data_.size(); }

private:
  ::std::vector<char> data_;
};

} // namespace pluss::coden
