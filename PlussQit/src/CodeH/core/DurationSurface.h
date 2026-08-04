#pragma once

// This header is included *inside* class pluss::codeh::Duration.
// It carries the bulk method surface (generated wrappers + ergonomic aliases),
// routed through CodeH's invoke engine.

// Generated Timedelta/Duration methods (Torch/pandas-style wrappers).
#include "CodeH/generated/DurationMethodsGen.h"

// Convenience operators / aliases
//
// Compound-assignment sugar around the two binary operators -- the same
// trivial "*this = *this OP other; return *this;" pattern Timeline's
// operator+=/-= uses for Duration operands (see TimelineSurface.h).
Duration& operator+=(const Duration& other) { *this = (*this) + other; return *this; }
Duration& operator-=(const Duration& other) { *this = (*this) - other; return *this; }
