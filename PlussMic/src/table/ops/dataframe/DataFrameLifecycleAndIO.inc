ColumnRef::ColumnRef(std::string name, Slate column)
    : name_(std::move(name)), column_(std::move(column)) {}

const std::string& ColumnRef::name() const noexcept { return name_; }
const Slate& ColumnRef::slate() const noexcept { return column_; }
pluss::Series ColumnRef::mean() const { return column_.mean(); }
Slate ColumnRef::head(std::int64_t n) const { return column_.head(n, false); }
SlateValue ColumnRef::value() const { return column_.value(); }
Slate ColumnRef::values() const { return column_; }

DataFrame::LetSlot::LetSlot(DataFrame& owner, std::string name, AlignOptions options)
    : owner_(&owner), name_(std::move(name)), options_(options) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlot: owner must be defined");
  PLUSS_CHECK(!name_.empty(), "DataFrame::set: feature name cannot be empty");
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const Slate& value) {
  set_prepared_feature_(
      owner_->mut_impl_(),
      name_,
      prepare_feature_assignment_(owner_->impl_ref_(), value, "DataFrame::set(name)=Slate", options_));
  return *this;
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const SlateValue& value) {
  const auto& src = owner_->impl_ref_();
  set_prepared_feature_(
      owner_->mut_impl_(),
      name_,
      (src.columns_.empty() && src.nrows_ == 0)
          ? clear_assignment_metadata_(value.slate())
          : broadcast_assignment_slate_(
                value.slate(), src.nrows_,
                "DataFrame::set(name)=SlateValue"));
  return *this;
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::Series& value) {
  try {
    if (value.dim() == 0) {
      return (*this = SlateValue(Slate(value.reshape({1}))));
    }
    return (*this = Slate(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=Series failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::Note& value) {
  try {
    if (value.dim() == 0) {
      return (*this = SlateValue(Slate(pluss::make_note(
          std::vector<std::string>{std::string(value.scalar())}))));
    }
    return (*this = Slate(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=Note failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const pluss::Timeline& value) {
  try {
    return (*this = Slate(value));
  } catch (const std::exception& e) {
    PLUSS_FAIL("DataFrame::set(name)=Timeline failed: " + std::string(e.what()));
  }
}

DataFrame::LetSlot& DataFrame::LetSlot::operator=(const DataFrame& frame) {
  PLUSS_CHECK(frame.nfeatures() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
  const auto& names = frame.features();
  PLUSS_CHECK(names.size() == 1, "DataFrame::set(name)=DataFrame requires rhs with exactly one feature");
  return (*this = detail::with_display_labels(frame.features(names.front()), frame.labels().values()));
}

DataFrame::LetSlots::LetSlots(DataFrame& owner, std::vector<std::string> names, AlignOptions options)
    : owner_(&owner), names_(std::move(names)), options_(options) {
  PLUSS_CHECK(owner_ != nullptr, "DataFrame::LetSlots: owner must be defined");
  PLUSS_CHECK(!names_.empty(), "DataFrame::set(names): at least one feature name is required");
  std::unordered_set<std::string> seen;
  seen.reserve(names_.size());
  for (const auto& n : names_) {
    PLUSS_CHECK(!n.empty(), "DataFrame::set(names): feature name cannot be empty");
    PLUSS_CHECK(seen.insert(n).second, "DataFrame::set(names): duplicate feature name '" + n + "'");
  }
}

void DataFrame::LetSlots::assign_from_slates(std::span<const Slate> values) {
  PLUSS_CHECK(values.size() == names_.size(),
              "DataFrame::set(names): rhs feature count must match lhs feature count");

  const auto& src = owner_->impl_ref_();
  std::optional<std::int64_t> expected_rows;
  if (!src.columns_.empty() || src.nrows_ > 0) {
    expected_rows = src.nrows_;
  }
  std::vector<Slate> prepared_values;
  prepared_values.reserve(values.size());

  for (const auto& value : values) {
    PLUSS_CHECK(value.defined(), "DataFrame::set(names): rhs slate must be defined");
    auto prepared = prepare_feature_assignment_(
        src, value, "DataFrame::set(names)", options_);
    if (!expected_rows.has_value()) {
      expected_rows = prepared.size();
    } else {
      PLUSS_CHECK(prepared.size() == *expected_rows,
                  "DataFrame::set(names): rhs slate lengths must match target DataFrame cases");
    }
    prepared_values.push_back(std::move(prepared));
  }

  // Batch-apply updates with at most one reindex/validation pass.
  // This avoids O(k * reindex) behavior when assigning many features at once.
  auto& w = owner_->mut_impl_();
  bool touched_schema = (w.columns_.empty() && !values.empty());
  for (std::size_t i = 0; i < names_.size(); ++i) {
    const auto& key = names_[i];
    const auto it = w.name_to_idx_.find(key);
    if (it == w.name_to_idx_.end()) {
      w.names_.push_back(key);
      w.columns_.push_back(prepared_values[i]);
      touched_schema = true;
    } else {
      w.columns_[it->second] = prepared_values[i];
    }
  }
  if (touched_schema) {
    w.validate_and_reindex_();
  }
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(const DataFrame& frame) {
  std::vector<Slate> cols;
  const auto& names = frame.features();
  cols.reserve(names.size());
  for (const auto& name : names) {
    cols.push_back(detail::with_display_labels(frame.features(name), frame.labels().values()));
  }
  assign_from_slates(std::span<const Slate>(cols.data(), cols.size()));
  return *this;
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(std::initializer_list<Slate> values) {
  assign_from_slates(std::span<const Slate>(values.begin(), values.size()));
  return *this;
}

DataFrame::LetSlots& DataFrame::LetSlots::operator=(const std::vector<Slate>& values) {
  assign_from_slates(std::span<const Slate>(values.data(), values.size()));
  return *this;
}

DataFrame::DataFrame(std::vector<std::pair<std::string, Slate>> columns,
                     std::optional<Slate> cases) {
  std::vector<std::string> names;
  std::vector<Slate> values;
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
    const auto raw = pluss::loader::read_csv_raw_batch(filepath, options);
    auto batch = detail::csv_batch_from_raw_batch(raw, options);
    return DataFrame(std::move(batch.features), std::move(batch.cases));
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
                          pluss::msharp::RangeView::Of<std::string_view> usefeats,
                          std::optional<std::string> case_col,
                          pluss::loader::BadLinePolicy on_bad_lines) {
  return load(source_type, filepath, sep, ncases, normalize_name_list(usefeats), std::move(case_col), on_bad_lines);
}

DataFrame DataFrame::load_csv(std::string_view filepath) {
  pluss::loader::ReadCsvOptions options;
  return load_csv(filepath, options);
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) {
  const auto raw = pluss::loader::read_csv_raw_batch(filepath, options);
  auto batch = detail::csv_batch_from_raw_batch(raw, options);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
}

DataFrame DataFrame::load_csv(std::string_view filepath, const pluss::loader::Config& config) {
  const auto options = pluss::loader::csv_options_from_config(config);
  const auto raw = pluss::loader::read_csv_raw_batch(filepath, config);
  auto batch = detail::csv_batch_from_raw_batch(raw, options);
  return DataFrame(std::move(batch.features), std::move(batch.cases));
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
                              pluss::msharp::RangeView::Of<std::string_view> usefeats,
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
  return load_excel(filepath, pluss::msharp::RangeView::Of<std::string_view>(sheet_names), header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                pluss::msharp::RangeView::Of<std::string_view> sheet_names,
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
  return load_excel(filepath, pluss::msharp::RangeView::Int(sheet_indices), header);
}

pluss::loader::LoadResult DataFrame::load_excel(std::string_view filepath,
                                                pluss::msharp::RangeView::Int sheet_indices,
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

pluss::kindle::Catalog DataFrame::kindelize() const {
  return kindelize(pluss::kindle::KindelizePolicy{});
}

pluss::kindle::Catalog DataFrame::kindelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "DataFrame::kindelize requires a defined DataFrame");
  std::vector<std::pair<std::string, pluss::kindle::Stream>> out;
  out.reserve(static_cast<std::size_t>(nfeatures()) + (policy.materialize_label ? 1u : 0u));

  const auto case_axis = labels().values();
  for (const auto& key : features()) {
    auto stream = pluss::kindle::Stream::from_slate(
        this->features(key),
        policy.attach_labels ? std::optional<Slate>{case_axis} : std::nullopt);
    out.emplace_back(key, std::move(stream));
  }

  if (policy.materialize_label) {
    PLUSS_CHECK(features().end() == std::find(features().begin(), features().end(), policy.label_key),
                "DataFrame::kindelize: label key collides with existing feature '" + policy.label_key + "'");
    out.emplace_back(policy.label_key, pluss::kindle::Stream::from_slate(case_axis));
  }

  return pluss::kindle::Catalog(std::move(out));
}

pluss::scribble::Registry DataFrame::scribbelize() const {
  return scribbelize(pluss::kindle::KindelizePolicy{});
}

pluss::scribble::Registry DataFrame::scribbelize(const pluss::kindle::KindelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "DataFrame::scribbelize requires a defined DataFrame");
  return kindelize(policy).scribbelize();
}
