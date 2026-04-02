#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "Flame/Flame.h"
#include "CodeN/core/TextArena.h"

namespace pluss::coden {

// Note storage: arena-backed UTF-8 bytes + per-element (offset,len) + shape.
class NoteImpl final : public ::pluss::flame::RefCounted {
public:
  using OffLen = ::std::pair<::std::size_t, ::std::size_t>;

  enum class Encoding : ::std::uint8_t {
    Utf8Text = 0,
    Bytes = 1,
  };

  NoteImpl(::pluss::flame::intrusive_ptr<TextArena> arena,
           ::std::vector<OffLen> spans,
           ::std::array<::std::int64_t, 3> sizes,
           ::std::int64_t dim,
           Encoding enc = Encoding::Utf8Text)
      : arena_(::std::move(arena)), spans_(::std::move(spans)), sizes_(sizes), dim_(dim), enc_(enc) {}

  bool defined() const noexcept { return static_cast<bool>(arena_); }
  Encoding encoding() const noexcept { return enc_; }
  bool is_bytes() const noexcept { return enc_ == Encoding::Bytes; }
  ::std::int64_t dim() const noexcept { return dim_; }
  ::std::array<::std::int64_t, 3> sizes3() const noexcept { return sizes_; }
  ::std::span<const ::std::int64_t> sizes() const noexcept {
    return {sizes_.data(), static_cast<::std::size_t>(dim_)};
  }
  ::std::int64_t numel() const noexcept {
    ::std::int64_t n = 1;
    for (int i = 0; i < dim_; ++i) n *= sizes_[i];
    return n;
  }

  // Returns the raw stored bytes for element `linear`.
  // If encoding()==Utf8Text this is intended to be UTF-8 text, but it is not validated.
  ::std::string_view bytes_at(::std::int64_t linear) const {
    const auto [off, len] = spans_.at(static_cast<::std::size_t>(linear));
    return arena_ ? arena_->view(off, len) : ::std::string_view{};
  }

  const ::std::vector<OffLen>& spans() const noexcept { return spans_; }
  const ::pluss::flame::intrusive_ptr<TextArena>& arena() const noexcept { return arena_; }

private:
  ::pluss::flame::intrusive_ptr<TextArena> arena_;
  ::std::vector<OffLen> spans_;
  ::std::array<::std::int64_t, 3> sizes_{0,0,0};
  ::std::int64_t dim_ = 0;
  Encoding enc_ = Encoding::Utf8Text;
};

} // namespace pluss::coden
