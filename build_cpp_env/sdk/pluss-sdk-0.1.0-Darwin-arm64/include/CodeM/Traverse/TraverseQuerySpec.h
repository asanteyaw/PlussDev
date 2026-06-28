#pragma once

#include <vector>

#include "CodeM/core/SeriesImpl.h"
#include "CodeM/Indexing/ExtractSpec.h"
#include "CodeM/Traverse/TraverseFwd.h"

namespace pluss::codem {

struct TraverseQuerySpec final {
  std::vector<ExtractSpec> on_specs{};
  bool compact{true};
  bool strict{true};

  bool has_on_clause() const noexcept { return !on_specs.empty(); }
};

TraverseQuerySpec normalize_query_spec(const TraverseQuerySpec& spec);

} // namespace pluss::codem
