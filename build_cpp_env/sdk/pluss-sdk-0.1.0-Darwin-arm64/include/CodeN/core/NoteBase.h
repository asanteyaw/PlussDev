#pragma once

#include "pluss/export.h"

#include <concepts>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Flame/Flame.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteImpl.h"

namespace pluss::coden {

namespace ops_internal { struct NoteAccess; }

class CODEN_API NoteBase {
public:
  using value_type = ::std::string_view;
  using index_type = ::std::int64_t;

  struct HostArray {
    ::std::vector<::std::string> data;
    ::std::vector<index_type> sizes;
    bool contiguous{true};

    ::std::span<const index_type> shape() const noexcept { return sizes; }
    index_type dim() const noexcept { return static_cast<index_type>(sizes.size()); }
    index_type numel() const noexcept { return static_cast<index_type>(data.size()); }
  };

  class HostAny final {
  public:
    enum class Kind : ::std::uint8_t { Scalar = 0, Vector = 1, Array = 2 };

    HostAny() = default;

    static HostAny from_scalar(::std::string value) {
      HostAny out;
      out.kind_ = Kind::Scalar;
      out.payload_ = ::std::move(value);
      return out;
    }

    static HostAny from_vector(::std::vector<::std::string> values) {
      HostAny out;
      out.kind_ = Kind::Vector;
      out.payload_ = ::std::move(values);
      return out;
    }

    static HostAny from_array(HostArray values) {
      HostAny out;
      out.kind_ = Kind::Array;
      out.payload_ = ::std::move(values);
      return out;
    }

    Kind kind() const noexcept { return kind_; }
    bool is_scalar() const noexcept { return kind_ == Kind::Scalar; }
    bool is_vector() const noexcept { return kind_ == Kind::Vector; }
    bool is_array() const noexcept { return kind_ == Kind::Array; }

    const ::std::string& scalar() const {
      PLUSS_CHECK(is_scalar(), "Note::host(): value is not scalar");
      return ::std::get<::std::string>(payload_);
    }

    const ::std::vector<::std::string>& vector() const {
      PLUSS_CHECK(is_vector(), "Note::host(): value is not a 1-D vector");
      return ::std::get<::std::vector<::std::string>>(payload_);
    }

    const HostArray& array() const {
      PLUSS_CHECK(is_array(), "Note::host(): value is not N-D array");
      return ::std::get<HostArray>(payload_);
    }

    friend ::std::ostream& operator<<(::std::ostream& os, const HostAny& value) {
      switch (value.kind_) {
        case Kind::Scalar:
          os << value.scalar();
          break;
        case Kind::Vector:
          os << "[";
          for (::std::size_t i = 0; i < value.vector().size(); ++i) {
            if (i) os << ", ";
            os << value.vector()[i];
          }
          os << "]";
          break;
        case Kind::Array:
          os << "Note::HostArray(shape=[";
          for (::std::size_t i = 0; i < value.array().sizes.size(); ++i) {
            if (i) os << ", ";
            os << value.array().sizes[i];
          }
          os << "], values=" << value.array().data.size() << ")";
          break;
      }
      return os;
    }

  private:
    Kind kind_{Kind::Scalar};
    ::std::variant<::std::string, ::std::vector<::std::string>, HostArray> payload_{::std::string{}};
  };

  class const_iterator {
  public:
    using iterator_category = ::std::forward_iterator_tag;
    using difference_type = ::std::ptrdiff_t;
    using value_type = ::std::string_view;
    using reference = ::std::string_view;
    using pointer = void;

    const_iterator() = default;

    reference operator*() const {
      PLUSS_CHECK(impl_, "Note iterator dereference on undefined Note");
      PLUSS_CHECK(index_ >= 0 && index_ < impl_->numel(), "Note iterator dereference out of range");
      return impl_->bytes_at(index_);
    }

    const_iterator& operator++() {
      ++index_;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const const_iterator& other) const noexcept {
      return impl_.get() == other.impl_.get() && index_ == other.index_;
    }

    bool operator!=(const const_iterator& other) const noexcept {
      return !(*this == other);
    }

