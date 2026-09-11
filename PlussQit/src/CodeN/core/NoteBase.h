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
#include <type_traits>
#include <variant>
#include <vector>

#include "Flame/Flame.h"
#include "CodeN/core/Errors.h"
#include "CodeN/core/NoteImpl.h"

namespace pluss::coden {

class Note;
namespace ops_internal {
struct NoteAccess;
Note note_contiguous_view(const Note &source, ::std::int64_t offset,
                          ::std::span<const ::std::int64_t> sizes);
}

class CODEN_API NoteBase {
public:
  using value_type = ::std::string_view;
  using index_type = ::std::int64_t;

  struct HostArray {
    // `data` remains the source-compatible Text/Bytes host field. Native
    // dtypes use their correspondingly named field and never stringify.
    ::std::vector<::std::string> data;
    ::std::vector<::std::uint8_t> bool_data;
    ::std::vector<::std::int64_t> int_data;
    ::std::vector<double> float_data;
    ::std::vector<index_type> sizes;
    NoteDType dtype{NoteDType::Text};
    bool contiguous{true};

    ::std::span<const index_type> shape() const noexcept { return sizes; }
    index_type dim() const noexcept { return static_cast<index_type>(sizes.size()); }
    index_type numel() const noexcept {
      if (dtype == NoteDType::Bool) return static_cast<index_type>(bool_data.size());
      if (dtype == NoteDType::Int64) return static_cast<index_type>(int_data.size());
      if (dtype == NoteDType::Float64) return static_cast<index_type>(float_data.size());
      return static_cast<index_type>(data.size());
    }
  };

  class HostAny final {
  public:
    enum class Kind : ::std::uint8_t { Scalar = 0, Vector = 1, Array = 2 };

    HostAny() = default;

    template <class T>
    static HostAny from_scalar(T value, NoteDType dtype) {
      HostAny out;
      out.kind_ = Kind::Scalar;
      out.payload_ = ::std::move(value);
      out.dtype_ = dtype;
      return out;
    }

    template <class T>
    static HostAny from_vector(::std::vector<T> values, NoteDType dtype) {
      HostAny out;
      out.kind_ = Kind::Vector;
      out.payload_ = ::std::move(values);
      out.dtype_ = dtype;
      return out;
    }

    static HostAny from_array(HostArray values) {
      HostAny out;
      out.kind_ = Kind::Array;
      out.dtype_ = values.dtype;
      out.payload_ = ::std::move(values);
      return out;
    }

    Kind kind() const noexcept { return kind_; }
    bool is_scalar() const noexcept { return kind_ == Kind::Scalar; }
    bool is_vector() const noexcept { return kind_ == Kind::Vector; }
    bool is_array() const noexcept { return kind_ == Kind::Array; }
    NoteDType dtype() const noexcept { return dtype_; }

    const ::std::string& scalar() const {
      PLUSS_CHECK(is_scalar(), "Note::host(): value is not scalar");
      return ::std::get<::std::string>(payload_);
    }

    const ::std::vector<::std::string>& vector() const {
      PLUSS_CHECK(is_vector(), "Note::host(): value is not a 1-D vector");
      return ::std::get<::std::vector<::std::string>>(payload_);
    }

