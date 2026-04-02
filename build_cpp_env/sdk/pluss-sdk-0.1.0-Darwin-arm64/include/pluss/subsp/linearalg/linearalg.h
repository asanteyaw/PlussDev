#pragma once

#include "pluss/codem/codem.h"
#include "pluss/subsp/linearalg/linearalg_fwd.h"
#include "CodeM/compat/Invoke.h"

#include <string_view>
#include <utility>

namespace pluss::subsp::linearalg {

template <class... Args>
inline Series call(std::string_view op, const Series& x, Args&&... args) {
  return pluss::codem::_compat::invoke(op, x, std::forward<Args>(args)...);
}

} // namespace pluss::subsp::linearalg
