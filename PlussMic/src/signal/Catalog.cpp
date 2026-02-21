#include "signal/catalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <limits>
#include <cmath>
#include <iostream>
#include <ostream>
#include <numeric>
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
#include "pluss/codem/series_factory.h"
#include "pluss/coden/note_factory.h"
#include "signal/display/Display.h"
#include "table/DataFrame.h"

namespace pluss::signal {

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

Catalog::Catalog() = default;
Catalog::Catalog(std::vector<std::pair<std::string, Stream>> streams)
    : impl_(pluss::msharp::make_intrusive<detail::CatalogImpl>(std::move(streams))) {}
Catalog::Catalog(std::initializer_list<std::pair<std::string, Stream>> streams)
    : Catalog(std::vector<std::pair<std::string, Stream>>(streams)) {}
Catalog::Catalog(std::initializer_list<NamedStream> streams) {
  std::vector<std::pair<std::string, Stream>> items;
  items.reserve(streams.size());
  for (const auto& kv : streams) {
    items.emplace_back(std::string(kv.key), kv.value);
  }
  impl_ = pluss::msharp::make_intrusive<detail::CatalogImpl>(std::move(items));
}
Catalog::Catalog(const Catalog& other) = default;
Catalog::Catalog(Catalog&& other) noexcept = default;
Catalog& Catalog::operator=(const Catalog& other) = default;
Catalog& Catalog::operator=(Catalog&& other) noexcept = default;
Catalog::~Catalog() = default;

Catalog::Catalog(pluss::msharp::intrusive_ptr<detail::CatalogImpl> impl) : impl_(std::move(impl)) {}

Catalog Catalog::signalize(const pluss::table::DataFrame& frame, const SignalizePolicy& policy) {
  PLUSS_CHECK(frame.defined(), "Catalog::signalize requires a defined DataFrame");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(frame.nfeatures()) + (policy.materialize_case_stream ? 1u : 0u));

  const auto case_axis = frame.cases();
  for (const auto& key : frame.feature_names()) {
    auto s = Stream::from_thread(frame.features(key),
                                 policy.attach_cases_to_streams ? std::optional<pluss::table::Thread>{case_axis}
                                                                 : std::nullopt);
    out.emplace_back(key, std::move(s));
  }

  if (policy.materialize_case_stream) {
    PLUSS_CHECK(frame.feature_names().end() ==
                    std::find(frame.feature_names().begin(), frame.feature_names().end(), policy.case_stream_name),
                "Catalog::signalize: case stream key collides with existing feature '" + policy.case_stream_name + "'");
    out.emplace_back(policy.case_stream_name, Stream::from_thread(case_axis));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::load(std::string_view source_type, std::string_view filepath) {
  return signalize(pluss::table::DataFrame::load(source_type, filepath));
}

Catalog Catalog::load(std::string_view source_type,
                      std::string_view filepath,
                      const pluss::loader::Config& config) {
  return signalize(pluss::table::DataFrame::load(source_type, filepath, config));
}

Catalog Catalog::load_csv(std::string_view filepath) {
  return signalize(pluss::table::DataFrame::load_csv(filepath));
}

Catalog Catalog::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return signalize(pluss::table::DataFrame::load_csv(filepath, options));
}

Catalog Catalog::load_excel(std::string_view filepath) {
  return signalize(pluss::table::DataFrame::load_excel(filepath));
}

Catalog Catalog::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  return signalize(pluss::table::DataFrame::load_excel(filepath, options));
}

Catalog Catalog::load_csv_with_key(std::string_view filepath,
                                   std::string_view key_field,
                                   const pluss::loader::ReadCsvOptions& options) {
  auto frame = pluss::table::DataFrame::load_csv(filepath, options);
  const auto key = trim_copy(key_field);
  PLUSS_CHECK(!key.empty(), "Catalog::load_csv_with_key: key field cannot be empty");
  PLUSS_CHECK(frame.has_feature(key),
              "Catalog::load_csv_with_key: key field not found '" + key + "'");
  frame = frame.set_cases(key, false);
  SignalizePolicy policy;
  policy.attach_cases_to_streams = true;
  policy.materialize_case_stream = true;
  policy.case_stream_name = "_case";
  return signalize(frame, policy);
}

