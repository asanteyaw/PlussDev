#pragma once

// In order to preserve bc, we make DeprecatedTypeProperties instances unique
// just like they are for Type.

#include <MSharp/core/Backend.h>
#include <MSharp/core/ScalarType.h>
#include <memory>

namespace pluss {

class DeprecatedTypeProperties;

struct TORCH_API DeprecatedTypePropertiesDeleter {
  void operator()(DeprecatedTypeProperties * ptr);
};

class TORCH_API DeprecatedTypePropertiesRegistry {
 public:
  DeprecatedTypePropertiesRegistry();

  DeprecatedTypeProperties& getDeprecatedTypeProperties(Backend p, ScalarType s) const;

private:
  // NOLINTNEXTLINE(*c-array*)
  std::unique_ptr<DeprecatedTypeProperties> registry
    [static_cast<int>(Backend::NumOptions)]
    [static_cast<int>(ScalarType::NumOptions)];
};

TORCH_API DeprecatedTypePropertiesRegistry& globalDeprecatedTypePropertiesRegistry();

} // namespace pluss
