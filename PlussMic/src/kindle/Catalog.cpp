#include "kindle/catalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <limits>
#include <cmath>
#include <iostream>
#include <ostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/Errors.h"
#include "loader/Config.h"
#include "loader/Loader.h"
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "kindle/display/Display.h"
#include "table/DataFrame.h"
#include "table/io/CsvSchemaConvert.h"

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

std::unordered_set<std::string> normalize_name_set(std::initializer_list<std::string_view> names) {
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
  PLUSS_CHECK(!key_streams.empty(), "Catalog::sort_by: no key streams provided");
  const auto n = key_streams.front().size();
  for (const auto& s : key_streams) {
    PLUSS_CHECK(s.size() == n, "Catalog::sort_by: all key streams must have same length");
  }

  std::vector<pluss::table::Thread> key_threads;
  key_threads.reserve(key_streams.size());
  for (const auto& s : key_streams) key_threads.push_back(s.to_thread());

  std::vector<std::int64_t> idx(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) idx[static_cast<std::size_t>(i)] = i;

  std::stable_sort(idx.begin(), idx.end(), [&](std::int64_t a, std::int64_t b) {
    for (std::size_t k = 0; k < key_threads.size(); ++k) {
      const int cmp = key_threads[k].compare_rows(a, b, true);
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
  pluss::codem::throw_error(fmt::format("{}: anchor key '{}' not found", where, anchor));
  return 0;
}

std::int64_t resolve_anchor_idx(const std::vector<std::string>& keys,
                                const TabelizePolicy& policy) {
  if (keys.empty()) return 0;
  if (policy.anchor.has_value()) {
    return find_anchor_idx(keys, *policy.anchor, "Catalog::tabelize");
  }
  if (policy.align == AlignPolicy::Right) {
    return static_cast<std::int64_t>(keys.size() - 1);
  }
  return 0;
}

std::vector<bool> validity_or_all_true(const Stream& stream) {
  std::vector<bool> valid(static_cast<std::size_t>(stream.size()), true);
  if (!stream.has_validity_mask()) return valid;
  return stream.validity_mask()->to_vec<bool>();
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

std::vector<std::string> case_labels_from_stream(const Stream& stream) {
  PLUSS_CHECK(stream.has_cases(), "Catalog::tabelize(by_case_label): stream has no case labels");
  auto as_note = stream.cases()->cast(pluss::table::ThreadCastTarget::Note).as_note();
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
                "Catalog::tabelize(by_case_label): duplicate case labels are not supported");
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
                                   AlignPolicy align) {
  if (streams.empty()) return 0;
  std::int64_t min_len = std::numeric_limits<std::int64_t>::max();
  std::int64_t max_len = 0;
  for (const auto& s : streams) {
    min_len = std::min(min_len, s.size());
    max_len = std::max(max_len, s.size());
  }
  switch (align) {
    case AlignPolicy::Strict:
      PLUSS_CHECK(min_len == max_len,
                  "Catalog::tabelize(strict): stream lengths differ; use inner/outer/left/right");
      return min_len;
    case AlignPolicy::Inner:
      return min_len;
    case AlignPolicy::Outer:
      return max_len;
    case AlignPolicy::Left:
    case AlignPolicy::Right:
      return streams[static_cast<std::size_t>(anchor_idx)].size();
  }
  return min_len;
}

pluss::table::Thread apply_fill_policy(pluss::table::Thread thread,
                                       StreamKind kind,
                                       const TabelizePolicy& policy) {
  switch (policy.fill) {
    case FillPolicy::Null:
      return thread;
    case FillPolicy::Scalar:
      PLUSS_CHECK(kind == StreamKind::Series,
                  "Catalog::tabelize(fill=scalar): supported only for Series streams");
      PLUSS_CHECK(policy.fill_scalar.has_value(),
                  "Catalog::tabelize(fill=scalar): fill_scalar must be provided");
      return thread.fillna(*policy.fill_scalar);
    case FillPolicy::ForwardFill:
      return thread.ffill();
    case FillPolicy::BackwardFill:
      return thread.bfill();
  }
  return thread;
}

pluss::table::Thread build_aligned_thread(const Stream& stream,
                                          const std::vector<std::int64_t>& source_index,
                                          const TabelizePolicy& policy) {
  const auto target_len = static_cast<std::int64_t>(source_index.size());
  const auto source = stream.to_thread();
  const auto source_note = source.cast(pluss::table::ThreadCastTarget::Note).as_note();
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
  pluss::table::Thread out_note(note, optional_mask_from_valid(valid));

  pluss::table::Thread out{};
  try {
    switch (stream.kind()) {
      case StreamKind::Series:
        out = out_note.cast(pluss::table::ThreadCastTarget::Series, stream.as_series().scalar_type());
        break;
      case StreamKind::Note:
        out = out_note;
        break;
      case StreamKind::Timeline:
        out = out_note.cast(pluss::table::ThreadCastTarget::Timeline);
        break;
      case StreamKind::TimeIndex:
        out = out_note.cast(pluss::table::ThreadCastTarget::TimeIndex);
        break;
    }
  } catch (const std::exception&) {
    if (policy.dtype == DTypePolicy::BestEffort && stream.kind() == StreamKind::Series) {
      out = out_note.cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Float64);
    } else {
      throw;
    }
  }

  return apply_fill_policy(std::move(out), stream.kind(), policy);
}

std::string unique_case_feature_name(std::string base, const std::unordered_set<std::string>& taken) {
  if (taken.find(base) == taken.end()) return base;
  std::string candidate = base + "_case";
  std::int64_t suffix = 1;
  while (taken.find(candidate) != taken.end()) {
    candidate = fmt::format("{}_{}", base, suffix++);
  }
  return candidate;
}

Catalog catalog_from_raw_batch(const pluss::loader::CsvRawBatch& raw,
                               const pluss::loader::ReadCsvOptions& infer_options,
                               const KindelizePolicy& policy = {}) {
  std::optional<pluss::table::Thread> case_axis = std::nullopt;
  if (raw.cases.has_value()) {
    case_axis = pluss::table::detail::infer_thread_from_raw_csv_column(*raw.cases, infer_options);
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(raw.features.size() + (policy.materialize_case_stream ? 1u : 0u));
  std::unordered_set<std::string> names;
  names.reserve(raw.features.size() * 2);

  for (const auto& col : raw.features) {
    auto feature = pluss::table::detail::infer_thread_from_raw_csv_column(col, infer_options);

    std::optional<pluss::table::Thread> stream_cases = std::nullopt;
    if (policy.attach_cases_to_streams && case_axis.has_value() && case_axis->size() == feature.size()) {
      stream_cases = case_axis;
    }

    names.insert(col.name);
    out.emplace_back(col.name, Stream::from_thread(feature, std::move(stream_cases)));
  }

  if (policy.materialize_case_stream && case_axis.has_value()) {
    PLUSS_CHECK(names.find(policy.case_stream_name) == names.end(),
                "Catalog::kindelize: case stream key collides with existing feature '" + policy.case_stream_name + "'");
    out.emplace_back(policy.case_stream_name, Stream::from_thread(*case_axis));
  }

  return Catalog(std::move(out));
}

Catalog catalog_from_batch(pluss::loader::CsvBatch batch, const KindelizePolicy& policy = {}) {
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(batch.features.size() + (policy.materialize_case_stream ? 1u : 0u));

  const auto case_axis = batch.cases;
  std::unordered_set<std::string> names;
  names.reserve(batch.features.size() * 2);
  for (auto& [name, feature] : batch.features) {
    auto s = policy.attach_cases_to_streams
        ? Stream::from_thread(feature, case_axis)
        : Stream::from_thread(feature);
    names.insert(name);
    out.emplace_back(std::move(name), std::move(s));
  }

  if (policy.materialize_case_stream && case_axis.has_value()) {
    PLUSS_CHECK(names.find(policy.case_stream_name) == names.end(),
                "Catalog::kindelize: case stream key collides with existing feature '" + policy.case_stream_name + "'");
    out.emplace_back(policy.case_stream_name, Stream::from_thread(*case_axis));
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
#include "kindle/ops/catalog/CatalogTextAndMutation.inc"
#include "kindle/ops/catalog/CatalogTabelizeAndInternals.inc"

} // namespace pluss::kindle