Catalog Catalog::from_json(std::string_view json_text) {
  (void)json_text;
  PLUSS_FAIL("Catalog::from_json: not implemented yet");
}

bool Catalog::defined() const noexcept { return static_cast<bool>(impl_); }
bool Catalog::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Catalog::nstreams() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().streams_.size());
}

bool Catalog::empty() const { return nstreams() == 0; }

const std::vector<std::string>& Catalog::keys() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return impl_ref_().keys_;
}

bool Catalog::has(std::string_view key) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return impl_ref_().key_to_pos_.find(std::string(key)) != impl_ref_().key_to_pos_.end();
}

Stream Catalog::stream(std::string_view key) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto it = impl_ref_().key_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != impl_ref_().key_to_pos_.end(), "Catalog: key '" + std::string(key) + "' not found");
  return impl_ref_().streams_[static_cast<std::size_t>(it->second)];
}

std::optional<Stream> Catalog::get(std::string_view key) const {
  if (!defined()) return std::nullopt;
  auto it = impl_ref_().key_to_pos_.find(std::string(key));
  if (it == impl_ref_().key_to_pos_.end()) return std::nullopt;
  return impl_ref_().streams_[static_cast<std::size_t>(it->second)];
}

std::vector<Stream> Catalog::streams() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return impl_ref_().streams_;
}

Catalog Catalog::series() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    if (impl_ref_().streams_[i].kind() == StreamKind::Series) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i]);
    }
  }
  return Catalog(std::move(out));
}

Catalog Catalog::note() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    if (impl_ref_().streams_[i].kind() == StreamKind::Note) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i]);
    }
  }
  return Catalog(std::move(out));
}

Catalog Catalog::timeline() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto k = impl_ref_().streams_[i].kind();
    if (k == StreamKind::Timeline || k == StreamKind::TimeIndex) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i]);
    }
  }
  return Catalog(std::move(out));
}

Catalog Catalog::timeindex() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    if (impl_ref_().streams_[i].kind() == StreamKind::TimeIndex) {
      out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i]);
    }
  }
  return Catalog(std::move(out));
}

Catalog Catalog::head(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].head(n));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::tail(std::int64_t n) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].tail(n));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::slice(std::int64_t start, std::int64_t stop) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].slice(start, stop));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::iloc(std::int64_t start, std::int64_t stop) const {
  return slice(start, stop);
}

Catalog Catalog::filter(const pluss::Series& condition) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(condition.defined(), "Catalog::filter: condition is undefined");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& s = impl_ref_().streams_[i];
    PLUSS_CHECK(s.size() == condition.numel(),
                "Catalog::filter: mask length must match stream length for key '" + impl_ref_().keys_[i] + "'");
    out.emplace_back(impl_ref_().keys_[i], s.filter(condition));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::masked_select(const pluss::Series& mask) const {
  return filter(mask);
}

Catalog Catalog::select(const std::vector<std::string>& field_names) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto names = normalize_names(field_names);
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Catalog::select: unknown key '" + n + "'");
    out.emplace_back(n, stream(n));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::drop(const std::vector<std::string>& field_names) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto drop_set = normalize_name_set(field_names);
  for (const auto& n : drop_set) {
    PLUSS_CHECK(has(n), "Catalog::drop: unknown key '" + n + "'");
  }
  std::vector<std::pair<std::string, Stream>> kept;
  kept.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& k = impl_ref_().keys_[i];
    if (drop_set.find(k) == drop_set.end()) kept.emplace_back(k, impl_ref_().streams_[i]);
  }
  return Catalog(std::move(kept));
}

