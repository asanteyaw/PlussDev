ColumnRef::ColumnRef(std::string name, Thread column)
    : name_(std::move(name)), column_(std::move(column)) {}

const std::string& ColumnRef::name() const noexcept { return name_; }
const Thread& ColumnRef::thread() const noexcept { return column_; }
pluss::Series ColumnRef::mean() const { return column_.mean(); }
Thread ColumnRef::head(std::int64_t n) const { return column_.head(n, false); }
ThreadValue ColumnRef::value() const { return column_.value(); }
Thread ColumnRef::values() const { return column_; }

DataFrame::LetSlot::LetSlot(DataFrame& owner, std::string name) : owner_(&owner), name_(std::move(name)) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlot: owner must be defined");
  PLUSS_CHECK(!name_.empty(), "DataFrame::set: feature name cannot be empty");
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const Thread& value) {
  PLUSS_CHECK(value.defined(), "DataFrame::set(name)=Thread requires defined rhs thread");
  if (owner_->nfeats() > 0) {
    PLUSS_CHECK(value.size() == owner_->ncases(),
                "DataFrame::set(name)=Thread requires rhs length to match DataFrame cases");
  }
  owner_->set_features_(name_, value);
  return *this;
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const ThreadValue& value) {
  return (*this = value.thread());
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::Series& value) {
  try {
    return (*this = Thread(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=Series failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::Note& value) {
  try {
    return (*this = Thread(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=Note failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::Timeline& value) {
  try {
    return (*this = Thread(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=Timeline failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::TimeIndex& value) {
  try {
    return (*this = Thread(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=TimeIndex failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const DataFrame& frame) {
  PLUSS_CHECK(frame.nfeats() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
  const auto cols = frame.features();
  PLUSS_CHECK(cols.size() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
  return (*this = cols.front());
}

DataFrame::LetSlots::LetSlots(DataFrame& owner, std::vector<std::string> names)
    : owner_(&owner), names_(std::move(names)) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlots: owner must be defined");
  PLUSS_CHECK(!names_.empty(), "DataFrame::set(names): at least one feature name is required");
  std::unordered_set<std::string> seen;
  seen.reserve(names_.size());
  for (const auto& n : names_) {
    PLUSS_CHECK(!n.empty(), "DataFrame::set(names): feature name cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::set(names): duplicate feature name '" + n + "'");
  }
}

void DataFrame::LetSlots::assign_from_threads(std::span<const Thread> values) {
  PLUSS_CHECK(values.size() == names_.size(),
              "DataFrame::set(names): rhs feature count must match lhs feature count");

  std::optional<std::int64_t> expected_rows;
  if (owner_->nfeats() > 0) {
    expected_rows = owner_->ncases();
  }

  for (const auto& value : values) {
    PLUSS_CHECK(value.defined(), "DataFrame::set(names): rhs thread must be defined");
    if (!expected_rows.has_value()) {
      expected_rows = value.size();
    } else {
      PLUSS_CHECK(value.size() == *expected_rows,
                  "DataFrame::set(names): rhs thread lengths must match target DataFrame cases");
    }
  }

  for (std::size_t i = 0; i < names_.size(); ++i) {
    owner_->set_features_(names_[i], values[i]);
  }
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(const DataFrame& frame) {
  const auto cols = frame.features();
  assign_from_threads(std::span<const Thread>(cols.data(), cols.size()));
  return *this;
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(std::initializer_list<Thread> values) {
  assign_from_threads(std::span<const Thread>(values.begin(), values.size()));
  return *this;
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(const std::vector<Thread>& values) {
  assign_from_threads(std::span<const Thread>(values.data(), values.size()));
  return *this;
}

DataFrame::DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> cases) {
  std::vector<std::string> names;
  std::vector<Thread> values;
  names.reserve(columns.size());
  values.reserve(columns.size());
  for (auto& kv : columns) {
    names.push_back(std::move(kv.first));
    values.push_back(std::move(kv.second));
  }
  impl_ = pluss::msharp::make_intrusive<detail::DataFrameImpl>(std::move(names), std::move(values), std::move(cases));
}

DataFrame::DataFrame() = default;

DataFrame::DataFrame(pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl) : impl_(std::move(impl)) {}

DataFrame::DataFrame(const DataFrame& other) = default;
DataFrame::DataFrame(DataFrame&& other) noexcept = default;
DataFrame& DataFrame::operator=(const DataFrame& other) = default;
DataFrame& DataFrame::operator=(DataFrame&& other) noexcept = default;
DataFrame::~DataFrame() = default;

DataFrame DataFrame::load(std::string_view source_type, std::string_view filepath) {
  return load(source_type, filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          const pluss::loader::Config& config) {
  const auto source = lower_copy(owned_name(source_type));
  if (source == "csv") {
    return load_csv(filepath, config);
  }
  if (source == "txt" || source == "text") {
    return load_txt(filepath, config);
  }
  if (source == "excel" || source == "xlsx") {
    return load_excel(filepath, config);
  }
  if (source == "parquet") {
    return load_parquet(filepath, config);
  }
  if (source == "json") {
    return load_json(filepath, config);
  }
  PLUSS_FAIL("DataFrame::load: unsupported source type '" + std::string(source_type) + "'");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load(source_type, filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          const pluss::loader::ReadCsvOptions& options) {
  const auto source = lower_copy(owned_name(source_type));
  if (source == "csv") {
    return dataframe_from_raw_batch_padded(pluss::loader::read_csv_raw_batch(filepath, options), options);
  }
  if (source == "txt" || source == "text") {
    pluss::loader::Config config;
    config.emplace("header", options.header);
    config.emplace("sep", std::string(1, options.sep));
    if (options.ncases.has_value()) {
      config.emplace("ncases", static_cast<std::int64_t>(*options.ncases));
    }
    if (options.case_col.has_value()) {
      config.emplace("case", *options.case_col);
    } else if (options.case_col_idx.has_value()) {
      config.emplace("caseindex", static_cast<std::int64_t>(*options.case_col_idx));
    }
    if (!options.usefeats.empty()) {
      config.emplace("usefeats", options.usefeats);
    }
    if (!options.usefeats_idx.empty()) {
      std::vector<std::string> idx_tokens;
      idx_tokens.reserve(options.usefeats_idx.size());
      for (const auto idx : options.usefeats_idx) {
        idx_tokens.push_back(std::to_string(idx));
      }
      config.emplace("usefeatsidx", std::move(idx_tokens));
    }
    return dataframe_from_raw_batch_padded(pluss::loader::read_txt_raw_batch(filepath, config), options);
  }
  PLUSS_FAIL("DataFrame::load: unsupported source type '" + std::string(source_type) + "'");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> ncases,
                          std::vector<std::string> usefeats,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.ncases = ncases;
  options.usefeats = std::move(usefeats);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load(source_type, filepath, options);
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> ncases,
                          std::initializer_list<std::string_view> usefeats,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  return load(source_type, filepath, sep, ncases, normalize_name_list(usefeats), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  return dataframe_from_raw_batch_padded(pluss::loader::read_csv_raw_batch(filepath, options), options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::Config& config) {
  const auto options = pluss::loader::csv_options_from_config(config);
  return dataframe_from_raw_batch_padded(pluss::loader::read_csv_raw_batch(filepath, options), options);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load_csv(filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::string_view case_feature,
                              std::string_view engine,
                              std::optional<std::size_t> ncases) {
  pluss::loader::ReadCsvOptions options;
  options.header = !is_none_token(header);
  if (!is_none_token(case_feature)) {
    options.case_col = std::string(case_feature);
  }
  if (ncases.has_value()) {
    options.ncases = *ncases;
  }
  const auto eng = lower_copy(owned_name(engine));
  if (eng == "parallel") {
    options.engine = pluss::loader::CsvEngine::Parallel;
  } else if (eng == "serial") {
    options.engine = pluss::loader::CsvEngine::Serial;
  } else {
    options.engine = pluss::loader::CsvEngine::Auto;
  }
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::size_t ncases) {
  return load_csv(filepath, header, "none", "auto", ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> ncases,
                              std::vector<std::string> usefeats,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.ncases = ncases;
  options.usefeats = std::move(usefeats);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> ncases,
                              std::initializer_list<std::string_view> usefeats,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  return load_csv(filepath, sep, ncases, normalize_name_list(usefeats), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_txt(std::string_view filepath) {
  return load_txt(filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load_txt(std::string_view filepath, const pluss::loader::Config& config) {
  const auto infer_options = pluss::loader::csv_options_from_config(config);
  return dataframe_from_raw_batch_padded(pluss::loader::read_txt_raw_batch(filepath, config), infer_options);
}

DataFrame DataFrame::load_txt(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load_txt(filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load_excel(std::string_view filepath) {
  pluss::loader::ReadExcelOptions options;
  return load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = options.header;
  return dataframe_from_raw_batch_padded(pluss::loader::read_excel_raw_batch(filepath, options), infer_options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::Config& config) {
  auto xopts = pluss::loader::excel_options_from_config(config);
  auto infer_options = pluss::loader::csv_options_from_config(config);
  infer_options.header = xopts.header;
  return dataframe_from_raw_batch_padded(pluss::loader::read_excel_raw_batch(filepath, xopts), infer_options);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load_excel(filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::string_view sheet_name,
                                bool header) {
  const auto token = lower_copy(owned_name(sheet_name));
  PLUSS_CHECK(token != "none",
              "DataFrame::load_excel(sheet_name=\"none\") is workbook mode; use DataFrame::load_excel(filepath, nullptr, header)");
  pluss::loader::ReadExcelOptions options;
  options.header = header;
  options.sheet_name = std::string(sheet_name);
  return load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::int64_t sheet_index,
                                bool header) {
  pluss::loader::ReadExcelOptions options;
  options.header = header;
  options.sheet_index = sheet_index;
  return load_excel(filepath, options);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath, std::nullptr_t all_sheets, bool header) {
  (void)all_sheets;
  pluss::loader::ReadExcelOptions options;
  options.header = header;
  options.all_sheets = true;
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = header;
  auto wb_batch = pluss::loader::read_excel_workbook_raw_batch(filepath, options);
  pluss::loader::Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    dataframe_from_raw_batch_padded(batch, infer_options));
  }
  return pluss::loader::LoadResult(std::move(wb));
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                const std::vector<std::string>& sheet_names,
                                                bool header) {
  pluss::loader::ReadExcelOptions options;
  options.header = header;
  options.sheet_names = sheet_names;
  options.all_sheets = sheet_names.empty();
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = header;
  auto wb_batch = pluss::loader::read_excel_workbook_raw_batch(filepath, options);
  pluss::loader::Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    dataframe_from_raw_batch_padded(batch, infer_options));
  }
  return pluss::loader::LoadResult(std::move(wb));
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                std::initializer_list<std::string_view> sheet_names,
                                                bool header) {
  std::vector<std::string> names;
  names.reserve(sheet_names.size());
  for (const auto sv : sheet_names) {
    names.emplace_back(sv);
  }
  return load_excel(filepath, names, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                const std::vector<std::int64_t>& sheet_indices,
                                                bool header) {
  pluss::loader::ReadExcelOptions options;
  options.header = header;
  options.sheet_indices = sheet_indices;
  options.all_sheets = sheet_indices.empty();
  pluss::loader::ReadCsvOptions infer_options;
  infer_options.header = header;
  auto wb_batch = pluss::loader::read_excel_workbook_raw_batch(filepath, options);
  pluss::loader::Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    dataframe_from_raw_batch_padded(batch, infer_options));
  }
  return pluss::loader::LoadResult(std::move(wb));
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                std::initializer_list<std::int64_t> sheet_indices,
                                                bool header) {
  std::vector<std::int64_t> indices;
  indices.reserve(sheet_indices.size());
  for (const auto idx : sheet_indices) {
    indices.push_back(idx);
  }
  return load_excel(filepath, indices, header);
}

DataFrame DataFrame::load_parquet(std::string_view filepath) {
  return load_parquet(filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load_parquet(std::string_view filepath, const pluss::loader::Config& config) {
  auto infer_options = pluss::loader::csv_options_from_config(config);
  return dataframe_from_raw_batch_padded(pluss::loader::read_parquet_raw_batch(filepath, config), infer_options);
}

DataFrame DataFrame::load_parquet(std::string_view filepath,
                                  std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load_parquet(filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::load_json(std::string_view filepath) {
  return load_json(filepath, pluss::loader::Config{});
}

DataFrame DataFrame::load_json(std::string_view filepath, const pluss::loader::Config& config) {
  const auto infer_options = pluss::loader::csv_options_from_config(config);
  return dataframe_from_raw_batch_padded(pluss::loader::read_json_raw_batch(filepath, config), infer_options);
}

DataFrame DataFrame::load_json(std::string_view filepath,
                               std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return load_json(filepath, pluss::loader::config_from(config_entries));
}

DataFrame DataFrame::tabelize(const pluss::kindle::Catalog& catalog) {
  return tabelize(catalog, pluss::kindle::TabelizePolicy{});
}

DataFrame DataFrame::tabelize(const pluss::kindle::Catalog& catalog,
                              const pluss::kindle::TabelizePolicy& policy) {
  PLUSS_CHECK(catalog.defined(), "DataFrame::tabelize requires a defined Catalog");
  return catalog.tabelize(policy);
}

pluss::kindle::Catalog DataFrame::kindelize() const {
  return kindelize(pluss::kindle::KindelizePolicy{});
}

pluss::kindle::Catalog DataFrame::kindelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "DataFrame::kindelize requires a defined DataFrame");
  return pluss::kindle::Catalog::kindelize(*this, policy);
}
