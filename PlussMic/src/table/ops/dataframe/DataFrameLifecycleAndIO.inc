ColumnRef::ColumnRef(std::string name, Thread column)
    : name_(std::move(name)), column_(std::move(column)) {}

const std::string& ColumnRef::name() const noexcept { return name_; }
const Thread& ColumnRef::thread() const noexcept { return column_; }
pluss::Series ColumnRef::mean() const { return column_.mean(); }
Thread ColumnRef::head(std::int64_t n) const { return column_.head(n); }
ThreadValue ColumnRef::value() const { return column_.value(); }
Thread ColumnRef::values() const { return column_; }

DataFrame::LetSlot::LetSlot(DataFrame& owner, std::string name) : owner_(&owner), name_(std::move(name)) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlot: owner must be defined");
  PLUSS_CHECK(!name_.empty(), "DataFrame::set: feature name cannot be empty");
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const Thread& value) {
  PLUSS_CHECK(value.defined(), "DataFrame::set(name)=Thread requires defined rhs thread");
  if (owner_->ncols() > 0) {
    PLUSS_CHECK(value.size() == owner_->nrows(),
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
  PLUSS_CHECK(frame.ncols() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
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
  if (owner_->ncols() > 0) {
    expected_rows = owner_->nrows();
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
  if (source == "excel" || source == "xlsx") {
    return load_excel(filepath, config);
  }
  if (source == "parquet") {
    return load_parquet(filepath, config);
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
    auto batch = pluss::loader::read_csv_batch(filepath, options);
    return DataFrame(std::move(batch.features), std::move(batch.cases));
  }
  PLUSS_FAIL("DataFrame::load: unsupported source type '" + std::string(source_type) + "'");
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> nrows,
                          std::vector<std::string> usecols,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load(source_type, filepath, options);
}

DataFrame DataFrame::load(std::string_view source_type,
                          std::string_view filepath,
                          char sep,
                          std::optional<std::size_t> nrows,
                          std::initializer_list<std::string_view> usecols,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  return load(source_type, filepath, sep, nrows, normalize_name_list(usecols), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  auto batch = pluss::loader::read_csv_batch(filepath, options);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::Config& config) {
  auto batch = pluss::loader::read_csv_batch(filepath, config);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  auto batch = pluss::loader::read_csv_batch(filepath, config_entries);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::string_view case_feature,
                              std::string_view engine,
                              std::optional<std::size_t> ncases) {
  return pluss::loader::load_csv(filepath, header, case_feature, engine, ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              std::string_view header,
                              std::size_t ncases) {
  return pluss::loader::load_csv(filepath, header, ncases);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> nrows,
                              std::vector<std::string> usecols,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  pluss::loader::ReadCsvOptions options;
  options.sep = sep;
  options.nrows = nrows;
  options.usecols = std::move(usecols);
  options.case_col = std::move(case_col);
  options.on_bad_lines = on_bad_lines;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath,
                              char sep,
                              std::optional<std::size_t> nrows,
                              std::initializer_list<std::string_view> usecols,
                              std::optional<std::string> case_col,
                              pluss::loader::BadLinePolicy on_bad_lines) {
  return load_csv(filepath, sep, nrows, normalize_name_list(usecols), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_excel(std::string_view filepath) {
  pluss::loader::ReadExcelOptions options;
  return load_excel(filepath, options);
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) {
  auto batch = pluss::loader::read_excel_batch(filepath, options);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_excel(std::string_view filepath, const pluss::loader::Config& config) {
  auto batch = pluss::loader::read_excel_batch(filepath, config);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  auto batch = pluss::loader::read_excel_batch(filepath, config_entries);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::string_view sheet_name,
                                bool header) {
  const auto token = lower_copy(owned_name(sheet_name));
  PLUSS_CHECK(token != "none",
              "DataFrame::load_excel(sheet_name=\"none\") is workbook mode; use DataFrame::load_excel(filepath, nullptr, header)");
  return pluss::loader::load_excel(filepath, sheet_name, header);
}

DataFrame DataFrame::load_excel(std::string_view filepath,
                                std::int64_t sheet_index,
                                bool header) {
  return pluss::loader::load_excel(filepath, sheet_index, header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath, std::nullptr_t all_sheets, bool header) {
  (void)all_sheets;
  pluss::loader::ReadExcelOptions options;
  options.header = header;
  options.all_sheets = true;
  auto wb_batch = pluss::loader::read_excel_workbook_batch(filepath, options);
  pluss::loader::Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    DataFrame(std::move(batch.features), std::move(batch.cases)));
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
  auto wb_batch = pluss::loader::read_excel_workbook_batch(filepath, options);
  pluss::loader::Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    DataFrame(std::move(batch.features), std::move(batch.cases)));
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
  auto wb_batch = pluss::loader::read_excel_workbook_batch(filepath, options);
  pluss::loader::Workbook wb;
  wb.reserve(wb_batch.size());
  for (auto& [sheet_name, batch] : wb_batch) {
    wb.emplace_back(std::move(sheet_name),
                    DataFrame(std::move(batch.features), std::move(batch.cases)));
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
  return pluss::loader::load_parquet(filepath, config);
}

DataFrame DataFrame::load_parquet(std::string_view filepath,
                                  std::initializer_list<pluss::loader::ConfigEntry> config_entries) {
  return pluss::loader::load_parquet(filepath, config_entries);
}

pluss::kindle::Catalog DataFrame::kindelize() const {
  return kindelize(pluss::kindle::KindelizePolicy{});
}

pluss::kindle::Catalog DataFrame::kindelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "DataFrame::kindelize requires a defined DataFrame");
  return pluss::kindle::Catalog::kindelize(*this, policy);
}