Catalog Catalog::rename(const std::unordered_map<std::string, std::string>& mapping) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (mapping.empty()) return *this;

  std::unordered_set<std::string> target_names;
  target_names.reserve(static_cast<std::size_t>(nstreams()) * 2);
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& old_key = impl_ref_().keys_[i];
    auto it = mapping.find(old_key);
    std::string new_key = (it == mapping.end()) ? old_key : trim_copy(it->second);
    PLUSS_CHECK(!new_key.empty(), "Catalog::rename: new key cannot be empty");
    PLUSS_CHECK(target_names.insert(new_key).second, "Catalog::rename: duplicate target key '" + new_key + "'");
    out.emplace_back(std::move(new_key), impl_ref_().streams_[i]);
  }

  for (const auto& [old_key, _] : mapping) {
    PLUSS_CHECK(has(old_key), "Catalog::rename: unknown key '" + old_key + "'");
  }
  return Catalog(std::move(out));
}

Catalog Catalog::sort(std::string_view by_field, bool ascending) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(by_field);
  PLUSS_CHECK(!key.empty(), "Catalog::sort: by_field cannot be empty");
  return sort_by({key}, {ascending});
}

Catalog Catalog::sort_by(const std::vector<std::string>& fields,
                         const std::vector<bool>& ascending) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto names = normalize_names(fields);
  PLUSS_CHECK(!names.empty(), "Catalog::sort_by: at least one field is required");
  PLUSS_CHECK(ascending.empty() || ascending.size() == names.size(),
              "Catalog::sort_by: ascending size must be 0 or equal to fields size");

  std::vector<Stream> key_streams;
  key_streams.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Catalog::sort_by: unknown key '" + n + "'");
    key_streams.push_back(stream(n));
  }
  const auto idx = sort_indices_for_keys(key_streams, ascending);

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& s = impl_ref_().streams_[i];
    PLUSS_CHECK(s.size() == static_cast<std::int64_t>(idx.size()),
                "Catalog::sort_by: sort fields require aligned stream lengths");
    out.emplace_back(impl_ref_().keys_[i], s.take(idx));
  }
  return Catalog(std::move(out));
}

std::unordered_map<std::string, Catalog> Catalog::group_by(std::string_view key_field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(key_field);
  PLUSS_CHECK(!key.empty(), "Catalog::group_by: key field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::group_by: unknown key '" + key + "'");

  const auto key_stream = stream(key);
  const auto key_note = key_stream.to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();
  const auto n = key_stream.size();

  std::unordered_map<std::string, std::vector<std::int64_t>> row_groups;
  row_groups.reserve(static_cast<std::size_t>(std::max<std::int64_t>(1, n / 8)));
  for (std::int64_t i = 0; i < n; ++i) {
    row_groups[std::string(key_note.at(i))].push_back(i);
  }

  std::unordered_map<std::string, Catalog> out;
  out.reserve(row_groups.size());
  for (const auto& [group_key, rows] : row_groups) {
    std::vector<std::pair<std::string, Stream>> gcols;
    gcols.reserve(static_cast<std::size_t>(nstreams()));
    for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
      const auto& s = impl_ref_().streams_[i];
      PLUSS_CHECK(s.size() == n, "Catalog::group_by: key field requires aligned stream lengths");
      gcols.emplace_back(impl_ref_().keys_[i], s.take(rows));
    }
    out.emplace(group_key, Catalog(std::move(gcols)));
  }
  return out;
}

Catalog Catalog::fillna(double value, std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::fillna: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() ? (stream.kind() == StreamKind::Series)
                                          : (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.fillna(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::dropna(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::dropna: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.dropna() : stream);
  }
  return Catalog(std::move(out));
}

Stream Catalog::sum(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return stream(field).sum();
}

Stream Catalog::mean(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return stream(field).mean();
}

Stream Catalog::min(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return stream(field).min();
}

Stream Catalog::max(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return stream(field).max();
}

std::int64_t Catalog::count() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (empty()) return 0;
  return tabelize().nrows();
}

