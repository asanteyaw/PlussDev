#pragma once

#include "pluss/detail/data_reader.h"
#include "pluss/detail/series_finalize.h"

namespace pluss::codem {

template <class... Opts>
inline Series value(::pluss::detail::DataReader data,
                    ::pluss::detail::DataReader::Option first = {},
                    Opts&&... opts) {
  const auto state = ::pluss::detail::build_state(first, opts...);
  return ::pluss::detail::finalize(
      data.to_series(std::move(first), std::forward<Opts>(opts)...),
      state.autodiff);
}

} // namespace pluss::codem
