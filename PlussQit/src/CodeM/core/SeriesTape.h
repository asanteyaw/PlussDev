#pragma once

#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesImpl.h"

#include <functional>
#include <memory>
#include <optional>
#include <source_location>
#include <string>
#include <vector>

namespace pluss::codem::series_tape {

// Renamed internal autograd terms:
// - tracks_slope  (was requires_grad)
// - sensitivity   (was grad)
// - source_link   (was grad_fn)
struct SourceLink {
  std::vector<Series> parents{};
  std::vector<std::int64_t> parent_versions{};
  std::string op_name{};
  std::string capture_file{};
  std::string capture_function{};
  std::uint_least32_t capture_line{0};
  std::function<void(const Series&)> propagate{};
};

struct SourceInfo {
  std::string op_name{};
  std::size_t parent_count{0};
  std::string capture_file{};
  std::string capture_function{};
  std::uint_least32_t capture_line{0};
};

class PauseRecording final {
public:
  PauseRecording();
  ~PauseRecording();

  PauseRecording(const PauseRecording&) = delete;
  PauseRecording& operator=(const PauseRecording&) = delete;
};

struct ReverseDiagnostics {
  bool enabled{false};
  bool check_non_finite{true};
};

void set_reverse_diagnostics(bool enabled, bool check_non_finite = true) noexcept;
ReverseDiagnostics reverse_diagnostics() noexcept;

class ReverseDiagnosticsScope final {
public:
  explicit ReverseDiagnosticsScope(bool check_non_finite = true) noexcept;
  ~ReverseDiagnosticsScope();

  ReverseDiagnosticsScope(const ReverseDiagnosticsScope&) = delete;
  ReverseDiagnosticsScope& operator=(const ReverseDiagnosticsScope&) = delete;

private:
  ReverseDiagnostics previous_{};
};

bool recording_enabled() noexcept;

bool tracks_slope(const Series& s);
void set_tracks_slope(const Series& s, bool enabled);

bool keeps_sensitivity(const Series& s);
void set_keep_sensitivity(const Series& s, bool keep);

bool is_leaf(const Series& s);
std::optional<SourceInfo> source_info(const Series& s);

void bind_source_link(
    const Series& s,
    std::shared_ptr<SourceLink> link,
    const std::source_location& loc = std::source_location::current());
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
