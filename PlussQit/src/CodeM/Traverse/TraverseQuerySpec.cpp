#include "CodeM/Traverse/TraverseQuerySpec.h"

namespace pluss::codem {

TraverseQuerySpec normalize_query_spec(const TraverseQuerySpec& spec) {
  TraverseQuerySpec out = spec;
  if (!out.compact) {
    out.compact = true;
  }
  return out;
}

} // namespace pluss::codem
