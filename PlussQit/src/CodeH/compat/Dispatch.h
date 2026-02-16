#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "CodeH/core/TimeIndex.h"
#include "CodeH/core/Timeline.h"

namespace pluss::codeh::_compat {

// Dispatch entry points used by invoke<...>.
// These are implemented in Dispatch.cpp and route to CodeH ops_internal
// implementations.

struct Args;

Timeline call_timeline(std::string_view op, const Timeline& self, const Args& args);
Duration call_duration(std::string_view op, const Duration& self, const Args& args);
Duration call_duration_from_timeline(std::string_view op, const Timeline& self, const Args& args);
pluss::codem::Series call_series_timeline(std::string_view op, const Timeline& self, const Args& args);
pluss::codem::Series call_series_duration(std::string_view op, const Duration& self, const Args& args);
pluss::coden::Note call_note_duration(std::string_view op, const Duration& self, const Args& args);
std::vector<std::int64_t> call_i64vec_duration(std::string_view op, const Duration& self, const Args& args);
pluss::codem::Series call_series_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
std::int64_t call_i64_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
bool call_bool_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
std::string call_string_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
pluss::coden::Note call_note_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
Timeline call_timeline_from_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
TimeIndex call_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
TimeIndex::ResampleBins call_resamplebins_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
TimeIndex::AlignUnion call_alignunion_timeindex(std::string_view op, const TimeIndex& self, const Args& args);
pluss::coden::Note call_note_timeline(std::string_view op, const Timeline& self, const Args& args);
std::string call_string_timeline(std::string_view op, const Timeline& self, const Args& args);
bool call_bool_timeline(std::string_view op, const Timeline& self, const Args& args);
std::vector<std::int64_t> call_i64vec_timeline(std::string_view op, const Timeline& self, const Args& args);

// Static-style: construct TimeIndex from a Timeline.
TimeIndex call_timeindex_from_timeline(std::string_view op, const Timeline& self, const Args& args);

} // namespace pluss::codeh::_compat
