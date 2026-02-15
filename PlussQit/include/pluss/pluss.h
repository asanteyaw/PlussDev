#pragma once

#include "pluss/export.h"
#include "pluss/config.h"
#include "pluss/codem/codem.h"
#include "CodeM/core/Config.h"

#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "pluss/detail/factory_deduction.h"

namespace pluss {
  using Series = pluss::codem::Series;
  using Config = pluss::codem::Config;
  using ScalarType = pluss::codem::ScalarType;
  using BlobDeleter = pluss::codem::BlobDeleter;

  // ---- Config builder helpers ----
  using pluss::config; // from pluss/config.h
  namespace detail {
    using pluss::detail::make_config;
  }

} // namespace pluss
