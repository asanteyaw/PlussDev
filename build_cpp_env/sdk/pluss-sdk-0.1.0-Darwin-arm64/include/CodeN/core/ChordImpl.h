#pragma once

#include <cstddef>
#include <string_view>

#include "Flame/Flame.h"
#include "CodeN/core/TextArena.h"

namespace pluss::coden {

class ChordImpl final : public ::pluss::flame::RefCounted {
public:
  ChordImpl(::pluss::flame::intrusive_ptr<TextArena> arena,
            ::std::size_t off,
            ::std::size_t len)
      : arena_(::std::move(arena)), off_(off), len_(len) {}

  ::std::string_view utf8() const {
    return arena_ ? arena_->view(off_, len_) : ::std::string_view{};
  }

  ::std::size_t size_bytes() const noexcept { return len_; }
  bool defined() const noexcept { return static_cast<bool>(arena_); }

  const ::pluss::flame::intrusive_ptr<TextArena>& arena() const noexcept { return arena_; }
  ::std::size_t offset() const noexcept { return off_; }
  ::std::size_t length() const noexcept { return len_; }

private:
  ::pluss::flame::intrusive_ptr<TextArena> arena_;
  ::std::size_t off_ = 0;
  ::std::size_t len_ = 0;
};

} // namespace pluss::coden
