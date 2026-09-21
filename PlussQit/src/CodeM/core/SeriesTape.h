#pragma once

#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesImpl.h"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <source_location>
#include <string>
#include <vector>

namespace pluss::codem::series_tape {

// INTERNAL ENGINE NAMESPACE -- not the public Series surface, even though
// this header is transitively reachable from public includes (it's used by
// pluss/subsp/math/diff.h and other engine-internal headers). End users
// should reach for the Series member surface instead:
//   - Series::source()          <- reverse-mode node info (see SourceInfo
//                                  below); this is the one most users want.
//   - Series::tracks_slope()    <- was requires_grad
//   - Series::sensitivity()     <- was grad
//   - Series::has_sensitivity()
//   - Series::is_leaf()
// The free functions below (source_info/source_link/bind_source_link/
// accumulate_sensitivity/...) exist for op implementations (OpsInternal.cpp,
// Invoke.cpp) and the autodiff test suite to wire and inspect the tape
// directly; they carry engine-internal detail (capture file/line, the raw
// propagate closure) that the public wrapper deliberately does not expose.
//
// Renamed internal terms (previously torch-style autograd naming; this
// engine consistently calls the whole system "autodiff"):
// - tracks_slope  (was requires_grad)
// - sensitivity   (was grad)
// - source_link   (was grad_fn)
struct SourceLink {
  std::vector<Series> parents{};
  std::vector<std::int64_t> parent_versions{};
  // Optional, parallel to `parents`: false means this parent is only a graph
  // edge for gradient accumulation and its *data* is not read by any backward
  // rule that will run, so an in-place mutation of it after capture is
  // harmless and must not trip the version check (matches how libtorch guards
  // only actually-saved variables). Empty => guard every parent (safe default).
  std::vector<bool> parent_data_guarded{};
  std::int64_t output_version{0};
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

// Thread-local grad-mode control, mirroring the core libtorch shape:
// a process-wide API would be wrong here because one thread's inference
// scope must not silently affect another thread's training work.
bool grad_mode_enabled() noexcept;
void set_grad_mode_enabled(bool enabled) noexcept;

class AutoGradMode final {
public:
  explicit AutoGradMode(bool enabled) noexcept;
  ~AutoGradMode();

  AutoGradMode(const AutoGradMode&) = delete;
  AutoGradMode& operator=(const AutoGradMode&) = delete;
  AutoGradMode(AutoGradMode&&) = delete;
  AutoGradMode& operator=(AutoGradMode&&) = delete;

private:
  bool previous_{true};
};

class NoGradGuard final {
public:
  NoGradGuard() noexcept;
  ~NoGradGuard() = default;

  NoGradGuard(const NoGradGuard&) = delete;
  NoGradGuard& operator=(const NoGradGuard&) = delete;
  NoGradGuard(NoGradGuard&&) = delete;
  NoGradGuard& operator=(NoGradGuard&&) = delete;

private:
  AutoGradMode guard_;
};

struct ReverseDiagnostics {
  bool enabled{false};
  bool check_non_finite{true};
};

struct BackpropOptions {
  // When true, clear existing sensitivities before this reverse pass.
  // When false, leaf sensitivities accumulate across passes.
  bool reset_sensitivity_before_run{true};
  // When false, drop source links for visited nodes after reverse.
  // This mirrors one-shot graph usage semantics.
  bool keep_source_links{true};
  // When true, allow reverse propagation math to be tape-recorded so
  // sensitivities can participate in higher-order reverse passes.
  bool record_propagation{false};
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
std::shared_ptr<SourceLink> source_link(const Series& s);

void bind_source_link(
    const Series& s,
    std::shared_ptr<SourceLink> link,
    const std::source_location& loc = std::source_location::current());
// Records alias lineage without forcing `tracks_slope=true` on `s`.
// This is used by view-producing ops so later in-place mutations can recover
// the base alias chain even when the base was not tracked at view creation.
void bind_alias_link(
    const Series& s,
    std::shared_ptr<SourceLink> link,
    const std::source_location& loc = std::source_location::current());
void clear_source_link(const Series& s);

void clear_sensitivity(const Series& s);
void accumulate_sensitivity(const Series& s, const Series& sens);
std::optional<Series> sensitivity(const Series& s);
Series sensitivity_or_zeros(const Series& s);
void prepare_tracked_leaves(std::span<Series* const> leaves);

void backpropagate(const Series& loss,
                   const std::optional<Series>& seed = std::nullopt,
                   const BackpropOptions& options = {});

// Detach semantics: return a view sharing storage metadata but with no tape link.
Series separate(const Series& s);

// Called from SeriesImpl destructor so stale pointer keys are never kept.
void purge_impl_state(const SeriesImpl* impl);

} // namespace pluss::codem::series_tape
