#pragma once

#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesImpl.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace pluss::codem::series_tape {

// Renamed internal autograd terms:
// - tracks_slope  (was requires_grad)
// - sensitivity   (was grad)
// - source_link   (was grad_fn)
struct SourceLink {
  std::vector<Series> parents{};
  std::vector<std::int64_t> parent_versions{};
  std::function<void(const Series&)> propagate{};
};

class PauseRecording final {
public:
  PauseRecording();
  ~PauseRecording();

  PauseRecording(const PauseRecording&) = delete;
  PauseRecording& operator=(const PauseRecording&) = delete;
};

bool recording_enabled() noexcept;

bool tracks_slope(const Series& s);
void set_tracks_slope(const Series& s, bool enabled);

bool keeps_sensitivity(const Series& s);
void set_keep_sensitivity(const Series& s, bool keep);

bool is_leaf(const Series& s);

void bind_source_link(const Series& s, std::shared_ptr<SourceLink> link);
void clear_source_link(const Series& s);

void clear_sensitivity(const Series& s);
void accumulate_sensitivity(const Series& s, const Series& sens);
std::optional<Series> sensitivity(const Series& s);
Series sensitivity_or_zeros(const Series& s);

void backpropagate(const Series& loss, const std::optional<Series>& seed = std::nullopt);

// Detach semantics: return a view sharing storage metadata but with no tape link.
Series separate(const Series& s);

// Called from SeriesImpl destructor so stale pointer keys are never kept.
void purge_impl_state(const SeriesImpl* impl);

} // namespace pluss::codem::series_tape
