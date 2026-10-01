#pragma once

// `pluss::series::*` (series_factory.h) is the primary Series factory
// surface. `pluss::make_series` is the one flat spelling kept as a permanent
// shortcut (mirroring the Note, Timeline, and TimeIndex factory families/
// make_duration) for the common construct-from-data case -- every other flat
// spelling (pluss::zeros, pluss::full, pluss::pile, ...) was a transitional
// compatibility alias and has been removed now that all internal call sites
// across the codebase go through pluss::series::* directly.

#include "pluss/codem/codem.h"
#include "pluss/detail/data_reader.h"

#include <utility>

namespace pluss {

template <class... Opts>
inline Series make_series(detail::DataReader data, detail::DataReader::Option first = {}, Opts&&... opts) {
  return pluss::series::make(std::move(data), std::move(first), std::forward<Opts>(opts)...);
}

} // namespace pluss
