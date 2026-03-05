#include "CodeN/core/Note.h"

#include <algorithm>
#include <sstream>
#include <utility>

#include "CodeN/core/OpsInternal.h"

namespace pluss::coden {

namespace {

static ::std::string escape_for_debug(::std::string_view s) {
  ::std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c >= 0x20 && c != 0x7F) {
          out.push_back(static_cast<char>(c));
        } else {
          out += "\\x";
          static constexpr char kHex[] = "0123456789ABCDEF";
          out.push_back(kHex[(c >> 4) & 0x0F]);
          out.push_back(kHex[c & 0x0F]);
        }
        break;
    }
  }
  return out;
}

enum class NoteCmpOp : ::std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

::std::vector<::std::int64_t> note_shape(const Note& n) {
  const auto s = n.sizes3();
  ::std::vector<::std::int64_t> out;
  out.reserve(static_cast<::std::size_t>(n.dim()));
  for (::std::int64_t d = 0; d < n.dim(); ++d) {
    out.push_back(s[static_cast<::std::size_t>(d)]);
  }
  return out;
}

void validate_note_pair_for_text_ops(const Note& lhs, const Note& rhs, ::std::string_view where) {
  PLUSS_CHECK(lhs.defined() && rhs.defined(), ::std::string(where) + ": operands must be defined");
  const bool lhs_bytes = static_cast<const NoteBase&>(lhs).is_bytes();
  const bool rhs_bytes = static_cast<const NoteBase&>(rhs).is_bytes();
  PLUSS_CHECK(lhs_bytes == rhs_bytes, ::std::string(where) + ": mixed bytes/utf8 operands are not supported");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  PLUSS_CHECK(nx == ny || bx || by, ::std::string(where) + ": expected equal numel or scalar broadcast");
}

