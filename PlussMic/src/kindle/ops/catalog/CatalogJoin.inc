// Catalog-native relational join. Keys are matched as encoded integers (the
// shared relational::match), then every stream is rebuilt through the matched
// index pairs with Stream::gather. No tabelize: ragged catalogs stay ragged.

namespace {

using relational::kNoKey;

void check_join_policy(const RaggedPolicy& p, std::string_view where) {
  p.validate(where);
  PLUSS_CHECK(p.alignment != RaggedAlignment::Label,
              std::string(where) + ": label alignment is not a join mode; join on key columns instead");
  PLUSS_CHECK(p.domain != RaggedDomain::Anchor,
              std::string(where) + ": an anchor-stream domain is not supported for joins");
  PLUSS_CHECK(p.absent == RaggedAbsent::Null,
              std::string(where) + ": a row with no partner is always an explicit null; absent must be Null");
  PLUSS_CHECK(p.nulls == RaggedNulls::Skip && p.min_support == 1,
              std::string(where) + ": nulls and min_support do not apply to joins (null keys never match)");
}

std::int64_t shift_of(const RaggedLayout& layout, const std::string& key) { return layout.shift_of(key); }

std::vector<bool> join_validity_of(const Stream& stream) {
  if (!detail::has_validity_mask(stream)) return {};
  return detail::validity_mask(stream)->host<bool>();
}

inline std::int64_t canonical_double_code(double v) {
  if (std::isnan(v)) return kNoKey;
  if (v == 0.0) v = 0.0;  // -0.0 and +0.0 are the same key
  std::int64_t bits;
  std::memcpy(&bits, &v, sizeof bits);
  return bits == kNoKey ? bits + 1 : bits;
}

enum class JoinKeyMode { Integer, Float, Text, Time };

JoinKeyMode join_key_mode(const Stream& a, const Stream& b, std::string_view where, std::string_view key) {
  const auto ka = a.kind(), kb = b.kind();
  PLUSS_CHECK(ka == kb, pluss::tils::OutputEngine::format("{}: key '{}' has different kinds on the two sides", where, key));
  switch (ka) {
    case StreamKind::Series: {
      const auto da = a.as_series().dtype(), db = b.as_series().dtype();
      PLUSS_CHECK(!da.is_complex() && !db.is_complex(), pluss::tils::OutputEngine::format("{}: key '{}' cannot be complex", where, key));
      return (!da.is_floating() && !db.is_floating()) ? JoinKeyMode::Integer : JoinKeyMode::Float;
    }
    case StreamKind::Note:
      return JoinKeyMode::Text;
    case StreamKind::Timeline:
      return JoinKeyMode::Time;
    default:
      PLUSS_FAIL(pluss::tils::OutputEngine::format("{}: key '{}' has a kind that cannot be joined on", where, key));
  }
}

// One key column on one side as codes over `domain` window records: records the
// stream does not reach, and null values, get kNoKey.
void encode_join_side(const Stream& s, JoinKeyMode mode, std::int64_t domain, std::int64_t shift,
                      std::vector<std::int64_t>& out,
                      std::unordered_map<std::string_view, std::int64_t>* dictionary) {
  out.assign(static_cast<std::size_t>(domain), kNoKey);
  const std::int64_t size = s.size();
  const auto valid = join_validity_of(s);
  const auto usable = [&](std::int64_t p) { return valid.empty() || valid[static_cast<std::size_t>(p)]; };
  const std::int64_t first = std::max<std::int64_t>(0, -shift);          // first window record that has a position
  const std::int64_t last = std::min<std::int64_t>(domain, size - shift);  // one past the last
  const auto fix = [](std::int64_t v) { return v == kNoKey ? kNoKey + 1 : v; };
  switch (mode) {
    case JoinKeyMode::Integer: {
      const ColumnView<std::int64_t> values(s.as_series(), pluss::codem::ScalarType::Int64);
      for (std::int64_t r = first; r < last; ++r) {
        const auto p = r + shift;
        if (usable(p)) out[static_cast<std::size_t>(r)] = fix(values[static_cast<std::size_t>(p)]);
      }
      break;
    }
    case JoinKeyMode::Float: {
      const ColumnView<double> values(s.as_series(), pluss::codem::ScalarType::Float64);
      for (std::int64_t r = first; r < last; ++r) {
        const auto p = r + shift;
        if (usable(p)) out[static_cast<std::size_t>(r)] = canonical_double_code(values[static_cast<std::size_t>(p)]);
      }
      break;
    }
    case JoinKeyMode::Time: {
      const ColumnView<std::int64_t> values(s.as_timeline().epoch_nanos(), pluss::codem::ScalarType::Int64);
      for (std::int64_t r = first; r < last; ++r) {
        const auto p = r + shift;
        if (usable(p)) out[static_cast<std::size_t>(r)] = fix(values[static_cast<std::size_t>(p)]);
      }
      break;
    }
    case JoinKeyMode::Text: {
      // The views point into the Note's storage, which the catalogs keep alive
      // for the whole call.
      const auto views = s.as_note().vector();
      for (std::int64_t r = first; r < last; ++r) {
        const auto p = r + shift;
        if (!usable(p)) continue;
        const auto it = dictionary->try_emplace(views[static_cast<std::size_t>(p)],
                                                static_cast<std::int64_t>(dictionary->size())).first;
        out[static_cast<std::size_t>(r)] = it->second;
      }
      break;
    }
  }
}

struct EncodedJoinSide {
  std::vector<std::vector<std::int64_t>> columns{};
  relational::KeyColumns view(std::int64_t records) const {
    relational::KeyColumns out;
    out.records = records;
    for (const auto& c : columns) out.cols.push_back(c.data());
    return out;
  }
};

void encode_join_keys(const Catalog& left, const Catalog& right, const std::vector<std::string>& lk,
                      const std::vector<std::string>& rk, const RaggedLayout& ll, const RaggedLayout& rl,
                      std::string_view where, EncodedJoinSide& L, EncodedJoinSide& R) {
  PLUSS_CHECK(lk.size() == rk.size(), pluss::tils::OutputEngine::format("{}: left and right key counts differ", where));
  for (const auto& k : lk) PLUSS_CHECK(left.has(k), pluss::tils::OutputEngine::format("{}: left catalog has no key '{}'", where, k));
  for (const auto& k : rk) PLUSS_CHECK(right.has(k), pluss::tils::OutputEngine::format("{}: right catalog has no key '{}'", where, k));
  L.columns.resize(lk.size());
  R.columns.resize(rk.size());
  for (std::size_t c = 0; c < lk.size(); ++c) {
    const auto ls = left.keys(lk[c]);
    const auto rs = right.keys(rk[c]);
    const auto mode = join_key_mode(ls, rs, where, lk[c]);
    std::unordered_map<std::string_view, std::int64_t> dictionary;
    encode_join_side(ls, mode, ll.domain(), shift_of(ll, lk[c]), L.columns[c], &dictionary);
    encode_join_side(rs, mode, rl.domain(), shift_of(rl, rk[c]), R.columns[c], &dictionary);
  }
}

// Hashed right-hand sides reused across joins: the same dimension table joined
// again (a pull over several hops, an audit then a join, repeated queries) skips
// re-encoding and re-hashing its keys. Keyed on stream fingerprints, so an edit
// to the data is a different key; entries pin their catalog so addresses stay
// unique. Bounded by entry count and bytes; least recently used goes first.
class BuiltSideCache {
public:
  struct Entry {
    std::string key;
    std::shared_ptr<relational::BuiltSide> side;
    Catalog holder;
    std::size_t bytes{0};
  };

