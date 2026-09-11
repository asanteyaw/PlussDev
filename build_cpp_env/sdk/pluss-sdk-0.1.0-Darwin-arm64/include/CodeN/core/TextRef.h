#pragma once

// TextRef: an owning reference to a single text cell.
//
// A TextRef co-owns the backing TextArena (one atomic refcount bump on copy)
// and points at an (offset, length) slice of it. Unlike the std::string_view
// returned by Note::at(), a TextRef stays valid after the parent Note is
// destroyed, so it is the right type to store or return past the Note's scope.
//
// It converts implicitly to std::string_view, so anywhere a borrowed view is
// accepted a TextRef works too; call .str() to materialize an owned copy.

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "Flame/Flame.h"
#include "CodeN/core/TextArena.h"

namespace pluss::coden {

class TextRef {
public:
  using value_type = char;
  using size_type = ::std::size_t;

  TextRef() = default;

  TextRef(::pluss::flame::intrusive_ptr<TextArena> arena, size_type off, size_type len) noexcept
      : arena_(::std::move(arena)), off_(off), len_(len) {}

  bool defined() const noexcept { return static_cast<bool>(arena_); }
  bool empty() const noexcept { return len_ == 0; }
  size_type size() const noexcept { return len_; }
  size_type size_bytes() const noexcept { return len_; }

  const char* data() const noexcept { return arena_ ? arena_->data() + off_ : nullptr; }

  ::std::string_view view() const noexcept {
    return arena_ ? arena_->view(off_, len_) : ::std::string_view{};
  }

  // Implicit: a TextRef is usable anywhere a std::string_view is expected.
  operator ::std::string_view() const noexcept { return view(); }

  // Materialize an owned copy.
  ::std::string str() const { return ::std::string(view()); }

  char operator[](size_type i) const noexcept { return view()[i]; }

  const char* begin() const noexcept { return data(); }
  const char* end() const noexcept { return data() + len_; }

  friend bool operator==(const TextRef& a, ::std::string_view b) noexcept { return a.view() == b; }
  friend bool operator==(::std::string_view a, const TextRef& b) noexcept { return a == b.view(); }
  friend bool operator==(const TextRef& a, const TextRef& b) noexcept { return a.view() == b.view(); }
  friend bool operator!=(const TextRef& a, ::std::string_view b) noexcept { return a.view() != b; }
  friend bool operator!=(::std::string_view a, const TextRef& b) noexcept { return a != b.view(); }
  friend bool operator!=(const TextRef& a, const TextRef& b) noexcept { return a.view() != b.view(); }
  friend bool operator<(const TextRef& a, const TextRef& b) noexcept { return a.view() < b.view(); }
  friend bool operator<(const TextRef& a, ::std::string_view b) noexcept { return a.view() < b; }
  friend bool operator<(::std::string_view a, const TextRef& b) noexcept { return a < b.view(); }

  friend ::std::ostream& operator<<(::std::ostream& os, const TextRef& r) { return os << r.view(); }

private:
  ::pluss::flame::intrusive_ptr<TextArena> arena_;
  size_type off_ = 0;
  size_type len_ = 0;
};

}  // namespace pluss::coden
