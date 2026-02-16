#include "CodeN/core/Note.h"

#include "CodeN/core/OpsInternal.h"

namespace pluss::coden {

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
