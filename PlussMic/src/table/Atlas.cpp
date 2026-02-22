#include "table/Atlas.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CodeM/core/Errors.h"
#include "kindle/Archive.h"
#include "table/display/Display.h"

namespace pluss::table {

#include "table/ops/atlas/AtlasInternals.inc"
#include "table/ops/atlas/AtlasCoreOps.inc"

} // namespace pluss::table
