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

// Convenience compare operators returning Series[bool]
pluss::codem::Series operator==(const Timeline& other) const { return eq(other); }
pluss::codem::Series operator!=(const Timeline& other) const { return ne(other); }
pluss::codem::Series operator<(const Timeline& other) const { return lt(other); }
pluss::codem::Series operator<=(const Timeline& other) const { return le(other); }
pluss::codem::Series operator>(const Timeline& other) const { return gt(other); }
pluss::codem::Series operator>=(const Timeline& other) const { return ge(other); }