Note note_concat_pair(const Note& lhs, const Note& rhs) {
  validate_note_pair_for_text_ops(lhs, rhs, "Note::operator+");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  const auto n = (nx == ny) ? nx : (bx ? ny : nx);

  ::std::vector<::std::string> out;
  out.reserve(static_cast<::std::size_t>(n));
  for (::std::int64_t i = 0; i < n; ++i) {
    const auto x = lhs.at(bx ? 0 : i);
    const auto y = rhs.at(by ? 0 : i);
    ::std::string z;
    z.reserve(x.size() + y.size());
    z.append(x.data(), x.size());
    z.append(y.data(), y.size());
    out.push_back(::std::move(z));
  }

  ::std::vector<::std::string_view> flat;
  flat.reserve(out.size());
  for (const auto& v : out) flat.push_back(v);

  const Note& shape_src = (nx == ny) ? lhs : (bx ? rhs : lhs);
  const auto shape = note_shape(shape_src);
  if (static_cast<const NoteBase&>(lhs).is_bytes()) {
    return ops_internal::note_from_bytes(
        ::std::span<const ::std::string_view>(flat.data(), flat.size()),
        ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
  }
  return ops_internal::note_from_utf8(
      ::std::span<const ::std::string_view>(flat.data(), flat.size()),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

Note note_repeat_scalar(const Note& n, ::std::int64_t times) {
  PLUSS_CHECK(n.defined(), "Note::operator*: operand must be defined");
  PLUSS_CHECK(times >= 0, "Note::operator*: repeat count must be >= 0");
  ::std::vector<::std::string> out;
  out.reserve(static_cast<::std::size_t>(n.numel()));
  for (::std::int64_t i = 0; i < n.numel(); ++i) {
    const auto s = n.at(i);
    ::std::string z;
    const auto reserve = static_cast<::std::size_t>(times) * s.size();
    z.reserve(reserve);
    for (::std::int64_t k = 0; k < times; ++k) {
      z.append(s.data(), s.size());
    }
    out.push_back(::std::move(z));
  }
  ::std::vector<::std::string_view> flat;
  flat.reserve(out.size());
  for (const auto& v : out) flat.push_back(v);
  const auto shape = note_shape(n);
  if (static_cast<const NoteBase&>(n).is_bytes()) {
    return ops_internal::note_from_bytes(
        ::std::span<const ::std::string_view>(flat.data(), flat.size()),
        ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
  }
  return ops_internal::note_from_utf8(
      ::std::span<const ::std::string_view>(flat.data(), flat.size()),
      ::std::span<const ::std::int64_t>(shape.data(), shape.size()));
}

bool note_compare_strings(::std::string_view lhs, ::std::string_view rhs, NoteCmpOp op) {
  switch (op) {
    case NoteCmpOp::Eq:
      return lhs == rhs;
    case NoteCmpOp::Ne:
      return lhs != rhs;
    case NoteCmpOp::Lt:
      return lhs < rhs;
    case NoteCmpOp::Le:
      return lhs <= rhs;
    case NoteCmpOp::Gt:
      return lhs > rhs;
    case NoteCmpOp::Ge:
      return lhs >= rhs;
    default:
      return false;
  }
}

::std::vector<::std::uint8_t> note_compare_pair(const Note& lhs, const Note& rhs, NoteCmpOp op) {
  validate_note_pair_for_text_ops(lhs, rhs, "Note::compare");
  const auto nx = lhs.numel();
  const auto ny = rhs.numel();
  const bool bx = (nx == 1);
  const bool by = (ny == 1);
  const auto n = (nx == ny) ? nx : (bx ? ny : nx);
  ::std::vector<::std::uint8_t> out(static_cast<::std::size_t>(n), 0);
  for (::std::int64_t i = 0; i < n; ++i) {
    out[static_cast<::std::size_t>(i)] =
        static_cast<::std::uint8_t>(note_compare_strings(lhs.at(bx ? 0 : i), rhs.at(by ? 0 : i), op));
  }
  return out;
}

::std::vector<::std::uint8_t> note_compare_scalar(const Note& lhs, ::std::string_view rhs, NoteCmpOp op) {
  PLUSS_CHECK(lhs.defined(), "Note::compare: lhs must be defined");
  ::std::vector<::std::uint8_t> out(static_cast<::std::size_t>(lhs.numel()), 0);
  for (::std::int64_t i = 0; i < lhs.numel(); ++i) {
    out[static_cast<::std::size_t>(i)] =
        static_cast<::std::uint8_t>(note_compare_strings(lhs.at(i), rhs, op));
  }
  return out;
}

} // namespace

::std::vector<NoteBase::index_type> NoteBase::shape() const {
  PLUSS_CHECK(defined(), "shape() on undefined Note");
  ::std::vector<index_type> out;
  const auto d = dim();
  out.reserve(static_cast<::std::size_t>(d));
  const auto s = sizes3();
  for (index_type i = 0; i < d; ++i) {
    out.push_back(s[static_cast<::std::size_t>(i)]);
  }
  return out;
}

NoteBase::value_type NoteBase::flat_at(index_type linear) const {
  PLUSS_CHECK(defined(), "flat_at() on undefined Note");
  PLUSS_CHECK(linear >= 0 && linear < numel(), "flat_at() index out of range");
  return impl_->bytes_at(linear);
}

NoteBase::index_type NoteBase::linear_index(::std::span<const index_type> indices) const {
  PLUSS_CHECK(defined(), "linear_index() on undefined Note");
  PLUSS_CHECK(indices.size() == static_cast<::std::size_t>(dim()), "linear_index() rank mismatch");

  if (indices.empty()) return 0;

  const auto s = sizes3();
  index_type linear = 0;
  for (index_type d = 0; d < dim(); ++d) {
    const auto idx = indices[static_cast<::std::size_t>(d)];
    const auto size_d = s[static_cast<::std::size_t>(d)];
    PLUSS_CHECK(idx >= 0 && idx < size_d, "linear_index() index out of bounds");
    linear = linear * size_d + idx;
  }
  return linear;
}

NoteBase::value_type NoteBase::at_indices(::std::span<const index_type> indices) const {
  return flat_at(linear_index(indices));
}

NoteBase::value_type NoteBase::scalar() const {
  PLUSS_CHECK(defined(), "scalar() on undefined Note");
  PLUSS_CHECK(numel() == 1, "scalar() requires exactly one element");
  return impl_->bytes_at(0);
}

::std::vector<NoteBase::value_type> NoteBase::vector() const {
  PLUSS_CHECK(defined(), "vector() on undefined Note");

  ::std::vector<value_type> out;
  out.reserve(static_cast<::std::size_t>(numel()));
  copy_flat_to(::std::back_inserter(out));
  return out;
}

::std::vector<::std::vector<NoteBase::value_type>> NoteBase::matrix() const {
  PLUSS_CHECK(defined(), "matrix() on undefined Note");
  PLUSS_CHECK(dim() <= 2, "matrix() requires scalar, 1D, or 2D Note");

  if (dim() == 0) return {{scalar()}};

  if (dim() == 1) {
    return {vector()};
  }

  const auto rows = size(0);
  const auto cols = size(1);
  ::std::vector<::std::vector<value_type>> out(static_cast<::std::size_t>(rows));
  for (index_type r = 0; r < rows; ++r) {
    auto& row = out[static_cast<::std::size_t>(r)];
    row.reserve(static_cast<::std::size_t>(cols));
    const auto base = r * cols;
    for (index_type c = 0; c < cols; ++c) {
      row.push_back(impl_->bytes_at(base + c));
    }
  }
  return out;
}

::std::vector<::std::vector<::std::vector<NoteBase::value_type>>> NoteBase::tensor3() const {
  PLUSS_CHECK(defined(), "tensor3() on undefined Note");
  PLUSS_CHECK(dim() == 3, "tensor3() requires a 3D Note");

  const auto d0 = size(0);
  const auto d1 = size(1);
  const auto d2 = size(2);
  ::std::vector<::std::vector<::std::vector<value_type>>> out(static_cast<::std::size_t>(d0));

  for (index_type i = 0; i < d0; ++i) {
    auto& slab = out[static_cast<::std::size_t>(i)];
    slab.resize(static_cast<::std::size_t>(d1));
    for (index_type j = 0; j < d1; ++j) {
      auto& row = slab[static_cast<::std::size_t>(j)];
      row.reserve(static_cast<::std::size_t>(d2));
      const auto base = i * d1 * d2 + j * d2;
      for (index_type k = 0; k < d2; ++k) {
        row.push_back(impl_->bytes_at(base + k));
      }
    }
  }
  return out;
}

::std::string NoteBase::join_flat(::std::string_view sep) const {
  PLUSS_CHECK(defined(), "join_flat() on undefined Note");
  if (numel() == 0) return {};

  ::std::string out;
  const auto n = numel();
  for (index_type i = 0; i < n; ++i) {
    if (i != 0) out.append(sep);
    out.append(impl_->bytes_at(i));
  }
  return out;
}

::std::string NoteBase::debug(index_type max_items) const {
  if (!defined()) return "Note(undefined)";
  if (max_items < 0) max_items = 0;

  ::std::ostringstream oss;
  oss << "Note(shape=[";
  const auto shp = shape();
  for (::std::size_t i = 0; i < shp.size(); ++i) {
    if (i != 0) oss << ", ";
    oss << shp[i];
  }
  oss << "], dim=" << dim()
      << ", numel=" << numel()
      << ", encoding=" << (is_bytes() ? "bytes" : "utf8")
      << ", values=[";

  const auto shown = ::std::min<index_type>(numel(), max_items);
  for (index_type i = 0; i < shown; ++i) {
    if (i != 0) oss << ", ";
    oss << '"' << escape_for_debug(impl_->bytes_at(i)) << '"';
  }
  if (numel() > shown) oss << ", ...";
  oss << "])";
  return oss.str();
}

::std::vector<::std::int64_t> Note::sizes() const {
  ::std::vector<::std::int64_t> out;
  const auto d = dim();
  out.reserve(static_cast<::std::size_t>(d));
  const auto s = sizes3();
  for (::std::int64_t i = 0; i < d; ++i) out.push_back(s[static_cast<::std::size_t>(i)]);
  return out;
}

::std::string_view Note::at(::std::int64_t linear) const {
  PLUSS_CHECK(defined(), "Note::at on undefined Note");
  const auto& impl = this->impl();
  PLUSS_CHECK(linear >= 0 && linear < impl->numel(), "Note::at index out of range");
  return impl->bytes_at(linear);
}

Note Note::latex_to_text(::std::int64_t mode) const {
  return ops_internal::tex_to_text(*this, mode);
}

Note Note::text_to_latex(::std::int64_t mode) const {
  return ops_internal::text_to_tex(*this, mode);
}

Note Note::render_latex(::std::int64_t mode) const {
  return ops_internal::tex_render(*this, mode);
}

Note Note::operator+(const Note& other) const { return note_concat_pair(*this, other); }
Note Note::operator+(::std::string_view suffix) const {
  const ::std::array<::std::string_view, 1> flat{suffix};
  auto scalar = static_cast<const NoteBase&>(*this).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return note_concat_pair(*this, scalar);
}
Note& Note::operator+=(const Note& other) {
  *this = *this + other;
  return *this;
}
Note& Note::operator+=(::std::string_view suffix) {
  *this = *this + suffix;
  return *this;
}
Note Note::operator*(::std::int64_t times) const { return note_repeat_scalar(*this, times); }
Note& Note::operator*=(::std::int64_t times) {
  *this = *this * times;
  return *this;
}

::std::vector<::std::uint8_t> Note::operator==(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Eq);
}
::std::vector<::std::uint8_t> Note::operator!=(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Ne);
}
::std::vector<::std::uint8_t> Note::operator<(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Lt);
}
::std::vector<::std::uint8_t> Note::operator<=(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Le);
}
::std::vector<::std::uint8_t> Note::operator>(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Gt);
}
::std::vector<::std::uint8_t> Note::operator>=(const Note& other) const {
  return note_compare_pair(*this, other, NoteCmpOp::Ge);
}

