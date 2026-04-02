#pragma once

#include "pluss/export.h"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

#include "Flame/Flame.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/ChordImpl.h"

namespace pluss::coden {

namespace ops_internal { struct ChordAccess; }

// Lightweight handle (like TensorBase): cheap copy, refcounted, non-owning view.
class CODEN_API ChordBase {
public:
  using value_type = char;
  using size_type = ::std::size_t;

  class const_iterator {
  public:
    using iterator_category = ::std::forward_iterator_tag;
    using difference_type = ::std::ptrdiff_t;
    using value_type = char;
    using reference = char;
    using pointer = void;

    const_iterator() = default;

    reference operator*() const {
      PLUSS_CHECK(view_.data() != nullptr, "Chord iterator dereference on undefined Chord");
      PLUSS_CHECK(pos_ < view_.size(), "Chord iterator dereference out of range");
      return view_[pos_];
    }

    const_iterator& operator++() {
      ++pos_;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const const_iterator& other) const noexcept {
      return view_.data() == other.view_.data() &&
             view_.size() == other.view_.size() &&
             pos_ == other.pos_;
    }

    bool operator!=(const const_iterator& other) const noexcept {
      return !(*this == other);
    }

  private:
    friend class ChordBase;
    explicit const_iterator(::std::string_view view, size_type pos) : view_(view), pos_(pos) {}

    ::std::string_view view_{};
    size_type pos_ = 0;
  };

  ChordBase() = default;

  bool defined() const noexcept { return static_cast<bool>(impl_); }
  bool empty() const noexcept { return size_bytes() == 0; }
  size_type size() const noexcept { return size_bytes(); }
  ::std::size_t size_bytes() const noexcept { return impl_ ? impl_->size_bytes() : 0; }
  ::std::string_view utf8() const { return impl_ ? impl_->utf8() : ::std::string_view{}; }
  const char* data() const noexcept { return utf8().data(); }
  ::std::string as_string() const { return ::std::string(utf8()); }

  template <typename Fn>
    requires ::std::invocable<Fn&, char>
  void for_each_byte(Fn&& fn) const {
    PLUSS_CHECK(defined(), "for_each_byte on undefined Chord");
    for (char c : utf8()) {
      fn(c);
    }
  }

  const_iterator begin() const noexcept { return const_iterator{utf8(), 0}; }
  const_iterator end() const noexcept { return const_iterator{utf8(), utf8().size()}; }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

protected:
  friend struct ops_internal::ChordAccess;
  explicit ChordBase(::pluss::flame::intrusive_ptr<ChordImpl> impl)
      : impl_(::std::move(impl)) {}

  const ::pluss::flame::intrusive_ptr<ChordImpl>& impl() const noexcept { return impl_; }
  ::pluss::flame::intrusive_ptr<ChordImpl>& impl() noexcept { return impl_; }

private:
  ::pluss::flame::intrusive_ptr<ChordImpl> impl_;
};

} // namespace pluss::coden