Catalog Catalog::merge(const Catalog& other, std::string_view on, std::string_view how) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(other.defined(), "Catalog::merge: other catalog is undefined");
  const auto how_norm = lowercase_copy(trim_copy(how));
  PLUSS_CHECK(how_norm == "inner",
              "Catalog::merge currently supports only how='inner'");
  const auto key = trim_copy(on);
  PLUSS_CHECK(!key.empty(), "Catalog::merge: key cannot be empty");

  const auto left = tabelize();
  const auto right = other.tabelize();
  PLUSS_CHECK(left.has_feature(key), "Catalog::merge: left key not found '" + key + "'");
  PLUSS_CHECK(right.has_feature(key), "Catalog::merge: right key not found '" + key + "'");
  return signalize(left.stitch_on(key, right, key));
}

Catalog Catalog::apply(std::string_view field, const std::function<Stream(const Stream&)>& func) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(static_cast<bool>(func), "Catalog::apply: func is empty");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::apply: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::apply: unknown key '" + key + "'");
  auto out = *this;
  out.apply_(key, func);
  return out;
}

Catalog& Catalog::apply_(std::string_view field, const std::function<Stream(const Stream&)>& func) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(static_cast<bool>(func), "Catalog::apply_: func is empty");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::apply_: field cannot be empty");
  const auto src = stream(key);
  const auto dst = func(src);
  PLUSS_CHECK(dst.defined(), "Catalog::apply_: function returned undefined stream");
  PLUSS_CHECK(dst.size() == src.size(),
              "Catalog::apply_: output stream length must match input stream length");
  return set_stream_(key, dst);
}

Catalog Catalog::text_preprocess(std::string_view field) const {
  return apply(field, [](const Stream& s) {
    const auto t = s.to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();
    std::vector<std::string> out;
    out.reserve(static_cast<std::size_t>(s.size()));
    for (std::int64_t i = 0; i < s.size(); ++i) {
      auto tokens = tokenize_ascii_words(t.at(i));
      std::string joined;
      for (std::size_t j = 0; j < tokens.size(); ++j) {
        if (j) joined.push_back(' ');
        joined += tokens[j];
      }
      out.push_back(std::move(joined));
    }
    return Stream(pluss::make_note(out), s.validity_mask(), s.cases());
  });
}

Catalog Catalog::extract_keywords(std::string_view field, int top_n) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::extract_keywords: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::extract_keywords: unknown key '" + key + "'");
  const auto src = stream(key).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();

  std::unordered_map<std::string, std::int64_t> counts;
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    for (const auto& tok : tokenize_ascii_words(src.at(i))) {
      if (!tok.empty()) counts[tok] += 1;
    }
  }

  std::vector<std::pair<std::string, std::int64_t>> items(counts.begin(), counts.end());
  std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    return a.first < b.first;
  });

  const auto limit = std::max<std::int64_t>(0, static_cast<std::int64_t>(top_n));
  if (limit > 0 && static_cast<std::int64_t>(items.size()) > limit) {
    items.resize(static_cast<std::size_t>(limit));
  }

  std::vector<std::string> words;
  std::vector<std::int64_t> freqs;
  words.reserve(items.size());
  freqs.reserve(items.size());
  for (const auto& [w, c] : items) {
    words.push_back(w);
    freqs.push_back(c);
  }

  std::vector<std::pair<std::string, Stream>> out_cols;
  out_cols.reserve(2);
  out_cols.emplace_back("keyword", Stream(pluss::make_note(words)));
  out_cols.emplace_back("count",
                        Stream(pluss::make_series(freqs, {static_cast<std::int64_t>(freqs.size())})));
  return Catalog(std::move(out_cols));
}

