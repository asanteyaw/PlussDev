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

Catalog Catalog::cloc(std::int64_t row) const {
  return slice(row, row + 1);
}

Catalog Catalog::cloc(std::int64_t start, std::int64_t stop) const {
  return slice(start, stop);
}

Catalog Catalog::take(std::span<const std::int64_t> indices) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].take(indices));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::sample(std::int64_t n, std::optional<std::uint64_t> random_state) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(n >= 0, "Catalog::sample: n must be >= 0");
  if (empty()) {
    return *this;
  }

  const auto nrows = impl_ref_().streams_.front().size();
  for (std::size_t i = 1; i < impl_ref_().streams_.size(); ++i) {
    PLUSS_CHECK(impl_ref_().streams_[i].size() == nrows,
                "Catalog::sample: requires aligned stream lengths");
  }
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows);
  std::vector<std::int64_t> idx(static_cast<std::size_t>(nrows));
  std::iota(idx.begin(), idx.end(), static_cast<std::int64_t>(0));
  std::mt19937_64 rng(random_state.value_or(static_cast<std::uint64_t>(std::random_device{}())));
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(clipped));
  return take(idx);
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

Catalog Catalog::where(const pluss::Series& condition) const {
  return filter(condition);
}

Catalog Catalog::query(const QueryFn& fn) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Catalog::query: query function is empty");
  const auto mask = fn(*this);
  return filter(mask);
}

Catalog Catalog::select(std::initializer_list<std::string_view> field_names) const {
  std::vector<std::string> names;
  names.reserve(field_names.size());
  for (const auto name : field_names) {
    names.emplace_back(name);
  }
  return select(names);
}

