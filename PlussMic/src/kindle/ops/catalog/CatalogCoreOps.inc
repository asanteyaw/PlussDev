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

Catalog Catalog::where(const pluss::Series& condition) const {
  return filter(condition);
}

Catalog Catalog::query(const QueryFn& fn) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(static_cast<bool>(fn), "Catalog::query: query function is empty");
  const auto mask = fn(*this);
  return filter(mask);
}

Catalog Catalog::select(pluss::msharp::RangeView::Of<pluss::table::Slate> conditions,
                        pluss::msharp::RangeView::Of<Catalog> choices,
                        const Catalog& default_value) {
  PLUSS_CHECK(!choices.empty(), "Catalog::select: at least one choice is required");
  PLUSS_CHECK(default_value.defined(), "Catalog::select: default value is undefined");
  PLUSS_CHECK(conditions.size() == choices.size(),
              "Catalog::select: condition/choice count mismatch");

  const auto& base_keys = default_value.keys();
  for (const auto& choice : choices) {
    PLUSS_CHECK(choice.defined(), "Catalog::select: choice is undefined");
    PLUSS_CHECK(choice.keys() == base_keys,
                "Catalog::select: all choices must have the same keys and order");
  }

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(base_keys.size());
  for (const auto& key : base_keys) {
    std::vector<Stream> stream_choices;
    stream_choices.reserve(choices.size());
    for (const auto& choice : choices) {
      stream_choices.push_back(choice.keys(key));
    }
    out.emplace_back(
        key,
        Stream::select(
            conditions,
            pluss::msharp::RangeView::Of<Stream>(stream_choices),
            default_value.keys(key)));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::drop(std::string_view field_name) const {
  return drop(std::vector<std::string>{std::string(field_name)});
}

Catalog Catalog::drop(std::span<const std::string_view> field_names) const {
  const auto names = normalize_names(field_names);
  return drop(names);
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
  return sort(std::vector<std::string>{key}, {ascending});
}

Catalog Catalog::sort(std::span<const std::string_view> fields,
                      const std::vector<bool>& ascending) const {
  std::vector<std::string> names;
  names.reserve(fields.size());
  for (const auto name : fields) {
    names.emplace_back(name);
  }
  return sort(names, ascending);
}

Catalog Catalog::sort(std::initializer_list<std::string_view> fields,
                      const std::vector<bool>& ascending) const {
  return sort(std::span<const std::string_view>(fields.begin(), fields.size()), ascending);
}

Catalog Catalog::sort(const std::vector<std::string>& fields,
                      const std::vector<bool>& ascending) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto names = normalize_names(fields);
  PLUSS_CHECK(!names.empty(), "Catalog::sort: at least one field is required");
  PLUSS_CHECK(ascending.empty() || ascending.size() == names.size(),
              "Catalog::sort: ascending size must be 0 or equal to fields size");

  std::vector<Stream> key_streams;
  key_streams.reserve(names.size());
  for (const auto& n : names) {
    PLUSS_CHECK(has(n), "Catalog::sort: unknown key '" + n + "'");
    key_streams.push_back(at(n));
  }
  const auto idx = sort_indices_for_keys(key_streams, ascending);

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& s = impl_ref_().streams_[i];
    PLUSS_CHECK(s.size() == static_cast<std::int64_t>(idx.size()),
                "Catalog::sort: sort fields require aligned stream lengths");
    out.emplace_back(impl_ref_().keys_[i], s.take(idx));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(double value) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = stream.kind() == StreamKind::Series;
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(std::string_view value) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = stream.kind() == StreamKind::Note;
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(const pluss::Timeline& value) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = stream.kind() == StreamKind::Timeline;
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::fillnull(const pluss::Duration& value) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    const bool selected = stream.kind() == StreamKind::Duration;
    out.emplace_back(key, selected ? stream.fillnull(value) : stream);
  }
  return Catalog(std::move(out));
}

Catalog Catalog::dropnull() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::optional<pluss::table::Slate> keep;
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto valid = impl_ref_().streams_[i].notnull().to_slate();
    keep = keep.has_value() ? keep->land(valid) : valid;
  }
  if (!keep.has_value()) return *this;
  const auto mask = keep->as_series();

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    const auto& key = impl_ref_().keys_[i];
    const auto& stream = impl_ref_().streams_[i];
    out.emplace_back(key, stream.filter(mask));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::ffill() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].ffill());
  }
  return Catalog(std::move(out));
}

Catalog Catalog::bfill() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].bfill());
  }
  return Catalog(std::move(out));
}

Catalog Catalog::isnull() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].isnull());
  }
  return Catalog(std::move(out));
}

Catalog Catalog::notnull() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].notnull());
  }
  return Catalog(std::move(out));
}

Catalog Catalog::invalid() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");

  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].invalid());
  }
  return Catalog(std::move(out));
}

Catalog Catalog::cast(pluss::table::SlateCastTarget target,
                      const pluss::table::SlateCastOptions& options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].cast(target, options));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::astype(pluss::codem::ScalarType dtype) const {
  return cast(pluss::table::SlateCastTarget::Series,
              pluss::table::SlateCastOptions{.series_dtype = dtype});
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

Catalog Catalog::freq(FreqOptions options) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(nstreams()));
  for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
    out.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i].freq(options));
  }
  return Catalog(std::move(out));
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

std::int64_t Catalog::count() const {
  return nrows();
}

Catalog Catalog::map(const std::function<Stream(const Stream&)>& fn) const {
  return transform(fn);
}

Catalog& Catalog::map_(const std::function<Stream(const Stream&)>& fn) {
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
    out.emplace_back(key, src.map(fn));
  }
  return Catalog(std::move(out));
}
