#include "CodeM/core/SeriesTape.h"

#include "CodeM/core/Errors.h"

#include <algorithm>
#include <mutex>
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

std::mutex g_mu;
std::unordered_map<const SeriesImpl*, TapeState> g_states;
thread_local int g_pause_depth = 0;

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

} // namespace

PauseRecording::PauseRecording() { ++g_pause_depth; }
PauseRecording::~PauseRecording() { --g_pause_depth; }

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

void bind_source_link(const Series& s, std::shared_ptr<SourceLink> link) {
  if (!s.defined()) return;
  std::scoped_lock lk(g_mu);
  auto& st = ensure_locked_(s);
  st.tracks_slope = true;
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

void backpropagate(const Series& loss, const std::optional<Series>& seed) {
  PLUSS_CHECK(loss.defined(), "series_tape::backpropagate: expected defined loss");

  std::vector<Series> topo;
  std::unordered_set<const SeriesImpl*> seen;
  topo_visit_(loss, seen, topo);

  {
    std::scoped_lock lk(g_mu);
    for (const auto& n : topo) {
      clear_sensitivity_locked_(n);
    }
  }

  std::optional<Series> root_sens;
  if (seed.has_value()) {
    root_sens = *seed;
  } else {
    PLUSS_CHECK(loss.numel() == 1,
                "series_tape::backpropagate: non-scalar output requires explicit seed");
    root_sens = Series::ones(loss.sizes(), loss.scalar_type()).to(loss.platform());
  }
  accumulate_sensitivity(loss, *root_sens);

  PauseRecording no_record;
  const auto* root_key = key_of_(loss);
  for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
    const auto& cur = *it;
    auto sens = sensitivity_copy_(cur);
    auto link = source_link_copy_(cur);
    if (!sens.has_value() || !link || !link->propagate) {
      continue;
    }
    link->propagate(*sens);

    if (!keep_sensitivity_copy_(cur) && key_of_(cur) != root_key) {
      clear_sensitivity(cur);
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