    bool bool_scalar() const {
      PLUSS_CHECK(is_scalar() && dtype_ == NoteDType::Bool,
                  "Note::host(): value is not a Bool scalar");
      return ::std::get<bool>(payload_);
    }
    ::std::int64_t int_scalar() const {
      PLUSS_CHECK(is_scalar() && dtype_ == NoteDType::Int64,
                  "Note::host(): value is not an Int64 scalar");
      return ::std::get<::std::int64_t>(payload_);
    }
    double float_scalar() const {
      PLUSS_CHECK(is_scalar() && dtype_ == NoteDType::Float64,
                  "Note::host(): value is not a Float64 scalar");
      return ::std::get<double>(payload_);
    }
    const ::std::vector<::std::uint8_t>& bool_vector() const {
      PLUSS_CHECK(is_vector() && dtype_ == NoteDType::Bool,
                  "Note::host(): value is not a Bool vector");
      return ::std::get<::std::vector<::std::uint8_t>>(payload_);
    }
    const ::std::vector<::std::int64_t>& int_vector() const {
      PLUSS_CHECK(is_vector() && dtype_ == NoteDType::Int64,
                  "Note::host(): value is not an Int64 vector");
      return ::std::get<::std::vector<::std::int64_t>>(payload_);
    }
    const ::std::vector<double>& float_vector() const {
      PLUSS_CHECK(is_vector() && dtype_ == NoteDType::Float64,
                  "Note::host(): value is not a Float64 vector");
      return ::std::get<::std::vector<double>>(payload_);
    }

    const HostArray& array() const {
      PLUSS_CHECK(is_array(), "Note::host(): value is not N-D array");
      return ::std::get<HostArray>(payload_);
    }

    friend ::std::ostream& operator<<(::std::ostream& os, const HostAny& value) {
      switch (value.kind_) {
        case Kind::Scalar:
          if (value.dtype_ == NoteDType::Text || value.dtype_ == NoteDType::Bytes) os << value.scalar();
          else if (value.dtype_ == NoteDType::Bool) os << (value.bool_scalar() ? "true" : "false");
          else if (value.dtype_ == NoteDType::Int64) os << value.int_scalar();
          else os << value.float_scalar();
          break;
        case Kind::Vector:
          os << "[";
          ::std::visit([&](const auto &values) {
            using Values = ::std::remove_cvref_t<decltype(values)>;
            if constexpr (!::std::same_as<Values, HostArray> &&
                          requires { values.size(); values[0]; }) {
              for (::std::size_t i = 0; i < values.size(); ++i) {
                if (i) os << ", ";
                if constexpr (::std::same_as<typename Values::value_type, ::std::uint8_t>)
                  os << (values[i] ? "true" : "false");
                else os << values[i];
              }
            }
          }, value.payload_);
          os << "]";
          break;
        case Kind::Array:
          os << "Note::HostArray(shape=[";
          for (::std::size_t i = 0; i < value.array().sizes.size(); ++i) {
            if (i) os << ", ";
            os << value.array().sizes[i];
          }
          os << "], values=" << value.array().numel() << ")";
          break;
      }
      return os;
    }

  private:
    Kind kind_{Kind::Scalar};
    NoteDType dtype_{NoteDType::Text};
    ::std::variant<::std::string, bool, ::std::int64_t, double,
                   ::std::vector<::std::string>, ::std::vector<::std::uint8_t>,
                   ::std::vector<::std::int64_t>, ::std::vector<double>, HostArray>
        payload_{::std::string{}};
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
  ::std::int64_t size(::std::int64_t d) const {
    PLUSS_CHECK(defined(), "Note::size(): undefined Note");
    if (d < 0) d += dim();
    PLUSS_CHECK(d >= 0 && d < dim(), "Note::size(): dimension out of range");
    return impl_->sizes3()[static_cast<::std::size_t>(d)];
  }
  ::std::int64_t numel() const noexcept { return impl_ ? impl_->numel() : 0; }
  bool empty() const noexcept { return numel() == 0; }
  bool is_scalar() const noexcept { return defined() && dim() == 0; }
  bool is_vector() const noexcept { return defined() && dim() == 1; }
  bool is_matrix() const noexcept { return defined() && dim() == 2; }
  bool is_tensor3() const noexcept { return defined() && dim() == 3; }

  NoteDType dtype() const {
    PLUSS_CHECK(defined(), "Note::dtype(): undefined Note");
    return impl_->dtype();
  }
  bool is_text() const noexcept { return impl_ && impl_->is_text(); }
  bool is_bytes() const noexcept { return impl_ ? impl_->is_bytes() : false; }
  bool is_bool() const noexcept { return impl_ && impl_->is_bool(); }
  bool is_int64() const noexcept { return impl_ && impl_->is_int64(); }
  bool is_float64() const noexcept { return impl_ && impl_->is_float64(); }