Catalog Catalog::compute_similarity(std::string_view field1,
                                    std::string_view field2,
                                    std::string_view method) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto left_name = trim_copy(field1);
  const auto right_name = trim_copy(field2);
  PLUSS_CHECK(!left_name.empty() && !right_name.empty(),
              "Catalog::compute_similarity: field names cannot be empty");
  PLUSS_CHECK(has(left_name), "Catalog::compute_similarity: unknown field '" + left_name + "'");
  PLUSS_CHECK(has(right_name), "Catalog::compute_similarity: unknown field '" + right_name + "'");
  PLUSS_CHECK(lowercase_copy(trim_copy(method)) == "cosine",
              "Catalog::compute_similarity currently supports only method='cosine'");

  const auto lhs = stream(left_name).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();
  const auto rhs = stream(right_name).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();
  PLUSS_CHECK(lhs.numel() == rhs.numel(), "Catalog::compute_similarity: fields must have same length");

  std::vector<double> sim;
  sim.reserve(static_cast<std::size_t>(lhs.numel()));
  for (std::int64_t i = 0; i < lhs.numel(); ++i) {
    std::unordered_map<std::string, double> lv;
    std::unordered_map<std::string, double> rv;
    for (const auto& tok : tokenize_ascii_words(lhs.at(i))) lv[tok] += 1.0;
    for (const auto& tok : tokenize_ascii_words(rhs.at(i))) rv[tok] += 1.0;

    double dot = 0.0;
    double ln = 0.0;
    double rn = 0.0;
    for (const auto& [w, v] : lv) {
      ln += v * v;
      auto it = rv.find(w);
      if (it != rv.end()) dot += v * it->second;
    }
    for (const auto& [_, v] : rv) rn += v * v;
    const double denom = std::sqrt(ln) * std::sqrt(rn);
    sim.push_back(denom > 0.0 ? dot / denom : 0.0);
  }

  const auto sim_name = left_name + "__" + right_name + "_similarity";
  return set_stream(sim_name, Stream(pluss::make_series(sim, {static_cast<std::int64_t>(sim.size())})));
}

Catalog Catalog::filter_by_text(std::string_view field, std::string_view pattern) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  const auto pat = lowercase_copy(trim_copy(pattern));
  PLUSS_CHECK(!key.empty(), "Catalog::filter_by_text: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::filter_by_text: unknown field '" + key + "'");
  const auto src = stream(key).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();
  std::vector<bool> keep(static_cast<std::size_t>(src.numel()), false);
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    const auto txt = lowercase_copy(src.at(i));
    keep[static_cast<std::size_t>(i)] = (txt.find(pat) != std::string::npos);
  }
  return filter(bool_series_from_mask(keep));
}

Catalog Catalog::add_text_features(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::add_text_features: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::add_text_features: unknown field '" + key + "'");
  const auto src = stream(key).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();

  std::vector<std::int64_t> char_len;
  std::vector<std::int64_t> word_count;
  char_len.reserve(static_cast<std::size_t>(src.numel()));
  word_count.reserve(static_cast<std::size_t>(src.numel()));
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    const auto text = std::string(src.at(i));
    char_len.push_back(static_cast<std::int64_t>(text.size()));
    word_count.push_back(static_cast<std::int64_t>(tokenize_ascii_words(text).size()));
  }

  auto out = *this;
  out = out.set_stream(key + "_char_len",
                       Stream(pluss::make_series(char_len, {static_cast<std::int64_t>(char_len.size())})));
  out = out.set_stream(key + "_word_count",
                       Stream(pluss::make_series(word_count, {static_cast<std::int64_t>(word_count.size())})));
  return out;
}

