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

// d.total().hours() -- an alternate spelling for the total_* family
// (total_nanoseconds/.../total_weeks, generated above) that reads more
// naturally at a call site chaining several different units off the same
// Duration. Declared here, defined out-of-line in Duration.h once Duration
// itself is a complete type (DurationTotal stores a Duration by value).
DurationTotal total() const;
