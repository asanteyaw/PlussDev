#include "kindle/catalog.h"
#include "kindle/ColumnView.h"

#include <algorithm>
#include <filesystem>
#include "table/io/SnapshotCsv.h"

#include "MSharp/Parallel.h"
#include "relational/Match.h"
#include <array>
#include <list>
#include <memory>
#include <mutex>
#include <cstring>
#include <cctype>
#include <exception>
#include <limits>
#include <cmath>
#include <iostream>
#include <ostream>
#include <numeric>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


#include "CodeM/core/Errors.h"
#include "scribble/registry.h"
#include "loader/Config.h"
#include "loader/Loader.h"
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "kindle/display/Display.h"
#include "table/DataFrame.h"
#include "table/io/CsvSchemaConvert.h"
#include "table/plot/detail/Plot3DConfig.h"
#include <pluss/tils/kit/output.h>

namespace pluss::kindle {

namespace {

std::vector<std::string> normalize_names(std::initializer_list<std::string_view> names) {
  std::vector<std::string> out;
  out.reserve(names.size());
  std::unordered_set<std::string> seen;
  for (const auto nsv : names) {
    std::string n(nsv);
    PLUSS_CHECK(!n.empty(), "Catalog: key cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "Catalog: duplicate key '" + n + "'");
    out.push_back(std::move(n));
  }
  return out;
}

std::vector<std::string> normalize_names(std::span<const std::string_view> names) {
  std::vector<std::string> out;
  out.reserve(names.size());
  std::unordered_set<std::string> seen;
  for (const auto nsv : names) {
    std::string n(nsv);
    PLUSS_CHECK(!n.empty(), "Catalog: key cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "Catalog: duplicate key '" + n + "'");
    out.push_back(std::move(n));
  }
  return out;
}

std::vector<std::string> normalize_names(const std::vector<std::string>& names) {
  std::vector<std::string> out;
  out.reserve(names.size());
  std::unordered_set<std::string> seen;
  for (const auto& n : names) {
    PLUSS_CHECK(!n.empty(), "Catalog: key cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "Catalog: duplicate key '" + n + "'");
    out.push_back(n);
  }
  return out;
}

Catalog project_catalog(const Catalog& catalog,
                        const std::vector<std::string>& names,
                        std::string_view where) {
  PLUSS_CHECK(catalog.defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(names.size());
  for (const auto& name : names) {
    PLUSS_CHECK(catalog.has(name), std::string(where) + ": unknown key '" + name + "'");
    out.emplace_back(name, catalog.keys(name));
  }
  return Catalog(std::move(out));
}

std::unordered_set<std::string> normalize_name_set(std::initializer_list<std::string_view> names) {
  const auto list = normalize_names(names);
  return std::unordered_set<std::string>(list.begin(), list.end());
}

std::unordered_set<std::string> normalize_name_set(std::span<const std::string_view> names) {
  const auto list = normalize_names(names);
  return std::unordered_set<std::string>(list.begin(), list.end());
}

std::unordered_set<std::string> normalize_name_set(const std::vector<std::string>& names) {
  const auto list = normalize_names(names);
  return std::unordered_set<std::string>(list.begin(), list.end());
}

std::string trim_copy(std::string_view in) {
  std::size_t start = 0;
  std::size_t end = in.size();
  while (start < end && std::isspace(static_cast<unsigned char>(in[start]))) ++start;
  while (end > start && std::isspace(static_cast<unsigned char>(in[end - 1]))) --end;
  return std::string(in.substr(start, end - start));
}

std::string lowercase_copy(std::string_view in) {
  std::string out;
  out.reserve(in.size());
  for (char c : in) {
    out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  return out;
}

std::vector<std::string> tokenize_ascii_words(std::string_view text) {
  std::vector<std::string> tokens;
  std::string cur;
  for (char c : text) {
    const auto uc = static_cast<unsigned char>(c);
    if (std::isalnum(uc)) {
      cur.push_back(static_cast<char>(std::tolower(uc)));
    } else if (!cur.empty()) {
      tokens.push_back(cur);
      cur.clear();
    }
  }
  if (!cur.empty()) tokens.push_back(std::move(cur));
  return tokens;
}

pluss::Series bool_series_from_mask(const std::vector<bool>& keep) {
  const auto n = static_cast<std::int64_t>(keep.size());
  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < n; ++i) p[i] = keep[static_cast<std::size_t>(i)];
  return mask;
}

std::vector<std::int64_t> sort_indices_for_keys(const std::vector<Stream>& key_streams,
                                                const std::vector<bool>& ascending) {
  PLUSS_CHECK(!key_streams.empty(), "Catalog::sort: no key streams provided");
  const auto n = key_streams.front().size();
  for (const auto& s : key_streams) {
    PLUSS_CHECK(s.size() == n, "Catalog::sort: all key streams must have same length");
  }

  std::vector<pluss::table::Slate> key_slates;
  key_slates.reserve(key_streams.size());
  for (const auto& s : key_streams) key_slates.push_back(s.to_slate());

  std::vector<std::int64_t> idx(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) idx[static_cast<std::size_t>(i)] = i;

  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    for (std::size_t k = 0; k < key_slates.size(); ++k) {
      const int cmp = pluss::table::detail::compare_rows(key_slates[k], a, b, true);
      if (cmp == 0) continue;
      const bool asc = k < ascending.size() ? ascending[k] : true;
      return asc ? (cmp < 0) : (cmp > 0);
    }
    return a < b;
  });

  return idx;
}

std::int64_t find_anchor_idx(const std::vector<std::string>& keys,
                             std::string_view anchor,
                             const char* where) {
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (keys[i] == anchor) return static_cast<std::int64_t>(i);
  }
  pluss::codem::throw_error(pluss::tils::OutputEngine::format("{}: anchor key '{}' not found", where, anchor));
  return 0;
}

std::int64_t resolve_anchor_idx(const std::vector<std::string>& keys,
                                const TabelizePolicy& policy) {
  if (keys.empty()) return 0;
  if (policy.anchor.has_value()) {
    return find_anchor_idx(keys, *policy.anchor, "Catalog::tabelize");
  }
  if (policy.align == Align::Right) {
    return static_cast<std::int64_t>(keys.size() - 1);
  }
  return 0;
}

std::vector<bool> validity_or_all_true(const Stream& stream) {
  std::vector<bool> valid(static_cast<std::size_t>(stream.size()), true);
  if (!detail::has_validity_mask(stream)) return valid;
  return detail::validity_mask(stream)->host<bool>();
}

std::optional<pluss::Series> optional_mask_from_valid(const std::vector<bool>& valid) {
  if (valid.empty()) return std::nullopt;
  bool all_valid = true;
  for (bool v : valid) {
    all_valid = all_valid && v;
  }
  if (all_valid) return std::nullopt;

  const auto n = static_cast<std::int64_t>(valid.size());
  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < n; ++i) p[i] = valid[static_cast<std::size_t>(i)];
  return mask;
}

std::vector<std::string> labels_from_stream(const Stream& stream) {
  PLUSS_CHECK(stream.has_label(), "Catalog::tabelize(by_label): stream has no labels");
  auto as_note = stream.label()->cast().note().as_note();
  std::vector<std::string> labels;
  labels.reserve(static_cast<std::size_t>(stream.size()));
  for (std::int64_t i = 0; i < stream.size(); ++i) {
    labels.emplace_back(as_note.at(i));
  }
  return labels;
}

std::unordered_map<std::string, std::int64_t> make_unique_label_to_idx(const std::vector<std::string>& labels) {
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(labels.size() * 2);
  for (std::size_t i = 0; i < labels.size(); ++i) {
    const auto [it, inserted] = out.emplace(labels[i], static_cast<std::int64_t>(i));
    PLUSS_CHECK(inserted,
                "Catalog::tabelize(by_label): duplicate labels are not supported");
    (void)it;
  }
  return out;
}

std::vector<std::int64_t> positional_mapping(std::int64_t stream_size, std::int64_t target_len) {
  std::vector<std::int64_t> map(static_cast<std::size_t>(target_len), -1);
  for (std::int64_t i = 0; i < target_len; ++i) {
    if (i < stream_size) map[static_cast<std::size_t>(i)] = i;
  }
  return map;
}

std::int64_t positional_target_len(const std::vector<Stream>& streams,
                                   std::int64_t anchor_idx,
                                   Align align) {
  if (streams.empty()) return 0;
  std::int64_t min_len = std::numeric_limits<std::int64_t>::max();
  std::int64_t max_len = 0;
  for (const auto& s : streams) {
    min_len = std::min(min_len, s.size());
    max_len = std::max(max_len, s.size());
  }
  switch (align) {
    case Align::Strict:
      PLUSS_CHECK(min_len == max_len,
                  "Catalog::tabelize(strict): stream lengths differ; use inner/outer/left/right");
      return min_len;
    case Align::Inner:
      return min_len;
    case Align::Outer:
      return max_len;
    case Align::Left:
    case Align::Right:
      return streams[static_cast<std::size_t>(anchor_idx)].size();
  }
  return min_len;
}

pluss::table::Slate apply_fill_policy(pluss::table::Slate slate,
                                       StreamKind kind,
                                       const TabelizePolicy& policy) {
  switch (policy.fill) {
    case FillPolicy::Null:
      return slate;
    case FillPolicy::Scalar:
      PLUSS_CHECK(kind == StreamKind::Series,
                  "Catalog::tabelize(fill=scalar): supported only for Series streams");
      PLUSS_CHECK(policy.fill_scalar.has_value(),
                  "Catalog::tabelize(fill=scalar): fill_scalar must be provided");
      return slate.fillnull(*policy.fill_scalar);
    case FillPolicy::ForwardFill:
      return slate.ffill();
    case FillPolicy::BackwardFill:
      return slate.bfill();
  }
  return slate;
}

pluss::table::Slate build_aligned_slate(const Stream& stream,
                                          const std::vector<std::int64_t>& source_index,
                                          const TabelizePolicy& policy) {
  const auto target_len = static_cast<std::int64_t>(source_index.size());
  const auto source = stream.to_slate();
  const auto source_note = source.cast().note().as_note();
  const auto source_valid = validity_or_all_true(stream);

  std::vector<std::string> values(static_cast<std::size_t>(target_len));
  std::vector<bool> valid(static_cast<std::size_t>(target_len), false);
  for (std::int64_t out_i = 0; out_i < target_len; ++out_i) {
    const auto src_i = source_index[static_cast<std::size_t>(out_i)];
    if (src_i < 0) continue;
    PLUSS_CHECK(src_i < stream.size(), "Catalog::tabelize: source index out of bounds");
    values[static_cast<std::size_t>(out_i)] = std::string(source_note.at(src_i));
    valid[static_cast<std::size_t>(out_i)] = source_valid[static_cast<std::size_t>(src_i)];
  }

  const std::array<std::int64_t, 1> shape{target_len};
  auto note = pluss::make_note(values, shape);
  pluss::table::Slate out_note(note, optional_mask_from_valid(valid));

  pluss::table::Slate out{};
  try {
    switch (stream.kind()) {
      case StreamKind::Series:
        out = out_note.cast({.series_dtype = stream.as_series().scalar_type()}).series();
        break;
      case StreamKind::Note:
        out = out_note;
        break;
      case StreamKind::Timeline:
        out = out_note.cast().timeline();
        break;
      case StreamKind::Duration: {
        const auto source_values = stream.as_duration().host();
        std::vector<std::int64_t> duration_values(
            static_cast<std::size_t>(target_len), 0);
        for (std::int64_t out_i = 0; out_i < target_len; ++out_i) {
          const auto src_i = source_index[static_cast<std::size_t>(out_i)];
          if (src_i >= 0) {
            duration_values[static_cast<std::size_t>(out_i)] =
                source_values[static_cast<std::size_t>(src_i)];
          }
        }
        out = pluss::table::Slate(
            pluss::duration::nanos(std::move(duration_values), {target_len}),
            optional_mask_from_valid(valid));
        break;
      }
    }
  } catch (const std::exception&) {
    if (policy.dtype == DTypePolicy::BestEffort && stream.kind() == StreamKind::Series) {
      out = out_note.cast({.series_dtype = pluss::codem::ScalarType::Float64}).series();
    } else {
      throw;
    }
  }

  return apply_fill_policy(std::move(out), stream.kind(), policy);
}

struct AlignmentPlan {
  AlignBy effective_align_by{AlignBy::Pos};
  std::int64_t target_len{0};
  pluss::table::Slate label_axis{};
  std::vector<std::vector<std::int64_t>> source_index_by_stream{};
};

AlignmentPlan build_alignment_plan(const std::vector<std::string>& keys,
                                   const std::vector<Stream>& streams,
                                   const TabelizePolicy& policy) {
  AlignmentPlan plan;
  if (streams.empty()) return plan;

  const auto anchor_idx = resolve_anchor_idx(keys, policy);
  plan.effective_align_by = policy.align_by;
  if (plan.effective_align_by == AlignBy::Label) {
    const bool all_have_labels =
        std::all_of(streams.begin(), streams.end(), [](const Stream& s) { return s.has_label(); });
    if (!all_have_labels) {
      plan.effective_align_by = AlignBy::Pos; // explicit fallback: unlabeled streams cannot label-align
    }
  }

  plan.source_index_by_stream.reserve(streams.size());
  std::vector<std::string> target_labels;

  if (plan.effective_align_by == AlignBy::Pos) {
    plan.target_len = positional_target_len(streams, anchor_idx, policy.align);
    for (const auto& s : streams) {
      plan.source_index_by_stream.push_back(positional_mapping(s.size(), plan.target_len));
    }

    std::vector<std::int64_t> idx(static_cast<std::size_t>(plan.target_len));
    for (std::int64_t i = 0; i < plan.target_len; ++i) idx[static_cast<std::size_t>(i)] = i;
    plan.label_axis = pluss::table::Slate(pluss::make_series(idx, {plan.target_len}));
    return plan;
  }

  std::vector<std::vector<std::string>> labels;
  labels.reserve(streams.size());
  std::vector<std::unordered_map<std::string, std::int64_t>> label_to_idx;
  label_to_idx.reserve(streams.size());
  for (const auto& s : streams) {
    labels.push_back(labels_from_stream(s));
    label_to_idx.push_back(make_unique_label_to_idx(labels.back()));
  }

  const auto& anchor_labels = labels[static_cast<std::size_t>(anchor_idx)];
  switch (policy.align) {
    case Align::Strict: {
      for (const auto& label_set : labels) {
        PLUSS_CHECK(label_set == anchor_labels,
                    "Catalog::tabelize(strict, label): label order mismatch");
      }
      target_labels = anchor_labels;
      break;
    }
    case Align::Inner: {
      for (const auto& lbl : anchor_labels) {
        bool in_all = true;
        for (const auto& lut : label_to_idx) {
          in_all = in_all && (lut.find(lbl) != lut.end());
        }
        if (in_all) target_labels.push_back(lbl);
      }
      break;
    }
    case Align::Left:
    case Align::Right: {
      target_labels = anchor_labels;
      break;
    }
    case Align::Outer: {
      std::unordered_set<std::string> seen;
      target_labels.reserve(anchor_labels.size() * 2);
      for (const auto& lbl : anchor_labels) {
        seen.insert(lbl);
        target_labels.push_back(lbl);
      }
      for (const auto& label_set : labels) {
        for (const auto& lbl : label_set) {
          if (seen.insert(lbl).second) target_labels.push_back(lbl);
        }
      }
      break;
    }
  }

  plan.target_len = static_cast<std::int64_t>(target_labels.size());
  for (const auto& lut : label_to_idx) {
    std::vector<std::int64_t> map(static_cast<std::size_t>(plan.target_len), -1);
    for (std::int64_t i = 0; i < plan.target_len; ++i) {
      const auto it = lut.find(target_labels[static_cast<std::size_t>(i)]);
      if (it != lut.end()) map[static_cast<std::size_t>(i)] = it->second;
    }
    plan.source_index_by_stream.push_back(std::move(map));
  }

  const auto anchor = static_cast<std::size_t>(anchor_idx);
  const auto anchor_label_to_idx = make_unique_label_to_idx(labels_from_stream(streams[anchor]));
  std::vector<std::int64_t> anchor_label_indices;
  anchor_label_indices.reserve(target_labels.size());
  bool all_labels_in_anchor = true;
  for (const auto& label : target_labels) {
    const auto it = anchor_label_to_idx.find(label);
    if (it == anchor_label_to_idx.end()) {
      all_labels_in_anchor = false;
      break;
    }
    anchor_label_indices.push_back(it->second);
  }

  if (all_labels_in_anchor) {
    plan.label_axis = streams[anchor].label()->take(anchor_label_indices);
  } else {
    plan.label_axis = pluss::table::Slate(
        pluss::make_note(target_labels, {plan.target_len}));
  }
  return plan;
}

std::string unique_label_feature_name(std::string base, const std::unordered_set<std::string>& taken) {
  if (taken.find(base) == taken.end()) return base;
  std::string candidate = base + "_label";
  std::int64_t suffix = 1;
  while (taken.find(candidate) != taken.end()) {
    candidate = pluss::tils::OutputEngine::format("{}_{}", base, suffix++);
  }
  return candidate;
}

Catalog catalog_from_raw_batch(const pluss::loader::CsvRawBatch& raw,
                               const pluss::loader::ReadCsvOptions& infer_options,
                               const KindelizePolicy& policy = {}) {
  std::optional<pluss::table::Slate> case_axis = std::nullopt;
  if (raw.cases.has_value()) {
    case_axis = pluss::table::detail::infer_slate_from_raw_csv_column(*raw.cases, infer_options);
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(raw.features.size() + (policy.materialize_label ? 1u : 0u));
  std::unordered_set<std::string> names;
  names.reserve(raw.features.size() * 2);

  for (const auto& col : raw.features) {
    auto feature = pluss::table::detail::infer_slate_from_raw_csv_column(col, infer_options);

    std::optional<pluss::table::Slate> stream_label = std::nullopt;
    if (policy.attach_labels && case_axis.has_value() && case_axis->size() == feature.size()) {
      stream_label = case_axis;
    }

    names.insert(col.name);
    out.emplace_back(col.name, Stream::from_slate(feature, std::move(stream_label)));
  }

  if (policy.materialize_label && case_axis.has_value()) {
    PLUSS_CHECK(names.find(policy.label_key) == names.end(),
                "Catalog::kindelize: label key collides with existing feature '" + policy.label_key + "'");
    out.emplace_back(policy.label_key, Stream::from_slate(*case_axis));
  }

  return Catalog(std::move(out));
}

Catalog catalog_from_batch(pluss::loader::CsvBatch batch, const KindelizePolicy& policy = {}) {
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(batch.features.size() + (policy.materialize_label ? 1u : 0u));

  const auto case_axis = batch.cases;
  std::unordered_set<std::string> names;
  names.reserve(batch.features.size() * 2);
  for (auto& [name, feature] : batch.features) {
    auto s = policy.attach_labels
        ? Stream::from_slate(feature, case_axis)
        : Stream::from_slate(feature);
    names.insert(name);
    out.emplace_back(std::move(name), std::move(s));
  }

  if (policy.materialize_label && case_axis.has_value()) {
    PLUSS_CHECK(names.find(policy.label_key) == names.end(),
                "Catalog::kindelize: label key collides with existing feature '" + policy.label_key + "'");
    out.emplace_back(policy.label_key, Stream::from_slate(*case_axis));
  }

  return Catalog(std::move(out));
}

} // namespace

namespace detail {
class CatalogImpl final : public pluss::msharp::RefCounted {
public:
  CatalogImpl() = default;