Catalog Catalog::aggregate_text_stats(std::string_view field, std::string_view stat_type) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  const auto stat = lowercase_copy(trim_copy(stat_type));
  PLUSS_CHECK(!key.empty(), "Catalog::aggregate_text_stats: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::aggregate_text_stats: unknown field '" + key + "'");
  const auto src = stream(key).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();

  std::vector<std::int64_t> lens;
  lens.reserve(static_cast<std::size_t>(src.numel()));
  std::unordered_set<std::string> vocab;
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    const auto text = std::string(src.at(i));
    lens.push_back(static_cast<std::int64_t>(text.size()));
    for (const auto& tok : tokenize_ascii_words(text)) vocab.insert(tok);
  }
  const auto n = static_cast<double>(lens.size());
  const auto minmax = std::minmax_element(lens.begin(), lens.end());
  const auto sum_len = std::accumulate(lens.begin(), lens.end(), 0.0);

  double value = 0.0;
  if (stat == "mean" || stat == "avg") {
    value = n > 0.0 ? (sum_len / n) : 0.0;
  } else if (stat == "min") {
    value = lens.empty() ? 0.0 : static_cast<double>(*minmax.first);
  } else if (stat == "max") {
    value = lens.empty() ? 0.0 : static_cast<double>(*minmax.second);
  } else if (stat == "count") {
    value = n;
  } else if (stat == "vocab" || stat == "vocab_size") {
    value = static_cast<double>(vocab.size());
  } else {
    PLUSS_FAIL("Catalog::aggregate_text_stats: unsupported stat_type '" + stat + "'");
  }

  std::vector<std::pair<std::string, Stream>> out_cols;
  out_cols.reserve(2);
  out_cols.emplace_back("stat", Stream(pluss::make_note(std::vector<std::string>{stat})));
  out_cols.emplace_back("value", Stream(pluss::make_series(std::vector<double>{value}, {1})));
  return Catalog(std::move(out_cols));
}

Catalog Catalog::tfidf(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::tfidf: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::tfidf: unknown field '" + key + "'");
  const auto src = stream(key).to_thread().cast(pluss::table::ThreadCastTarget::Note).as_note();
  const auto n = src.numel();

  std::vector<std::vector<std::string>> docs;
  docs.reserve(static_cast<std::size_t>(n));
  std::unordered_map<std::string, std::int64_t> df;
  for (std::int64_t i = 0; i < n; ++i) {
    auto tok = tokenize_ascii_words(src.at(i));
    docs.push_back(tok);
    std::unordered_set<std::string> seen(tok.begin(), tok.end());
    for (const auto& w : seen) df[w] += 1;
  }

  std::vector<double> norm;
  norm.reserve(static_cast<std::size_t>(n));
  for (const auto& doc : docs) {
    std::unordered_map<std::string, double> tf;
    for (const auto& w : doc) tf[w] += 1.0;
    const double denom_tf = doc.empty() ? 1.0 : static_cast<double>(doc.size());
    double sum_sq = 0.0;
    for (auto& [w, c] : tf) {
      c /= denom_tf;
      const auto it = df.find(w);
      const double idf = std::log((static_cast<double>(n) + 1.0) /
                                  (static_cast<double>(it == df.end() ? 0 : it->second) + 1.0)) + 1.0;
      const double wv = c * idf;
      sum_sq += wv * wv;
    }
    norm.push_back(std::sqrt(sum_sq));
  }

  return set_stream(key + "_tfidf_norm",
                    Stream(pluss::make_series(norm, {static_cast<std::int64_t>(norm.size())})));
}

Catalog Catalog::set_stream(std::string_view key, const Stream& value) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto out = *this;
  out.set_stream_(key, value);
  return out;
}

Catalog& Catalog::set_stream_(std::string_view key, const Stream& value) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(value.defined(), "Catalog::set_stream_: stream is undefined");
  std::string k(key);
  PLUSS_CHECK(!k.empty(), "Catalog::set_stream_: key cannot be empty");

  auto& w = mut_impl_();
  auto it = w.key_to_pos_.find(k);
  if (it == w.key_to_pos_.end()) {
    const auto idx = static_cast<std::int64_t>(w.keys_.size());
    w.key_to_pos_.emplace(k, idx);
    w.keys_.push_back(std::move(k));
    w.streams_.push_back(value);
  } else {
    w.streams_[static_cast<std::size_t>(it->second)] = value;
  }
  return *this;
}

