#include "table/Shelf.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>
#include <concepts>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <regex>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>


#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "CodeN/core/OpsInternal.h"
#include "table/NonFinite.h"
#include "pluss/codeh/timeline_factory.h"
#include "scribble/Slot.h"
#include "kindle/Stream.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"
#include <pluss/tils/kit/output.h>

namespace pluss::table {

#include "table/ops/shelf/ShelfInternals.inc"
#include "table/ops/shared/GroupInternals.inc"
#include "table/ops/shared/TimeAxis.inc"
#include "table/ops/shelf/ShelfAlignment.inc"
#include "table/ops/shelf/ShelfCoreOps.inc"
#include "table/ops/shelf/ShelfGroup.inc"
#include "table/ops/shelf/ShelfAdvancedOps.inc"
#include "table/ops/shelf/ShelfOperatorsAndValue.inc"

} // namespace pluss::table
