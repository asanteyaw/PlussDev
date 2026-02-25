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
                "Catalog::kindelize: case stream key collides with existing feature '" + policy.case_stream_name + "'");
    out.emplace_back(policy.case_stream_name, Stream::from_thread(case_axis));
  }
  return Catalog(std::move(out));
}

Catalog Catalog::load(std::string_view source_type, std::string_view filepath) {
  const auto source = lowercase_copy(trim_copy(source_type));
  if (source == "csv") {
    return load_csv(filepath);
  }
  if (source == "excel" || source == "xlsx") {
    return load_excel(filepath);
  }
  return load(source_type, filepath, pluss::loader::Config{});
}

Catalog Catalog::load(std::string_view source_type,
                      std::string_view filepath,
                      const pluss::loader::Config& config) {
  const auto source = lowercase_copy(trim_copy(source_type));
  if (source == "csv") {
    return catalog_from_batch(pluss::loader::read_csv_batch(filepath, config));
  }
  if (source == "excel" || source == "xlsx") {
    return catalog_from_batch(pluss::loader::read_excel_batch(filepath, config));
  }
  if (source == "parquet") {
    return kindelize(pluss::table::DataFrame::load_parquet(filepath, config));
  }
  PLUSS_FAIL("Catalog::load: unsupported source type '" + std::string(source_type) + "'");
}

Catalog Catalog::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return catalog_from_batch(pluss::loader::read_csv_batch(filepath, options));
}

Catalog Catalog::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return catalog_from_batch(pluss::loader::read_csv_batch(filepath, options));
}

Catalog Catalog::load_excel(std::string_view filepath) {
  pluss::loader::ReadExcelOptions options;
  return catalog_from_batch(pluss::loader::read_excel_batch(filepath, options));
}

Catalog Catalog::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  return catalog_from_batch(pluss::loader::read_excel_batch(filepath, options));
}

Catalog Catalog::load_csv_with_key(std::string_view filepath,
                                   std::string_view key_field,
                                   const pluss::loader::ReadCsvOptions& options) {
  auto batch = pluss::loader::read_csv_batch(filepath, options);
  const auto key = trim_copy(key_field);
  PLUSS_CHECK(!key.empty(), "Catalog::load_csv_with_key: key field cannot be empty");
  auto it = std::find_if(batch.features.begin(),
                         batch.features.end(),
                         [&](const auto& kv) { return kv.first == key; });
  PLUSS_CHECK(it != batch.features.end(),
              "Catalog::load_csv_with_key: key field not found '" + key + "'");
  batch.cases = it->second;
  KindelizePolicy policy;
  policy.attach_cases_to_streams = true;
  policy.materialize_case_stream = true;
  policy.case_stream_name = "_case";
  return catalog_from_batch(std::move(batch), policy);
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

std::int64_t Catalog::nfields() const {
  return nstreams();
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

bool Catalog::has_stream(std::string_view key) const {
  return has(key);
}

Stream Catalog::stream(std::string_view key) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto it = impl_ref_().key_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != impl_ref_().key_to_pos_.end(), "Catalog: key '" + std::string(key) + "' not found");
  return impl_ref_().streams_[static_cast<std::size_t>(it->second)];
}

Stream Catalog::at(std::string_view key) const {
  return stream(key);
}

std::optional<Stream> Catalog::get(std::string_view key) const {
  if (!defined()) return std::nullopt;
  auto it = impl_ref_().key_to_pos_.find(std::string(key));
  if (it == impl_ref_().key_to_pos_.end()) return std::nullopt;
  return impl_ref_().streams_[static_cast<std::size_t>(it->second)];
}

std::optional<Stream> Catalog::get_stream(std::string_view key) const {
  return get(key);
}

std::vector<Stream> Catalog::streams() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return impl_ref_().streams_;
}