::std::vector<::std::uint8_t> Note::operator==(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Eq);
}
::std::vector<::std::uint8_t> Note::operator!=(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Ne);
}
::std::vector<::std::uint8_t> Note::operator<(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Lt);
}
::std::vector<::std::uint8_t> Note::operator<=(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Le);
}
::std::vector<::std::uint8_t> Note::operator>(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Gt);
}
::std::vector<::std::uint8_t> Note::operator>=(::std::string_view other) const {
  return note_compare_scalar(*this, other, NoteCmpOp::Ge);
}

Note operator+(::std::string_view prefix, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{prefix};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar + rhs;
}

Note operator*(::std::int64_t times, const Note& rhs) { return rhs * times; }

::std::vector<::std::uint8_t> operator==(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar == rhs;
}
::std::vector<::std::uint8_t> operator!=(::std::string_view lhs, const Note& rhs) {
  const auto eq = (lhs == rhs);
  ::std::vector<::std::uint8_t> out(eq.size(), 0);
  for (::std::size_t i = 0; i < eq.size(); ++i) out[i] = static_cast<::std::uint8_t>(eq[i] == 0);
  return out;
}
::std::vector<::std::uint8_t> operator<(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar < rhs;
}
::std::vector<::std::uint8_t> operator<=(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar <= rhs;
}
::std::vector<::std::uint8_t> operator>(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar > rhs;
}
::std::vector<::std::uint8_t> operator>=(::std::string_view lhs, const Note& rhs) {
  const ::std::array<::std::string_view, 1> flat{lhs};
  auto scalar = static_cast<const NoteBase&>(rhs).is_bytes()
                    ? ops_internal::note_from_bytes(flat, {})
                    : ops_internal::note_from_utf8(flat, {});
  return scalar >= rhs;
}

} // namespace pluss::coden

#ifdef CODEN_ENABLE_SERIES_INTEROP
#include "CodeM/core/SeriesSlim.h"

namespace pluss::coden {

pluss::codem::Series Note::contains_series(::std::string_view needle) const {
  return ops_internal::note_contains_series(*this, needle);
}

pluss::codem::Series Note::lengths_bytes_series() const {
  return ops_internal::note_lengths_bytes_series(*this);
}

} // namespace pluss::coden
#endif // CODEN_ENABLE_SERIES_INTEROP
