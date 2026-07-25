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

Catalog Catalog::kindelize(const pluss::table::DataFrame& frame, const KindelizePolicy& policy) {
  PLUSS_CHECK(frame.defined(), "Catalog::kindelize requires a defined DataFrame");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(frame.nfeatures()) + (policy.materialize_label ? 1u : 0u));

  const auto case_axis = frame.label().values();
  for (const auto& key : frame.features()) {
    auto s = Stream::from_thread(frame.features(key),
                                 policy.attach_labels ? std::optional<pluss::table::Thread>{case_axis}
                                                      : std::nullopt);
    out.emplace_back(key, std::move(s));
  }

  if (policy.materialize_label) {
    PLUSS_CHECK(frame.features().end() ==
                    std::find(frame.features().begin(), frame.features().end(), policy.label_key),
                "Catalog::kindelize: label key collides with existing feature '" + policy.label_key + "'");
    out.emplace_back(policy.label_key, Stream::from_thread(case_axis));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::load(std::string_view source_type, std::string_view filepath) {
  const auto source = lowercase_copy(trim_copy(source_type));
  if (source == "csv") {
    return load_csv(filepath);
  }
  if (source == "txt" || source == "text") {
    return load_txt(filepath);
  }
  if (source == "excel" || source == "xlsx") {
    return load_excel(filepath);
  }
  if (source == "parquet") {
    return load_parquet(filepath);
  }
  if (source == "json") {
    return load_json(filepath);
  }
  return load(source_type, filepath, pluss::loader::Config{});
}

Catalog Catalog::load(std::string_view source_type,
                      std::string_view filepath,
                      const pluss::loader::Config& config) {
  const auto source = lowercase_copy(trim_copy(source_type));
  if (source == "csv") {
    const auto options = pluss::loader::csv_options_from_config(config);
    return catalog_from_raw_batch(pluss::loader::read_csv_raw_batch(filepath, options), options);
  }
  if (source == "txt" || source == "text") {
    return load_txt(filepath, config);
  }
  if (source == "excel" || source == "xlsx") {
    auto xopts = pluss::loader::excel_options_from_config(config);
    auto infer_options = pluss::loader::csv_options_from_config(config);
    infer_options.header = xopts.header;
    return catalog_from_raw_batch(pluss::loader::read_excel_raw_batch(filepath, xopts), infer_options);
  }
  if (source == "parquet") {
    return load_parquet(filepath, config);
  }
  if (source == "json") {
    return load_json(filepath, config);
  }
  PLUSS_FAIL("Catalog::load: unsupported source type '" + std::string(source_type) + "'");
}

Catalog Catalog::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return catalog_from_raw_batch(pluss::loader::read_csv_raw_batch(filepath, options), options);
}

Catalog Catalog::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return catalog_from_raw_batch(pluss::loader::read_csv_raw_batch(filepath, options), options);
}

Catalog Catalog::load_txt(std::string_view filepath) {
  return load_txt(filepath, pluss::loader::Config{});
}

Catalog Catalog::load_txt(std::string_view filepath, const pluss::loader::Config& config) {
  const auto infer_options = pluss::loader::csv_options_from_config(config);
  return catalog_from_raw_batch(pluss::loader::read_txt_raw_batch(filepath, config), infer_options);
}

Catalog Catalog::load_excel(std::string_view filepath) {
  pluss::loader::ReadExcelOptions options;
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = options.header;
  return catalog_from_raw_batch(pluss::loader::read_excel_raw_batch(filepath, options), infer_options);
}

Catalog Catalog::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = options.header;
  return catalog_from_raw_batch(pluss::loader::read_excel_raw_batch(filepath, options), infer_options);
}

Catalog Catalog::load_parquet(std::string_view filepath) {
  return load_parquet(filepath, pluss::loader::Config{});
}

Catalog Catalog::load_parquet(std::string_view filepath, const pluss::loader::Config& config) {
  auto infer_options = pluss::loader::csv_options_from_config(config);
  return catalog_from_raw_batch(pluss::loader::read_parquet_raw_batch(filepath, config), infer_options);
}

Catalog Catalog::load_json(std::string_view filepath) {
  return load_json(filepath, pluss::loader::Config{});
}

Catalog Catalog::load_json(std::string_view filepath, const pluss::loader::Config& config) {
  auto infer_options = pluss::loader::csv_options_from_config(config);
  return catalog_from_raw_batch(pluss::loader::read_json_raw_batch(filepath, config), infer_options);
}

Catalog Catalog::load_csv_with_key(std::string_view filepath,
                                   std::string_view key_field,
                                   const pluss::loader::ReadCsvOptions& options) {
  auto raw = pluss::loader::read_csv_raw_batch(filepath, options);
  const auto key = trim_copy(key_field);
  PLUSS_CHECK(!key.empty(), "Catalog::load_csv_with_key: key field cannot be empty");
  auto it = std::find_if(raw.features.begin(),
                         raw.features.end(),
                         [&](const auto& col) { return col.name == key; });
  PLUSS_CHECK(it != raw.features.end(),
              "Catalog::load_csv_with_key: key field not found '" + key + "'");
  raw.cases = std::move(*it);
  raw.features.erase(it);
  KindelizePolicy policy;
  policy.attach_labels = true;
  policy.materialize_label = true;
  policy.label_key = "_label";
  return catalog_from_raw_batch(raw, options, policy);
}

Catalog Catalog::from_json(std::string_view json_text) {
  const pluss::loader::Config config{};
  const auto infer_options = pluss::loader::csv_options_from_config(config);
  return catalog_from_raw_batch(pluss::loader::read_json_raw_text(json_text, config), infer_options);
}

bool Catalog::defined() const noexcept { return static_cast<bool>(impl_); }
bool Catalog::unique() const noexcept { return defined() && impl_.use_count() == 1; }

std::int64_t Catalog::nstreams() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return static_cast<std::int64_t>(impl_ref_().streams_.size());
}

std::int64_t Catalog::nkeys() const {
  return nstreams();
}

std::int64_t Catalog::minrecords() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (impl_ref_().streams_.empty()) return 0;
  std::int64_t out = impl_ref_().streams_.front().size();
  for (const auto& stream : impl_ref_().streams_) {
    out = std::min(out, stream.size());
  }
  return out;
}

std::int64_t Catalog::maxrecords() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  std::int64_t out = 0;
  for (const auto& stream : impl_ref_().streams_) {
    out = std::max(out, stream.size());
  }
  return out;
}

std::int64_t Catalog::nrecords() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (impl_ref_().streams_.empty()) return 0;
  const auto n = impl_ref_().streams_.front().size();
  for (const auto& stream : impl_ref_().streams_) {
    PLUSS_CHECK(stream.size() == n,
                "Catalog::nrecords: catalog has ragged stream lengths; use minrecords()/maxrecords() or tabelize()");
  }
  return n;
}

std::int64_t Catalog::size() const {
  return nkeys();
}

std::pair<std::int64_t, std::int64_t> Catalog::shape() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return {maxrecords(), nkeys()};
}

bool Catalog::empty() const { return nkeys() == 0; }

const std::vector<std::string>& Catalog::keys() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return impl_ref_().keys_;
}

bool Catalog::has(std::string_view key) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return impl_ref_().key_to_pos_.find(std::string(key)) != impl_ref_().key_to_pos_.end();
}

Stream Catalog::at(std::string_view key) const {
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
