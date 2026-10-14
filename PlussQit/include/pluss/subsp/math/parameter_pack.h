#pragma once

// Compatibility shim.
// Parameter packing for the gradient-based optimization stack is owned by
// PlussMa (which includes it from optim.h); this forwards so PlussQit targets
// that reach optim.h through its own shim resolve the same header.

#include "../../../../../PlussMa/include/pluss/subsp/math/parameter_pack.h"
