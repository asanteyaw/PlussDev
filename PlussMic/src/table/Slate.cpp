#include "table/Slate.h"

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

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "CodeM/core/ScalarType.h"
#include "CodeN/core/OpsInternal.h"
#include "pluss/codeh/timeline_factory.h"
#include "scribble/Slot.h"
#include "kindle/Stream.h"
#include "table/DataFrame.h"
#include "table/display/Display.h"

namespace pluss::table {

#include "table/ops/slate/SlateInternals.inc"
#include "table/ops/shared/GroupInternals.inc"
#include "table/ops/shared/TimeAxis.inc"
#include "table/ops/slate/SlateAlignment.inc"
#include "table/ops/slate/SlateCoreOps.inc"
#include "table/ops/slate/SlateGroup.inc"
#include "table/ops/slate/SlateAdvancedOps.inc"
#include "table/ops/slate/SlateOperatorsAndValue.inc"

} // namespace pluss::table