  std::shared_ptr<relational::BuiltSide> find(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = lru_.begin(); it != lru_.end(); ++it) {
      if (it->key == key) {
        lru_.splice(lru_.begin(), lru_, it);
        ++hits_;
        return it->side;
      }
    }
    ++misses_;
    return nullptr;
  }

  void insert(std::string key, std::shared_ptr<relational::BuiltSide> side, const Catalog& holder) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto bytes = side->bytes();
    if (bytes > kMaxBytes / 2) return;  // never let one table take over the cache
    lru_.push_front(Entry{std::move(key), std::move(side), holder, bytes});
    bytes_ += bytes;
    while (lru_.size() > kMaxEntries || bytes_ > kMaxBytes) {
      bytes_ -= lru_.back().bytes;
      lru_.pop_back();
    }
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    lru_.clear();
    bytes_ = 0;
  }

  std::pair<std::int64_t, std::int64_t> stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    return {hits_, misses_};
  }

private:
  static constexpr std::size_t kMaxEntries = 32;
  static constexpr std::size_t kMaxBytes = std::size_t{256} << 20;
  std::mutex mutex_;
  std::list<Entry> lru_;
  std::size_t bytes_{0};
  std::int64_t hits_{0}, misses_{0};
};

BuiltSideCache& built_side_cache() {
  static BuiltSideCache cache;
  return cache;
}

