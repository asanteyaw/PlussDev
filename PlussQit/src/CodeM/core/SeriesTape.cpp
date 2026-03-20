#include "CodeM/core/SeriesTape.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pluss::codem::series_tape {
namespace {

struct TapeState {
  bool tracks_slope{false};
  bool keep_sensitivity{false};
  std::optional<Series> sensitivity{};
  std::shared_ptr<SourceLink> source_link{};
};

// Keep tape globals alive until process exit to avoid static-destruction-order
// issues when SeriesImpl teardown still calls purge_impl_state.
std::recursive_mutex& g_mu = *new std::recursive_mutex();
std::unordered_map<const SeriesImpl*, TapeState>& g_states =
    *new std::unordered_map<const SeriesImpl*, TapeState>();
thread_local int g_pause_depth = 0;
thread_local ReverseDiagnostics g_reverse_diag{};

const SeriesImpl* key_of_(const Series& s) noexcept {
  return s.defined() ? s.unsafe_impl() : nullptr;
}

void maybe_prune_locked_(const SeriesImpl* key) {
  if (key == nullptr) return;
  auto it = g_states.find(key);
  if (it == g_states.end()) return;
  const auto& st = it->second;
  if (!st.tracks_slope && !st.keep_sensitivity && !st.sensitivity.has_value() && !st.source_link) {
    g_states.erase(it);
  }
}

TapeState* find_locked_(const Series& s) {
  const auto* key = key_of_(s);
  if (key == nullptr) return nullptr;
  auto it = g_states.find(key);
  if (it == g_states.end()) return nullptr;
  return &it->second;
}

TapeState& ensure_locked_(const Series& s) {
  const auto* key = key_of_(s);
  PLUSS_CHECK(key != nullptr, "series_tape: expected defined Series");
  return g_states[key];
}

std::shared_ptr<SourceLink> source_link_copy_(const Series& s) {
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  return st ? st->source_link : nullptr;
}

std::optional<Series> sensitivity_copy_(const Series& s) {
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  return st ? st->sensitivity : std::nullopt;
}

bool keep_sensitivity_copy_(const Series& s) {
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  return st ? st->keep_sensitivity : false;
}

void clear_sensitivity_locked_(const Series& s) {
  auto* st = find_locked_(s);
  if (!st) return;
  st->sensitivity.reset();
  maybe_prune_locked_(key_of_(s));
}

void topo_visit_(const Series& s,
                 std::unordered_set<const SeriesImpl*>& seen,
                 std::vector<Series>& out) {
  const auto* key = key_of_(s);
  if (key == nullptr) return;
  if (seen.find(key) != seen.end()) return;
  seen.insert(key);

  auto link = source_link_copy_(s);
  if (link) {
    for (const auto& p : link->parents) {
      topo_visit_(p, seen, out);
    }
  }
  out.push_back(s);
}

bool same_shape_(const Series& a, const Series& b) {
  const auto as = a.sizes();
  const auto bs = b.sizes();
  if (as.size() != bs.size()) return false;
  for (std::size_t i = 0; i < as.size(); ++i) {
    if (as[i] != bs[i]) return false;
  }
  return true;
}

bool compatible_shape_or_scalar_(const Series& ref, const Series& value) {
  if (same_shape_(ref, value)) return true;
  return ref.numel() == 1 && value.numel() == 1;
}

bool has_non_finite_(const Series& x) {
  const ScalarType tally_dtype =
      (x.platform() == Platform::MPS) ? ScalarType::Float32 : ScalarType::Float64;
  const Series non_finite = x.isfinite().logical_not().to(tally_dtype);
  return non_finite.sum().to(Platform::CPU, ScalarType::Float64).item<double>() > 0.0;
}

std::string op_name_of_(const std::shared_ptr<SourceLink>& link) {
  if (!link) return "unknown";
  if (link->op_name.empty()) return "unknown";
  return link->op_name;
}

std::string captured_at_of_(const std::shared_ptr<SourceLink>& link) {
  if (!link) return "unknown";
  if (link->capture_file.empty()) return "unknown";
  std::ostringstream oss;
  oss << link->capture_file;
  if (link->capture_line != 0) {
    oss << ":" << link->capture_line;
  }
  if (!link->capture_function.empty()) {
    oss << " (" << link->capture_function << ")";
  }
  return oss.str();
}

} // namespace

PauseRecording::PauseRecording() { ++g_pause_depth; }
PauseRecording::~PauseRecording() { --g_pause_depth; }

void set_reverse_diagnostics(bool enabled, bool check_non_finite) noexcept {
  g_reverse_diag.enabled = enabled;
  g_reverse_diag.check_non_finite = check_non_finite;
}

ReverseDiagnostics reverse_diagnostics() noexcept { return g_reverse_diag; }

