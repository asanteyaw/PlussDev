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
  return kindelize(left.stitch_on(key, right, key));
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

