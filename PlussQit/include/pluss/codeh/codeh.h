#pragma once
// Public CodeH entry point (Timeline/Duration).

#include <cstdint>
#include <span>
#include <string_view>

#include "CodeH/core/Timeline.h"
#include "CodeH/core/Duration.h"
#include "CodeH/core/TimeIndex.h"
#include "CodeH/core/Printing.h"
#include "CodeH/compat/Invoke.h"

namespace pluss::codeh::ops_internal {

  // Parse ISO-like datetime strings into Timeline ticks (UTC epoch nanos).
  // utf8_flat is a flattened list of scalar strings (length = product(sizes)).
  // sizes is the desired N-D shape (0D..3D).
  Timeline timeline_from_utf8(std::span<const std::string_view> utf8_flat,
                              std::span<const std::int64_t> sizes);

  // Construct from epoch nanoseconds (UTC). ticks_flat length must equal product(sizes).
  Timeline timeline_from_epoch_nanos(std::span<const std::int64_t> ticks_flat,
                                    std::span<const std::int64_t> sizes);

  // Duration from nanoseconds.
  Duration duration_from_nanos(std::span<const std::int64_t> nanos_flat,
                               std::span<const std::int64_t> sizes);

} // namespace pluss::codeh::ops_internal

namespace pluss {
using Timeline = codeh::Timeline;
using Duration = codeh::Duration;
using DurationTotal = codeh::DurationTotal;
using codeh::BusinessCalendar;
using codeh::TimelineSpanAnchor;
using codeh::TimelineSpanClosed;
using codeh::TimelineSpanMode;
using codeh::TimelineSpanOptions;
using codeh::TimelineSpanUnit;
using codeh::TimelinePrintOptions;
using codeh::operator<<;

} // namespace pluss