  private:
    friend class NoteBase;
    explicit const_iterator(::pluss::flame::intrusive_ptr<NoteImpl> impl, index_type index)
        : impl_(::std::move(impl)), index_(index) {}

    ::pluss::flame::intrusive_ptr<NoteImpl> impl_;
    index_type index_ = 0;
  };

  NoteBase() = default;

  bool defined() const noexcept { return static_cast<bool>(impl_); }
  ::std::int64_t dim() const noexcept { return impl_ ? impl_->dim() : 0; }
  ::std::int64_t ndim() const noexcept { return dim(); }
  ::std::array<::std::int64_t,3> sizes3() const noexcept { return impl_ ? impl_->sizes3() : ::std::array<::std::int64_t,3>{0,0,0}; }
  ::std::int64_t size(::std::int64_t d) const noexcept {
    const auto s = sizes3();
    return (d>=0 && d<3) ? s[static_cast<::std::size_t>(d)] : 0;
  }
  ::std::int64_t numel() const noexcept { return impl_ ? impl_->numel() : 0; }
  bool empty() const noexcept { return numel() == 0; }
  bool is_scalar() const noexcept { return dim() == 0; }
  bool is_vector() const noexcept { return dim() == 1; }
  bool is_matrix() const noexcept { return dim() == 2; }
  bool is_tensor3() const noexcept { return dim() == 3; }

  // Encoding / storage intent.
  // - false: intended UTF-8 text (not validated)
  // - true : raw bytes (no UTF-8 guarantees)
  bool is_bytes() const noexcept { return impl_ ? impl_->is_bytes() : false; }

  ::std::vector<index_type> shape() const;
  value_type flat_at(index_type linear) const;
  index_type linear_index(::std::span<const index_type> indices) const;
  value_type at_indices(::std::span<const index_type> indices) const;

  // Legacy-style convenience projections.
  value_type scalar() const;
  ::std::vector<value_type> vector() const;
  ::std::vector<::std::vector<value_type>> matrix() const;
  ::std::vector<::std::vector<::std::vector<value_type>>> tensor3() const;

  HostAny host() const {
    PLUSS_CHECK(defined(), "Note::host(): undefined");

    if (is_scalar()) {
      return HostAny::from_scalar(::std::string(scalar()));
    }

    ::std::vector<::std::string> values;
    values.reserve(static_cast<::std::size_t>(numel()));
    for (index_type i = 0; i < numel(); ++i) {
      values.emplace_back(flat_at(i));
    }

    if (is_vector()) {
      return HostAny::from_vector(::std::move(values));
    }

    HostArray array;
    array.data = ::std::move(values);
    array.sizes = shape();
    array.contiguous = true;
    return HostAny::from_array(::std::move(array));
  }

  ::std::string join_flat(::std::string_view sep = " ") const;
  ::std::string debug(index_type max_items = 12) const;

  template <::std::output_iterator<value_type> OutIt>
  void copy_flat_to(OutIt out) const {
    PLUSS_CHECK(defined(), "copy_flat_to on undefined Note");
    for (index_type i = 0; i < numel(); ++i) {
      *out = impl_->bytes_at(i);
      ++out;
    }
  }

  template <typename Fn>
    requires ::std::invocable<Fn&, value_type>
  void for_each_flat(Fn&& fn) const {
    PLUSS_CHECK(defined(), "for_each_flat on undefined Note");
    for (index_type i = 0; i < numel(); ++i) {
      fn(impl_->bytes_at(i));
    }
  }

  const_iterator begin() const noexcept {
    if (!impl_) return const_iterator{};
    return const_iterator{impl_, 0};
  }

  const_iterator end() const noexcept {
    if (!impl_) return const_iterator{};
    return const_iterator{impl_, impl_->numel()};
  }

  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

protected:
  friend struct ops_internal::NoteAccess;
  explicit NoteBase(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : impl_(::std::move(impl)) {}

  const ::pluss::flame::intrusive_ptr<NoteImpl>& impl() const noexcept { return impl_; }
  ::pluss::flame::intrusive_ptr<NoteImpl>& impl() noexcept { return impl_; }

private:
  ::pluss::flame::intrusive_ptr<NoteImpl> impl_;
};

} // namespace pluss::coden