  value_type at_indices(::std::span<const index_type> indices) const;

  // Borrowed standard-container projections over the packed cell arena.
  // Text/Bytes only by design (see bool_item()/int_item()/float_item() and
  // bools()/integers()/reals() for the equivalent typed access on other
  // dtypes -- a single variant-returning accessor here would just force
  // std::visit/std::get boilerplate at every call site for no extra
  // capability those already provide).
  value_type scalar() const;
  ::std::vector<value_type> vector() const;
  ::std::vector<Note> matrix() const;
  ::std::vector<Note> tensor3() const;

  bool bool_item() const;
  ::std::int64_t int_item() const;
  double float_item() const;
  ::std::span<const ::std::uint8_t> bools() const;
  ::std::span<const ::std::int64_t> integers() const;
  ::std::span<const double> reals() const;
  HostAny host() const;

  ::std::string join_flat(::std::string_view sep = " ") const;
  ::std::string debug(index_type max_items = 12) const;

  // copy_flat_to/for_each_flat/begin/end (and cbegin/cend) all traverse the
  // packed cell arena directly and so are Text/Bytes only, same as scalar()
  // above -- use bools()/integers()/reals() (plain std::span<const T>,
  // already range-for- and <algorithm>-compatible) to traverse a Bool,
  // Int64, or Float64 Note instead.
  template <::std::output_iterator<value_type> OutIt>
  void copy_flat_to(OutIt out) const {
    PLUSS_CHECK(defined(), "copy_flat_to on undefined Note");
    PLUSS_CHECK(is_text() || is_bytes(),
                "copy_flat_to() requires Text or Bytes storage; use bools()/integers()/reals() for other dtypes");
    for (index_type i = 0; i < numel(); ++i) {
      *out = impl_->bytes_at(i);
      ++out;
    }
  }

  template <typename Fn>
    requires ::std::invocable<Fn&, value_type>
  void for_each_flat(Fn&& fn) const {
    PLUSS_CHECK(defined(), "for_each_flat on undefined Note");
    PLUSS_CHECK(is_text() || is_bytes(),
                "for_each_flat() requires Text or Bytes storage; use bools()/integers()/reals() for other dtypes");
    for (index_type i = 0; i < numel(); ++i) {
      fn(impl_->bytes_at(i));
    }
  }

  const_iterator begin() const {
    if (!impl_) return const_iterator{};
    PLUSS_CHECK(is_text() || is_bytes(),
                "Note::begin(): iteration requires Text or Bytes storage; use bools()/integers()/reals() for other dtypes");
    return const_iterator{impl_, 0};
  }

  const_iterator end() const {
    if (!impl_) return const_iterator{};
    PLUSS_CHECK(is_text() || is_bytes(),
                "Note::end(): iteration requires Text or Bytes storage; use bools()/integers()/reals() for other dtypes");
    return const_iterator{impl_, impl_->numel()};
  }

  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

protected:
  friend struct ops_internal::NoteAccess;
  explicit NoteBase(::pluss::flame::intrusive_ptr<NoteImpl> impl)
      : impl_(::std::move(impl)) {}

  const ::pluss::flame::intrusive_ptr<NoteImpl>& impl() const noexcept { return impl_; }
  ::pluss::flame::intrusive_ptr<NoteImpl>& impl() noexcept { return impl_; }
  ::std::array<::std::int64_t, 3> sizes3() const noexcept {
    return impl_ ? impl_->sizes3() : ::std::array<::std::int64_t, 3>{0, 0, 0};
  }
  value_type flat_at(index_type linear) const;
  index_type linear_index(::std::span<const index_type> indices) const;

private:
  ::pluss::flame::intrusive_ptr<NoteImpl> impl_;
};

} // namespace pluss::coden