constexpr std::int64_t kMinCachedRecords = 4096;  // below this, hashing again is cheaper than the bookkeeping

CatalogMatch compute_match(const Catalog& left, const Catalog& right, const std::vector<std::string>& lk,
                           const std::vector<std::string>& rk, relational::JoinHow how,
                           std::optional<relational::RelationshipCardinality> validate,
                           const RaggedPolicy& ragged, std::string_view where) {
  check_join_policy(ragged, where);
  const bool cross = how == relational::JoinHow::Cross;
  PLUSS_CHECK(cross || !lk.empty(), std::string(where) + ": join keys cannot be empty");
  CatalogMatch out;
  out.left = RaggedLayout::of(left, ragged, where);
  out.right = RaggedLayout::of(right, ragged, where);
  relational::MatchOptions mo;
  mo.how = how;
  mo.validate = validate;
  mo.where = where;

  // A right-hand side that is hashed (every join but Right/RightOnly) with
  // non-text keys can come from the cache.
  bool cacheable = !cross && how != relational::JoinHow::Right && how != relational::JoinHow::RightOnly &&
                   out.right.domain() >= kMinCachedRecords && lk.size() == rk.size();
  std::vector<JoinKeyMode> modes;
  if (cacheable) {
    for (std::size_t c = 0; c < lk.size() && cacheable; ++c) {
      if (!left.has(lk[c]) || !right.has(rk[c])) {
        cacheable = false;  // let the full path report the missing key
        break;
      }
      modes.push_back(join_key_mode(left.keys(lk[c]), right.keys(rk[c]), where, lk[c]));
      cacheable = modes.back() != JoinKeyMode::Text;
    }
  }

  if (cacheable) {
    std::string key = pluss::tils::OutputEngine::format("{}|{}|", out.right.domain(), static_cast<int>(ragged.alignment));
    for (std::size_t c = 0; c < rk.size(); ++c) {
      key += pluss::tils::OutputEngine::format("{}:{}:{}:{};", right.keys(rk[c]).fingerprint(), static_cast<int>(modes[c]),
                         shift_of(out.right, rk[c]), rk[c]);
    }
    auto built = built_side_cache().find(key);
    if (!built) {
      std::vector<std::vector<std::int64_t>> codes(rk.size());
      for (std::size_t c = 0; c < rk.size(); ++c) {
        encode_join_side(right.keys(rk[c]), modes[c], out.right.domain(), shift_of(out.right, rk[c]), codes[c],
                         nullptr);
      }
      built = std::make_shared<relational::BuiltSide>(std::move(codes), out.right.domain());
      built_side_cache().insert(key, built, right);
    }
    std::vector<std::vector<std::int64_t>> lcodes(lk.size());
    for (std::size_t c = 0; c < lk.size(); ++c) {
      encode_join_side(left.keys(lk[c]), modes[c], out.left.domain(), shift_of(out.left, lk[c]), lcodes[c], nullptr);
    }
    relational::KeyColumns lview;
    lview.records = out.left.domain();
    for (const auto& col : lcodes) lview.cols.push_back(col.data());
    out.pairs = relational::match(lview, *built, mo);
    return out;
  }

  EncodedJoinSide L, R;
  if (!cross) encode_join_keys(left, right, lk, rk, out.left, out.right, where, L, R);
  out.pairs = relational::match(L.view(out.left.domain()), R.view(out.right.domain()), mo);
  return out;
}

}  // namespace

