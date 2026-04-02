#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"
#include "CodeM/compat/Invoke.h"

#include <array>
#include <string_view>
#include <utility>

namespace pluss::subsp::linearalg {

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