  explicit CatalogImpl(std::vector<std::pair<std::string, Stream>> streams) {
    keys_.reserve(streams.size());
    streams_.reserve(streams.size());
    for (auto& [k, s] : streams) {
      PLUSS_CHECK(!k.empty(), "Catalog: key cannot be empty");
      PLUSS_CHECK(s.defined(), "Catalog: stream is undefined");
      PLUSS_CHECK(key_to_pos_.find(k) == key_to_pos_.end(), "Catalog: duplicate key '" + k + "'");
      const auto idx = static_cast<std::int64_t>(keys_.size());
      key_to_pos_.emplace(k, idx);
      keys_.push_back(std::move(k));
      streams_.push_back(std::move(s));
    }
  }

  std::vector<std::string> keys_{};
  std::vector<Stream> streams_{};
  std::unordered_map<std::string, std::int64_t> key_to_pos_{};
};
} // namespace detail


#include "kindle/ops/catalog/CatalogLifecycleAndIO.inc"
#include "kindle/ops/catalog/CatalogCoreOps.inc"
#include "kindle/ops/catalog/CatalogReduce.inc"
#include "kindle/ops/catalog/CatalogJoin.inc"
#include "kindle/ops/catalog/CatalogPersist.inc"
#include "kindle/ops/catalog/CatalogAccessors.inc"
#include "kindle/ops/catalog/CatalogTextAndMutation.inc"
#include "kindle/ops/catalog/CatalogTabelizeAndInternals.inc"

} // namespace pluss::kindle
