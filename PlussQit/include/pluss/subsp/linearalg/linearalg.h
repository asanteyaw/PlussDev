#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"
#include "CodeM/compat/Invoke.h"

#include <array>
#include <string_view>
#include <utility>

namespace pluss::subsp::linearalg {

struct SvdDecomposition {
  Series u = pluss::series::undefined();
  Series s = pluss::series::undefined();
  Series vh = pluss::series::undefined();
};

inline Series call(
    std::string_view op,
    const Series& x,
    pluss::codem::_compat::ArgView args = {}) {
  return pluss::codem::_compat::invoke(op, x, args);
}

template <class... Args>
inline Series call(std::string_view op, const Series& x, Args&&... args) {
  const std::array<pluss::codem::_compat::Arg, sizeof...(Args)> packed{
      pluss::codem::_compat::Arg(std::forward<Args>(args))...};
  return call(
      op,
      x,
      pluss::codem::_compat::ArgView{packed.data(), packed.size()});
}

} // namespace pluss::subsp::linearalg

namespace pluss::codem {

inline pluss::subsp::linearalg::SvdDecomposition Series::SeriesLinearAlgProxy::svd(bool full_matrices) const {
  return pluss::subsp::linearalg::svd(self(), full_matrices);
}

inline pluss::subsp::linearalg::SvdDecomposition Series::SeriesLinearAlgAccessor::svd(bool full_matrices) const {
  return (*this)().svd(full_matrices);
}

} // namespace pluss::codem