Catalog Catalog::select(const std::vector<std::string>& field_names) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto names = normalize_names(field_names);
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Catalog::select: unknown key '" + n + "'");
    out.emplace_back(n, at(n));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::drop(std::string_view field_name) const {
  return drop(std::vector<std::string>{std::string(field_name)});
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

Catalog Catalog::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const {
  std::unordered_map<std::string, std::string> out;
  out.reserve(mapping.size());
  for (const auto& [from, to] : mapping) {
    const auto inserted = out.emplace(std::string(from), std::string(to)).second;
    PLUSS_CHECK(inserted, "Catalog::rename: duplicate source key in mapping");
  }
  return rename(out);
}

Catalog Catalog::rename(std::string_view from, std::string_view to) const {
  return rename({{from, to}});
}

Catalog& Catalog::rename_(std::string_view from, std::string_view to) {
  *this = rename(from, to);
  return *this;
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
    key_streams.push_back(at(n));
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

  const auto key_stream = at(key);
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

Catalog Catalog::fillnull(double value, std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::fillnull: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() ? (stream.kind() == StreamKind::Series)
                                          : (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(std::string_view value, std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::fillnull(string): unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() ? (stream.kind() == StreamKind::Note)
                                          : (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(const pluss::Timeline& value, std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::fillnull(Timeline): unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() ? (stream.kind() == StreamKind::Timeline || stream.kind() == StreamKind::TimeIndex)
                                          : (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::fillnull(TimeIndex): unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() ? (stream.kind() == StreamKind::TimeIndex)
                                          : (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::dropnull(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::dropnull: unknown key '" + k + "'");
    }
  }

  const auto n = nrecords();
  std::vector<bool> keep(static_cast<std::size_t>(n), true);
  bool saw_selected = false;
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    if (!selected) continue;
    saw_selected = true;
    const auto valid = impl_ref_().streams_[i].to_thread().notnull().as_series().to_vec<bool>();
    PLUSS_CHECK(static_cast<std::int64_t>(valid.size()) == n,
                "Catalog::dropnull: selected key produced invalid null mask length");
    for (std::int64_t r = 0; r < n; ++r) {
      keep[static_cast<std::size_t>(r)] = keep[static_cast<std::size_t>(r)] && valid[static_cast<std::size_t>(r)];
    }
  }
  if (!saw_selected) return *this;

  auto mask = pluss::Series::zeros({n}, pluss::codem::ScalarType::Bool);
  auto* p = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t r = 0; r < n; ++r) {
    p[r] = keep[static_cast<std::size_t>(r)];
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    out.emplace_back(key, stream.filter(mask));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::ffill(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::ffill: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.ffill() : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::bfill(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::bfill: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.bfill() : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::isnull(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::isnull: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.isnull() : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::notnull(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::notnull: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.notnull() : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::invalid(std::initializer_list<std::string_view> keys) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::invalid: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.invalid() : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::cast(std::initializer_list<std::string_view> keys,
                      pluss::table::ThreadCastTarget target,
                      const pluss::table::ThreadCastOptions& options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  if (!key_set.empty()) {
    for (const auto& k : key_set) {
      PLUSS_CHECK(has(k), "Catalog::cast: unknown key '" + k + "'");
    }
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.empty() || (key_set.find(key) != key_set.end());
    out.emplace_back(key, selected ? stream.cast(target, options.series_dtype) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::astype(std::initializer_list<std::string_view> keys,
                        pluss::codem::ScalarType dtype) const {
  return cast(keys, pluss::table::ThreadCastTarget::Series,
              pluss::table::ThreadCastOptions{.series_dtype = dtype});
}

Stream Catalog::sum(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(field).sum();
}

Stream Catalog::mean(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(field).mean();
}

Stream Catalog::min(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(field).min();
}

Stream Catalog::max(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(field).max();
}

Stream Catalog::distinct(std::string_view key, DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(key).distinct(options);
}

Catalog Catalog::distinct(DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].distinct(options));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::distinct(std::initializer_list<std::string_view> keys, DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  PLUSS_CHECK(!key_set.empty(), "Catalog::distinct(keys): keys cannot be empty; use distinct() for all keys");
  for (const auto& key : key_set) {
    PLUSS_CHECK(has(key), "Catalog::distinct: unknown key '" + key + "'");
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.find(key) != key_set.end();
    out.emplace_back(key, selected ? stream.distinct(options) : stream);
  }
  return Catalog(std::move(out));
}

Stream Catalog::freq(std::string_view key, FreqOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(key).freq(options);
}

Catalog Catalog::freq(FreqOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].freq(options));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::freq(std::initializer_list<std::string_view> keys,
                      FreqOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  PLUSS_CHECK(!key_set.empty(), "Catalog::freq(keys): keys cannot be empty; use freq() for all keys");
  for (const auto& key : key_set) {
    PLUSS_CHECK(has(key), "Catalog::freq: unknown key '" + key + "'");
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.find(key) != key_set.end();
    out.emplace_back(key, selected ? stream.freq(options) : stream);
  }
  return Catalog(std::move(out));
}

Stream Catalog::ndistinct(std::string_view key, DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return at(key).ndistinct(options);
}

Catalog Catalog::ndistinct(DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].ndistinct(options));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::ndistinct(std::initializer_list<std::string_view> keys, DistinctOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key_set = normalize_name_set(keys);
  PLUSS_CHECK(!key_set.empty(), "Catalog::ndistinct(keys): keys cannot be empty; use ndistinct() for all keys");
  for (const auto& key : key_set) {
    PLUSS_CHECK(has(key), "Catalog::ndistinct: unknown key '" + key + "'");
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = key_set.find(key) != key_set.end();
    out.emplace_back(key, selected ? stream.ndistinct(options) : stream);
  }
  return Catalog(std::move(out));
}

std::int64_t Catalog::count() const {
  return nrecords();
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
  PLUSS_CHECK(left.get(key).has_value(), "Catalog::merge: left key not found '" + key + "'");
  PLUSS_CHECK(right.get(key).has_value(), "Catalog::merge: right key not found '" + key + "'");
  return kindelize(left.unify(key, right, key));
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
  const auto src = at(key);
  const auto dst = func(src);
  PLUSS_CHECK(dst.defined(), "Catalog::apply_: function returned undefined stream");
  PLUSS_CHECK(dst.size() == src.size(),
              "Catalog::apply_: output stream length must match input stream length");
  return set_stream_(key, dst);
}

Catalog Catalog::apply_all(const std::function<Stream(const Stream&)>& fn) const {
  return transform(fn);
}

Catalog& Catalog::apply_all_(const std::function<Stream(const Stream&)>& fn) {
  *this = transform(fn);
  return *this;
}

Catalog Catalog::transform(const std::function<Stream(const Stream&)>& fn) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Catalog::transform: function is empty");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& src = impl_ref_().streams_[i];
    const auto dst = fn(src);
    PLUSS_CHECK(dst.defined(), "Catalog::transform: function returned undefined stream");
    PLUSS_CHECK(dst.size() == src.size(),
                "Catalog::transform: output stream length must match input stream length");
    out.emplace_back(key, dst);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::transform(std::string_view field, const std::function<Stream(const Stream&)>& fn) const {
  return apply(field, fn);
}