ReverseDiagnosticsScope::ReverseDiagnosticsScope(bool check_non_finite) noexcept
    : previous_(reverse_diagnostics()) {
  set_reverse_diagnostics(true, check_non_finite);
}

ReverseDiagnosticsScope::~ReverseDiagnosticsScope() {
  set_reverse_diagnostics(previous_.enabled, previous_.check_non_finite);
}

bool recording_enabled() noexcept { return g_pause_depth == 0; }

bool tracks_slope(const Series& s) {
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  return st ? st->tracks_slope : false;
}

void set_tracks_slope(const Series& s, bool enabled) {
  if (!s.defined()) return;
  std::scoped_lock lk(g_mu);
  auto& st = ensure_locked_(s);
  st.tracks_slope = enabled;
  st.sensitivity.reset();
  if (!enabled) {
    st.source_link.reset();
  }
  maybe_prune_locked_(key_of_(s));
}

bool keeps_sensitivity(const Series& s) {
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  return st ? st->keep_sensitivity : false;
}

void set_keep_sensitivity(const Series& s, bool keep) {
  if (!s.defined()) return;
  std::scoped_lock lk(g_mu);
  auto& st = ensure_locked_(s);
  st.keep_sensitivity = keep;
  maybe_prune_locked_(key_of_(s));
}

bool is_leaf(const Series& s) {
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  if (!st) return true;
  return !st->source_link;
}

std::optional<SourceInfo> source_info(const Series& s) {
  auto link = source_link_copy_(s);
  if (!link) return std::nullopt;
  SourceInfo info{};
  info.op_name = link->op_name;
  info.parent_count = link->parents.size();
  info.capture_file = link->capture_file;
  info.capture_function = link->capture_function;
  info.capture_line = link->capture_line;
  return info;
}

void bind_source_link(
    const Series& s,
    std::shared_ptr<SourceLink> link,
    const std::source_location& loc) {
  if (!s.defined()) return;
  if (link && link->capture_file.empty()) {
    link->capture_file = loc.file_name();
    link->capture_function = loc.function_name();
    link->capture_line = loc.line();
  }
  if (link && link->parent_versions.empty()) {
    link->parent_versions.reserve(link->parents.size());
    for (const auto& p : link->parents) {
      link->parent_versions.push_back(p.defined() ? p.storage().version() : 0);
    }
  }
  std::scoped_lock lk(g_mu);
  auto& st = ensure_locked_(s);
  st.tracks_slope = true;
  st.sensitivity.reset();
  st.source_link = std::move(link);
}

void clear_source_link(const Series& s) {
  if (!s.defined()) return;
  std::scoped_lock lk(g_mu);
  auto* st = find_locked_(s);
  if (!st) return;
  st->source_link.reset();
  maybe_prune_locked_(key_of_(s));
}

void clear_sensitivity(const Series& s) {
  if (!s.defined()) return;
  std::scoped_lock lk(g_mu);
  clear_sensitivity_locked_(s);
}

void accumulate_sensitivity(const Series& s, const Series& sens) {
  if (!s.defined()) return;
  std::scoped_lock lk(g_mu);
  auto& st = ensure_locked_(s);
  if (!st.tracks_slope) return;
  if (!st.sensitivity.has_value()) {
    st.sensitivity = sens;
  } else {
    st.sensitivity = st.sensitivity->add(sens);
  }
}

std::optional<Series> sensitivity(const Series& s) { return sensitivity_copy_(s); }

Series sensitivity_or_zeros(const Series& s) {
  auto sens = sensitivity_copy_(s);
  if (sens.has_value()) return *sens;
  return Series::zeros(s.sizes(), s.scalar_type()).to(s.platform());
}

