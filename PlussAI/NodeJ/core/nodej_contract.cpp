#include "nodej_types.h"

#include <type_traits>

namespace pluss::ai::nodej::detail {

static_assert(std::is_same_v<Series, ::pluss::Series>,
              "NodeJ core type contract requires pluss::ai::nodej::Series == pluss::Series.");
static_assert(std::is_same_v<Series, ::pluss::codem::Series>,
              "NodeJ core type contract requires pluss::ai::nodej::Series == pluss::codem::Series.");

} // namespace pluss::ai::nodej::detail