std::string CatalogJoinReport::summary() const {
  return pluss::tils::OutputEngine::format(
      "{} output record(s); {} matched pair(s). left: {} of {} record(s) without a partner ({} without a key), "
      "{} omitted{}. right: {} of {} without a partner ({} without a key), {} omitted{}.",
      output_records, match.matched_pairs, match.left_unmatched, match.left_records, match.left_unkeyed,
      match.left_dropped, trimmed_left > 0 ? pluss::tils::OutputEngine::format(", {} trimmed", trimmed_left) : std::string(),
      match.right_unmatched, match.right_records, match.right_unkeyed, match.right_dropped,
      trimmed_right > 0 ? pluss::tils::OutputEngine::format(", {} trimmed", trimmed_right) : std::string());
}

CatalogJoinResult Catalog::join_detailed(const Catalog& other, const CatalogJoinSpec& spec,
                                         const CatalogJoinOptions& options) const {
  constexpr std::string_view where = "Catalog::join";
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(other.defined(), "Catalog::join: other catalog is undefined");
  const bool cross = spec.how == relational::JoinHow::Cross;

  auto matched_all = compute_match(*this, other, spec.left_keys, spec.right_keys, spec.how, spec.validate,
                                   options.ragged, where);
  const auto& ll = matched_all.left;
  const auto& rl = matched_all.right;
  auto& matched = matched_all.pairs;

  CatalogJoinResult result;
  result.report.match = matched.report;
  result.report.output_records = matched.report.pairs;
  result.report.trimmed_left = ll.trimmed();
  result.report.trimmed_right = rl.trimmed();
  PLUSS_CHECK(options.ragged.loss != RaggedLoss::Error ||
                  (matched.report.left_dropped == 0 && matched.report.right_dropped == 0),
              pluss::tils::OutputEngine::format("{}: loss is set to Error but the join omits records ({})", where,
                          result.report.summary()));

  // Output columns: left streams, then right streams (a same-named right key is
  // folded into the left one when the output cannot hold right-only records).
  const bool coalesce = options.coalesce_keys && !cross &&
                        (spec.how == relational::JoinHow::Inner || spec.how == relational::JoinHow::Left ||
                         spec.how == relational::JoinHow::LeftOnly);
  std::unordered_set<std::string> folded;
  if (coalesce) {
    for (std::size_t c = 0; c < spec.left_keys.size(); ++c) {
      if (spec.left_keys[c] == spec.right_keys[c]) folded.insert(spec.right_keys[c]);
    }
  }

  struct Job {
    std::string name;
    Stream stream;
    std::int64_t shift;
    bool from_left;
  };
  std::vector<Job> jobs;
  std::unordered_map<std::string, std::string> claimed_by;  // final name -> origin, for the error message
  std::unordered_set<std::string> left_names;
  for (const auto& name : keys()) {
    jobs.push_back({name, keys(name), shift_of(ll, name), true});
    claimed_by.emplace(name, "left '" + name + "'");
    left_names.insert(name);
  }
  // A right column keeps its name unless the left side already has it; the suffix
  // is applied to exactly those, and no column is ever renamed to make room.
  for (const auto& name : other.keys()) {
    if (folded.count(name) != 0) continue;
    const std::string final_name = left_names.count(name) != 0 ? name + spec.right_suffix : name;
    const auto clash = claimed_by.find(final_name);
    PLUSS_CHECK(clash == claimed_by.end(),
                pluss::tils::OutputEngine::format("{}: right column '{}' would be named '{}' in the output, which {} already uses; "
                            "choose another right_suffix or rename a column",
                            where, name, final_name, clash == claimed_by.end() ? std::string() : clash->second));
    claimed_by.emplace(final_name, "right '" + name + "'");
    jobs.push_back({final_name, other.keys(name), shift_of(rl, name), false});
  }

  const bool trim_trailing = options.ragged.materialize == RaggedMaterialize::Preserve;
  std::vector<Stream> built(jobs.size());
  std::exception_ptr failure;
  std::mutex failure_mutex;
  pluss::msharp::parallel_for(0, static_cast<std::int64_t>(jobs.size()), 1, [&](std::int64_t a, std::int64_t b) {
    for (std::int64_t j = a; j < b; ++j) {
      try {
        const auto& job = jobs[static_cast<std::size_t>(j)];
        built[static_cast<std::size_t>(j)] =
            job.stream.gather(job.from_left ? matched.left : matched.right, job.shift, trim_trailing);
      } catch (...) {
        std::lock_guard<std::mutex> lock(failure_mutex);
        if (!failure) failure = std::current_exception();
      }
    }
  });
  if (failure) std::rethrow_exception(failure);

  std::vector<std::pair<std::string, Stream>> streams;
  streams.reserve(jobs.size());
  for (std::size_t j = 0; j < jobs.size(); ++j) streams.emplace_back(jobs[j].name, std::move(built[j]));
  result.catalog = Catalog(std::move(streams));
  return result;
}

