#pragma once

// This header is included *inside* class pluss::codeh::Timeline.
// It carries the bulk op surface (generated wrappers + operator sugar).

#ifdef CODEH_ASSERT_NO_TIMELINE_SURFACE
#error "Timeline op surface is disabled for this translation unit."
#endif

#include "CodeH/generated/TimelineMethodsGen.h"

// Operator sugar (thin wrappers around generated methods)
Timeline operator+(const Duration& d) const { return add_duration(d); }
Timeline operator-(const Duration& d) const { return sub_duration(d); }
Duration operator-(const Timeline& other) const { return diff(other); }

// Compound-assignment sugar around the two binary operators above -- same
// trivial "*this = *this OP other; return *this;" pattern as Duration's
// operator+=/-= (see DurationSurface.h).
Timeline& operator+=(const Duration& d) { *this = (*this) + d; return *this; }
Timeline& operator-=(const Duration& d) { *this = (*this) - d; return *this; }
