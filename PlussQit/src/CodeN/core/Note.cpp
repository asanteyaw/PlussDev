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