CatalogMatch Catalog::match_keys(const Catalog& other, const std::vector<std::string>& left_keys,
                                 const std::vector<std::string>& right_keys, relational::JoinHow how,
                                 std::optional<relational::RelationshipCardinality> validate,
                                 const RaggedPolicy& ragged) const {
  PLUSS_CHECK(defined() && other.defined(), "Catalog::match_keys: undefined catalog");
  return compute_match(*this, other, left_keys, right_keys, how, validate, ragged, "Catalog::match_keys");
}

Catalog Catalog::join(const Catalog& other, const CatalogJoinSpec& spec, const CatalogJoinOptions& options) const {
  return join_detailed(other, spec, options).catalog;
}

CatalogKeyAudit Catalog::audit_keys(const Catalog& other, const std::vector<std::string>& left_keys,
                                    const std::vector<std::string>& right_keys, const RaggedPolicy& ragged) const {
  constexpr std::string_view where = "Catalog::audit_keys";
  PLUSS_CHECK(defined() && other.defined(), "Catalog::audit_keys: undefined catalog");
  CatalogJoinOptions options;
  options.ragged = ragged;
  check_join_policy(ragged, where);
  const auto ll = RaggedLayout::of(*this, ragged, where);
  const auto rl = RaggedLayout::of(other, ragged, where);
  EncodedJoinSide L, R;
  encode_join_keys(*this, other, left_keys, right_keys, ll, rl, where, L, R);
  const auto lview = L.view(ll.domain()), rview = R.view(rl.domain());

  CatalogKeyAudit out;
  out.left_records = ll.domain();
  out.right_records = rl.domain();
  const auto lu = relational::measure_uniqueness(lview);
  const auto ru = relational::measure_uniqueness(rview);
  out.left_unique = lu.unique;
  out.right_unique = ru.unique;
  out.left_unkeyed = lu.unkeyed;
  out.right_unkeyed = ru.unkeyed;

  relational::MatchOptions mo;
  mo.where = where;
  mo.how = relational::JoinHow::LeftOnly;
  out.left_orphans = relational::match(lview, rview, mo).report.pairs;
  mo.how = relational::JoinHow::RightOnly;
  out.right_orphans = relational::match(lview, rview, mo).report.pairs;
  return out;
}

Catalog Catalog::merge(const Catalog& other, std::string_view on, std::string_view how) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(other.defined(), "Catalog::merge: other catalog is undefined");
  const auto how_norm = lowercase_copy(trim_copy(how));
  const auto key = trim_copy(on);
  PLUSS_CHECK(!key.empty(), "Catalog::merge: key cannot be empty");
  CatalogJoinSpec spec;
  if (how_norm == "inner") spec.how = relational::JoinHow::Inner;
  else if (how_norm == "left") spec.how = relational::JoinHow::Left;
  else if (how_norm == "right") spec.how = relational::JoinHow::Right;
  else if (how_norm == "outer") spec.how = relational::JoinHow::Outer;
  else PLUSS_FAIL("Catalog::merge: how must be inner, left, right or outer, got '" + how_norm + "'");
  spec.left_keys = {key};
  spec.right_keys = {key};
  return join(other, spec);
}


void Catalog::clear_join_cache() { built_side_cache().clear(); }
std::pair<std::int64_t, std::int64_t> Catalog::join_cache_stats() { return built_side_cache().stats(); }