Catalog Catalog::set_streams(std::initializer_list<std::pair<std::string_view, Stream>> items) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto out = *this;
  out.set_streams_(items);
  return out;
}

Catalog& Catalog::set_streams_(std::initializer_list<std::pair<std::string_view, Stream>> items) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  for (const auto& [k, s] : items) {
    set_stream_(k, s);
  }
  return *this;
}

Catalog Catalog::set_streams(std::initializer_list<NamedStream> items) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto out = *this;
  out.set_streams_(items);
  return out;
}

Catalog& Catalog::set_streams_(std::initializer_list<NamedStream> items) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  for (const auto& kv : items) {
    set_stream_(kv.key, kv.value);
  }
  return *this;
}

Catalog Catalog::drop(std::initializer_list<std::string_view> keys_drop) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto drop_keys = normalize_names(keys_drop);
  std::unordered_set<std::string> drop_set(drop_keys.begin(), drop_keys.end());

  std::vector<std::pair<std::string, Stream>> kept;
  kept.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& k = impl_ref_().keys_[i];
    if (drop_set.find(k) == drop_set.end()) {
      kept.emplace_back(k, impl_ref_().streams_[i]);
    }
  }
  return Catalog(std::move(kept));
}

pluss::table::DataFrame Catalog::tabelize(const TabelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (empty()) return pluss::table::DataFrame(std::vector<std::pair<std::string, pluss::table::Thread>>{});

  const auto anchor_idx = resolve_anchor_idx(impl_ref_().keys_, policy);
  auto effective_index_mode = policy.index_mode;
  if (effective_index_mode == IndexMode::ByCaseLabel) {
    const bool all_have_cases = std::all_of(
        impl_ref_().streams_.begin(), impl_ref_().streams_.end(), [](const Stream& s) { return s.has_cases(); });
    if (!all_have_cases) {
      effective_index_mode = IndexMode::Positional; // default fallback policy
    }
  }

  std::int64_t target_len = 0;
  std::vector<std::string> target_labels;
  std::vector<std::vector<std::int64_t>> source_index_by_stream;
  source_index_by_stream.reserve(impl_ref_().streams_.size());

  if (effective_index_mode == IndexMode::Positional) {
    target_len = positional_target_len(impl_ref_().streams_, anchor_idx, policy.align);
    for (const auto& s : impl_ref_().streams_) {
      source_index_by_stream.push_back(positional_mapping(s.size(), target_len));
    }
  } else {
    std::vector<std::vector<std::string>> labels;
    labels.reserve(impl_ref_().streams_.size());
    std::vector<std::unordered_map<std::string, std::int64_t>> label_to_idx;
    label_to_idx.reserve(impl_ref_().streams_.size());
    for (const auto& s : impl_ref_().streams_) {
      labels.push_back(case_labels_from_stream(s));
      label_to_idx.push_back(make_unique_label_to_idx(labels.back()));
    }

    const auto& anchor_labels = labels[static_cast<std::size_t>(anchor_idx)];
    switch (policy.align) {
      case AlignPolicy::Strict: {
        for (const auto& l : labels) {
          PLUSS_CHECK(l == anchor_labels,
                      "Catalog::tabelize(strict, by_case_label): case-label order mismatch");
        }
        target_labels = anchor_labels;
        break;
      }
      case AlignPolicy::Inner: {
        for (const auto& lbl : anchor_labels) {
          bool in_all = true;
          for (const auto& lut : label_to_idx) {
            in_all = in_all && (lut.find(lbl) != lut.end());
          }
          if (in_all) target_labels.push_back(lbl);
        }
        break;
      }
      case AlignPolicy::Left:
      case AlignPolicy::Right: {
        target_labels = anchor_labels;
        break;
      }
      case AlignPolicy::Outer: {
        std::unordered_set<std::string> seen;
        target_labels.reserve(anchor_labels.size() * 2);
        for (const auto& lbl : anchor_labels) {
          seen.insert(lbl);
          target_labels.push_back(lbl);
        }
        for (const auto& l : labels) {
          for (const auto& lbl : l) {
            if (seen.insert(lbl).second) target_labels.push_back(lbl);
          }
        }
        break;
      }
    }

    target_len = static_cast<std::int64_t>(target_labels.size());
    for (const auto& lut : label_to_idx) {
      std::vector<std::int64_t> map(static_cast<std::size_t>(target_len), -1);
      for (std::int64_t i = 0; i < target_len; ++i) {
        auto it = lut.find(target_labels[static_cast<std::size_t>(i)]);
        if (it != lut.end()) map[static_cast<std::size_t>(i)] = it->second;
      }
      source_index_by_stream.push_back(std::move(map));
    }
  }

  pluss::table::Thread case_axis;
  if (effective_index_mode == IndexMode::Positional) {
    std::vector<std::int64_t> idx(static_cast<std::size_t>(target_len));
    for (std::int64_t i = 0; i < target_len; ++i) idx[static_cast<std::size_t>(i)] = i;
    case_axis = pluss::table::Thread(pluss::make_series(idx, {target_len}));
  } else {
    const std::array<std::int64_t, 1> shape{target_len};
    case_axis = pluss::table::Thread(pluss::make_note(target_labels, shape));
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> out_cols;
  out_cols.reserve(impl_ref_().streams_.size());
  for (std::size_t i = 0; i < impl_ref_().streams_.size(); ++i) {
    auto t = build_aligned_thread(impl_ref_().streams_[i], source_index_by_stream[i], policy);
    out_cols.emplace_back(impl_ref_().keys_[i], std::move(t));
  }

  if (policy.include_case_feature) {
    std::unordered_set<std::string> names;
    names.reserve(out_cols.size() * 2);
    for (const auto& c : out_cols) names.insert(c.first);
    std::string case_name = policy.case_feature_name;
    const bool case_name_taken = names.find(case_name) != names.end();
    if (!case_name_taken) {
      out_cols.emplace_back(case_name, case_axis);
    } else {
      switch (policy.collision) {
        case CollisionPolicy::Error:
          pluss::codem::throw_error("Catalog::tabelize: case feature name collision '" + case_name + "'");
          break;
        case CollisionPolicy::Suffix:
          case_name = unique_case_feature_name(case_name, names);
          out_cols.emplace_back(std::move(case_name), case_axis);
          break;
        case CollisionPolicy::Overwrite:
          for (auto& c : out_cols) {
            if (c.first == case_name) {
              c.second = case_axis;
              break;
            }
          }
          break;
      }
    }
  }
  return pluss::table::DataFrame(std::move(out_cols), std::move(case_axis));
}

void Catalog::display() const {
  show(std::cout);
}

void Catalog::info() const {
  tabelize().info();
}

void Catalog::save_csv(std::string_view filename) const {
  tabelize().save_csv(filename);
}

std::string Catalog::to_string() const {
  return display::render(*this);
}

std::string Catalog::to_json() const {
  return display::render(*this);
}

void Catalog::show(std::ostream& os) const { display::print(os, *this); }

Catalog::const_iterator Catalog::begin() const {
  return keys().begin();
}

Catalog::const_iterator Catalog::end() const {
  return keys().end();
}

const detail::CatalogImpl& Catalog::impl_ref_() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return *impl_;
}

detail::CatalogImpl& Catalog::mut_impl_() {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (impl_.use_count() > 1) {
    std::vector<std::pair<std::string, Stream>> copied;
    copied.reserve(impl_ref_().keys_.size());
    for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
      copied.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i]);
    }
    impl_ = pluss::msharp::make_intrusive<detail::CatalogImpl>(std::move(copied));
  }
  return *impl_;
}

std::ostream& operator<<(std::ostream& os, const Catalog& catalog) {
  catalog.show(os);
  return os;
}

} // namespace pluss::signal
