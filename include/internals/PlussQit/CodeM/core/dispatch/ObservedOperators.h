#pragma once

#include <PlussQit/core/operator_name.h>
#include <string>
#include <unordered_set>

namespace MSharp {

struct TORCH_API ObservedOperators {
  ObservedOperators() = delete;

  static bool isObserved(const OperatorName& name);

  static std::unordered_set<std::string>& getUnobservedOperatorList();
};

} // namespace MSharp