void backpropagate(const Series& loss,
                   const std::optional<Series>& seed,
                   const BackpropOptions& options) {
  PLUSS_CHECK(loss.defined(), "series_tape::backpropagate: expected defined loss");

  std::vector<Series> topo;
  std::unordered_set<const SeriesImpl*> seen;
  topo_visit_(loss, seen, topo);

  if (options.reset_sensitivity_before_run) {
    std::scoped_lock lk(g_mu);
    for (const auto& n : topo) {
      clear_sensitivity_locked_(n);
    }
  } else {
    // Root sensitivity should not implicitly accumulate across passes unless
    // explicitly retained; clear stale root seed before re-seeding this run.
    if (!keeps_sensitivity(loss)) {
      clear_sensitivity(loss);
    }
  }

  const ReverseDiagnostics diagnostics = reverse_diagnostics();

  std::optional<Series> root_sens;
  if (seed.has_value()) {
    PLUSS_CHECK(seed->defined(), "series_tape::backpropagate: seed must be defined");
    PLUSS_CHECK(
        seed->platform() == loss.platform(),
        "series_tape::backpropagate: seed/platform mismatch with loss");
    PLUSS_CHECK(
        compatible_shape_or_scalar_(loss, *seed),
        "series_tape::backpropagate: seed shape must match loss shape (or both scalar-numel)");
    root_sens = (seed->scalar_type() == loss.scalar_type()) ? *seed : seed->to(loss.scalar_type());
  } else {
    PLUSS_CHECK(loss.numel() == 1,
                "series_tape::backpropagate: non-scalar output requires explicit seed");
    root_sens = Series::ones(loss.sizes(), loss.scalar_type()).to(loss.platform());
  }
  if (diagnostics.enabled && diagnostics.check_non_finite && has_non_finite_(*root_sens)) {
    throw_error("series_tape::backpropagate: non-finite root sensitivity detected");
  }
  accumulate_sensitivity(loss, *root_sens);

  std::optional<PauseRecording> no_record;
  if (!options.record_propagation) {
    no_record.emplace();
  }
  const auto* root_key = key_of_(loss);
  std::size_t visited_nodes = 0;
  for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
    ++visited_nodes;
    const auto& cur = *it;
    auto sens = sensitivity_copy_(cur);
    auto link = source_link_copy_(cur);
    if (!sens.has_value() || !link || !link->propagate) {
      continue;
    }
    if (diagnostics.enabled && diagnostics.check_non_finite && has_non_finite_(*sens)) {
      std::ostringstream oss;
      oss << "series_tape::backpropagate: non-finite sensitivity detected before propagate at traversal step "
          << visited_nodes << " [op=" << op_name_of_(link)
          << ", captured_at=" << captured_at_of_(link) << "]";
      throw_error(oss.str());
    }
    PLUSS_CHECK(
        compatible_shape_or_scalar_(cur, *sens),
        "series_tape::backpropagate: internal sensitivity shape mismatch for node");
    if (!link->parent_versions.empty()) {
      PLUSS_CHECK(
          link->parent_versions.size() == link->parents.size(),
          "series_tape::backpropagate: saved parent version metadata mismatch");
      for (std::size_t i = 0; i < link->parents.size(); ++i) {
        const auto& p = link->parents[i];
        if (!p.defined()) continue;
        PLUSS_CHECK(
            p.storage().version() == link->parent_versions[i],
            std::string("series_tape::backpropagate: saved Series was modified after graph capture [op=") +
                op_name_of_(link) + ", captured_at=" + captured_at_of_(link) + "]");
      }
    }
    try {
      link->propagate(*sens);
    } catch (const std::exception& e) {
      std::ostringstream oss;
      oss << "series_tape::backpropagate: propagate failed at traversal step "
          << visited_nodes
          << " (op=" << op_name_of_(link)
          << ", captured_at=" << captured_at_of_(link)
          << ", parents=" << link->parents.size() << "): " << e.what();
      throw_error(oss.str());
    } catch (...) {
      std::ostringstream oss;
      oss << "series_tape::backpropagate: propagate failed with unknown exception at traversal step "
          << visited_nodes
          << " (op=" << op_name_of_(link)
          << ", captured_at=" << captured_at_of_(link)
          << ", parents=" << link->parents.size() << ")";
      throw_error(oss.str());
    }
    if (diagnostics.enabled && diagnostics.check_non_finite) {
      for (std::size_t i = 0; i < link->parents.size(); ++i) {
        const auto& parent = link->parents[i];
        auto parent_sens = sensitivity_copy_(parent);
        if (parent_sens.has_value() && has_non_finite_(*parent_sens)) {
          std::ostringstream oss;
          oss << "series_tape::backpropagate: non-finite sensitivity detected after propagate at traversal step "
              << visited_nodes << " for parent index " << i
              << " [op=" << op_name_of_(link)
              << ", captured_at=" << captured_at_of_(link) << "]";
          throw_error(oss.str());
        }
      }
    }

    if (!keep_sensitivity_copy_(cur) && key_of_(cur) != root_key) {
      clear_sensitivity(cur);
    }
  }

  if (!options.keep_source_links) {
    for (const auto& n : topo) {
      clear_source_link(n);
    }
  }
}

Series separate(const Series& s) {
  if (!s.defined()) return s;
  std::vector<std::int64_t> sizes(s.sizes().begin(), s.sizes().end());
  std::vector<std::int64_t> strides(s.strides().begin(), s.strides().end());
  Series out = Series::from_storage(
      s.storage(),
      s.storage_offset() / std::max<std::int64_t>(1, s.itemsize()),
      sizes,
      strides,
      s.scalar_type());
  set_tracks_slope(out, false);
  set_keep_sensitivity(out, false);
  clear_source_link(out);
  clear_sensitivity(out);
  return out;
}

void purge_impl_state(const SeriesImpl* impl) {
  if (impl == nullptr) return;
  std::scoped_lock lk(g_mu);
  g_states.erase(impl);
}

} // namespace pluss::codem::series_tape
