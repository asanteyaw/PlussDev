namespace {

template <class Operation>
DataFrame transform_text_features_(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
    Operation&& operation,
    std::string_view where) {
  std::unordered_set<std::string> selected;
  selected.reserve(features.size());
  for (const auto name_view : features) {
    const auto name = owned_name(name_view);
    PLUSS_CHECK(!name.empty(),
                std::string(where) + ": feature cannot be empty");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(),
                std::string(where) + ": unknown feature '" + name + "'");
    PLUSS_CHECK(src.columns_[it->second].kind() == SlateKind::Note,
                std::string(where) + ": feature '" + name +
                    "' is not text");
    PLUSS_CHECK(selected.insert(name).second,
                std::string(where) + ": repeated feature '" + name + "'");
  }
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    const bool apply =
        features.empty() ? source.kind() == SlateKind::Note
                         : selected.contains(src.names_[i]);
    columns.emplace_back(
        src.names_[i], apply ? operation(source) : source);
  }
  return DataFrame(std::move(columns), src.cases_).title(src.title_);
}

template <class Operation>
DataFrame transform_text_features_(
    const detail::DataFrameImpl& src,
    const std::vector<std::string>& features,
    Operation&& operation,
    std::string_view where) {
  std::vector<std::string_view> views;
  views.reserve(features.size());
  for (const auto& feature : features) {
    views.push_back(feature);
  }
  return transform_text_features_(
      src,
      pluss::msharp::RangeView::Of<std::string_view>(views),
      std::forward<Operation>(operation),
      where);
}

DataFrame expanded_text_result_(const DataFrame& expanded,
                                const Slate& cases,
                                std::string title) {
  std::vector<std::pair<std::string, Slate>> columns;
  const auto names = expanded.features();
  columns.reserve(names.size());
  for (const auto& name : names) {
    columns.emplace_back(name, expanded.features(name));
  }
  return DataFrame(std::move(columns), cases).title(std::move(title));
}

bool is_time_like_slate_(SlateKind kind) {
  return kind == SlateKind::Timeline;
}

std::vector<std::size_t> resolve_time_feature_indices_(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::string_view where) {
  std::vector<std::size_t> indices;
  if (features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      PLUSS_CHECK(is_time_like_slate_(src.columns_[i].kind()),
                  std::string(where) + ": no features supplied; current frame contains non-Timeline feature '" +
                      src.names_[i] + "'. Select timeline feature(s) first, e.g. df.features({\"date\"}).tl()");
      indices.push_back(i);
    }
    PLUSS_CHECK(!indices.empty(), std::string(where) + ": current frame has no Timeline features");
    return indices;
  }

  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  indices.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate feature '" + name + "'");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown feature '" + name + "'");
    const auto k = src.columns_[it->second].kind();
    PLUSS_CHECK(is_time_like_slate_(k),
                std::string(where) + ": feature '" + name + "' is not Timeline");
    indices.push_back(it->second);
  }
  return indices;
}

std::vector<std::size_t> resolve_shift_feature_indices_(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::string_view where) {
  std::vector<std::size_t> indices;
  if (features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      indices.push_back(i);
    }
    return indices;
  }

  std::unordered_set<std::string> seen;
  seen.reserve(features.size());
  indices.reserve(features.size());
  for (const auto name_sv : features) {
    const auto name = owned_name(name_sv);
    PLUSS_CHECK(!name.empty(), std::string(where) + ": feature name cannot be empty");
    PLUSS_CHECK(seen.insert(name).second, std::string(where) + ": duplicate feature '" + name + "'");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown feature '" + name + "'");
    indices.push_back(it->second);
  }
  return indices;
}

pluss::Series bool_series_from_slate_mask_(
    const Slate& bool_mask,
    std::int64_t expected_size,
    std::string_view where) {
  PLUSS_CHECK(bool_mask.kind() == SlateKind::Series, std::string(where) + ": mask must be a Series slate");
  auto mask = bool_mask.as_series();
  validate_bool_mask(mask, expected_size, where);
  if (detail::has_validity_mask(bool_mask)) {
    auto valid = *detail::validity_mask(bool_mask);
    if (valid.platform() != mask.platform()) {
      valid = valid.to(mask.platform());
    }
    mask = mask.logical_and(valid);
  }
  return mask;
}

} // namespace

DataFrameView DataFrame::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, ncases());
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Slate>> cols;
  cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    cols.emplace_back(src.names_[i], src.columns_[i].head(clipped, /*render=*/false));
  }
  auto out = DataFrame(std::move(cols), detail::clear_display_labels(src.cases_.head(clipped, /*render=*/false)))
                 .title(title());
  return DataFrameView(std::move(out), render, "head", title());
}

DataFrameView DataFrame::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, ncases());
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Slate>> cols;
  cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    cols.emplace_back(src.names_[i], src.columns_[i].tail(clipped, /*render=*/false));
  }
  auto out = DataFrame(std::move(cols), detail::clear_display_labels(src.cases_.tail(clipped, /*render=*/false)))
                 .title(title());
  return DataFrameView(std::move(out), render, "tail", title());
}

DataFrameView DataFrame::observe(std::int64_t row, bool render) const {
  const auto idx = normalize_row_index(row, ncases());
  const auto& names = features();
  const auto dt = dtypes();

  std::vector<std::string> dtype_values;
  std::vector<std::string> cell_values;
  dtype_values.reserve(names.size());
  cell_values.reserve(names.size());

  for (const auto& name : names) {
    const auto col = features(name);
    dtype_values.push_back(dt.contains(name) ? dt.at(name) : col.dtype());
    if (detail::valid_at(col, idx)) {
      cell_values.emplace_back(col.cast().note().as_note().at(idx));
    } else {
      cell_values.emplace_back("<null>");
    }
  }

  std::vector<std::pair<std::string, Slate>> out_cols;
  out_cols.reserve(2);
  out_cols.emplace_back("dtype", Slate(pluss::make_note(dtype_values)));
  out_cols.emplace_back("value", Slate(pluss::make_note(cell_values)));

  auto out = DataFrame(std::move(out_cols), Slate(pluss::make_note(names))).title(title());
  return DataFrameView(std::move(out), render, "observe", title());
}

namespace {

std::vector<std::int64_t> sample_row_indices_(std::int64_t total,
                                              std::int64_t draw,
                                              bool replace,
                                              std::mt19937_64& rng) {
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(draw));
  if (draw == 0 || total == 0) {
    return idx;
  }
  if (replace) {
    std::uniform_int_distribution<std::int64_t> dist(0, total - 1);
    for (std::int64_t i = 0; i < draw; ++i) {
      idx.push_back(dist(rng));
    }
    return idx;
  }

  idx.resize(static_cast<std::size_t>(total));
  std::iota(idx.begin(), idx.end(), 0);
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(draw));
  return idx;
}

std::vector<std::size_t> sample_feature_indices_(std::size_t total, std::size_t draw, std::mt19937_64& rng) {
  std::vector<std::size_t> idx(total);
  std::iota(idx.begin(), idx.end(), 0);
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(draw);
  return idx;
}

DataFrame take_feature_indices_(const DataFrame& frame, std::span<const std::size_t> indices) {
  const auto& all_names = frame.features();
  std::vector<std::string_view> selected;
  selected.reserve(indices.size());
  for (const auto idx : indices) {
    selected.push_back(all_names[idx]);
  }
  return frame.features(
      pluss::msharp::RangeView::Of<std::string_view>(
          std::span<const std::string_view>(selected.data(), selected.size())));
}

} // namespace

DataFrame DataFrame::sample(std::int64_t n) const {
  return sample_impl_(n, std::nullopt, false, Axis::Case);
}

DataFrame DataFrame::sample(std::int64_t n, bool replace, Axis axis) const {
  return sample_impl_(n, std::nullopt, replace, axis);
}

DataFrame DataFrame::sample(std::int64_t n, std::uint64_t random_state, bool replace, Axis axis) const {
  return sample_impl_(n, random_state, replace, axis);
}

DataFrame DataFrame::sample_impl_(std::int64_t n,
                                  std::optional<std::uint64_t> random_state,
                                  bool replace,
                                  Axis axis) const {
  PLUSS_CHECK(n >= 0, "DataFrame::sample: n must be >= 0");
  const auto& src = impl_ref_();
  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));

  if (axis == Axis::Case) {
    const auto total = ncases();
    PLUSS_CHECK(total >= 0, "DataFrame::sample: invalid case count");
    const auto draw = replace ? n : std::min<std::int64_t>(n, total);
    return take(sample_row_indices_(total, draw, replace, rng));
  }

  PLUSS_CHECK(!replace, "DataFrame::sample: feature sampling with replacement is not supported");
  const auto total = src.columns_.size();
  const auto draw = std::min<std::size_t>(static_cast<std::size_t>(n), total);
  const auto order = sample_feature_indices_(total, draw, rng);
  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(draw);
  out_cols.reserve(draw);
  for (const auto idx : order) {
    out_names.push_back(src.names_[idx]);
    out_cols.push_back(src.columns_[idx]);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::sample_fraction_(double frac,
                                      std::optional<std::uint64_t> random_state,
                                      bool replace,
                                      Axis axis) const {
  PLUSS_CHECK(std::isfinite(frac), "DataFrame::sample: frac must be finite");
  PLUSS_CHECK(frac >= 0.0, "DataFrame::sample: frac must be >= 0");
  PLUSS_CHECK(replace || frac <= 1.0, "DataFrame::sample: frac > 1 requires replace = true");
  const auto total = axis == Axis::Case ? ncases() : nfeatures();
  const auto draw = static_cast<std::int64_t>(std::llround(frac * static_cast<double>(total)));
  return sample_impl_(draw, random_state, replace, axis);
}

DataFrame DataFrame::shuffle(std::optional<std::uint64_t> random_state) const {
  return reshuffle(Axis::Case, random_state);
}

DataFrame DataFrame::reshuffle(Axis axis, std::optional<std::uint64_t> random_state) const {
  const auto& src = impl_ref_();
  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));

  if (axis == Axis::Case) {
    std::vector<std::int64_t> idx(static_cast<std::size_t>(src.nrows_));
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);
    return take(idx);
  }

  if (src.columns_.empty()) {
    return *this;
  }

  std::vector<std::size_t> order(src.columns_.size());
  std::iota(order.begin(), order.end(), 0);
  std::shuffle(order.begin(), order.end(), rng);

  std::vector<std::string> out_names;
  std::vector<Slate> out_cols;
  out_names.reserve(order.size());
  out_cols.reserve(order.size());
  for (const auto idx : order) {
    out_names.push_back(src.names_[idx]);
    out_cols.push_back(src.columns_[idx]);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::shift(std::int64_t periods) const {
  return shift({}, periods);
}

DataFrame DataFrame::shift(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t periods) const {
  const auto& src = impl_ref_();
  if (src.columns_.empty() || src.nrows_ == 0 || periods == 0) {
    return *this;
  }

  const auto indices = resolve_shift_feature_indices_(src, features, "DataFrame::shift");
  if (indices.empty()) {
    return *this;
  }

  auto out_cols = src.columns_;
  for (const auto idx : indices) {
    out_cols[idx] = src.columns_[idx].shift(periods);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, ncases());
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    idx.push_back(i);
  }
  return take(idx);
}

DataFrame DataFrame::zoom(std::int64_t case_start,
                          std::int64_t case_stop,
                          std::int64_t feature_start,
                          std::int64_t feature_stop) const {
  return select_cases_(detail::CaseSelector::slice(case_start, case_stop),
                       detail::FeatureSelector::slice(feature_start, feature_stop));
}

DataFrame DataFrame::take(pluss::msharp::RangeView::Int indices) const {
  const auto& src = impl_ref_();
  if (indices.size() == static_cast<std::size_t>(src.nrows_)) {
    bool is_identity = true;
    for (std::size_t i = 0; i < indices.size(); ++i) {
      if (indices[i] != static_cast<std::int64_t>(i)) {
        is_identity = false;
        break;
      }
    }
    if (is_identity) {
      return *this;
    }
  }
  std::vector<Slate> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.take(indices));
  }
  auto out_case = src.cases_.take(indices);
  auto out_axis = src.case_axis_.defined() ? take_case_axis_(src.case_axis_, indices) : detail::CaseAxis{};
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), std::move(out_case), src.title_, std::move(out_axis)));
}

DataFrame DataFrame::take(const Slate& indices) const {
  PLUSS_CHECK(indices.defined(), "DataFrame::take: indices slate is undefined");
  PLUSS_CHECK(indices.kind() == SlateKind::Series, "DataFrame::take: indices slate must be a Series");
  const auto values = indices.as_series();
  PLUSS_CHECK(values.dim() == 1, "DataFrame::take: indices slate must be 1D");
  PLUSS_CHECK(
      values.scalar_type() == pluss::codem::ScalarType::Int64
          || values.scalar_type() == pluss::codem::ScalarType::Int32
          || values.scalar_type() == pluss::codem::ScalarType::UInt64
          || values.scalar_type() == pluss::codem::ScalarType::UInt32
          || values.scalar_type() == pluss::codem::ScalarType::UInt16
          || values.scalar_type() == pluss::codem::ScalarType::UInt8
          || values.scalar_type() == pluss::codem::ScalarType::Int16
          || values.scalar_type() == pluss::codem::ScalarType::Int8,
      "DataFrame::take: indices slate must use an integer Series dtype");
  return take(values.to_vec<std::int64_t>());
}

std::pair<DataFrame, DataFrame> DataFrame::split(double frac) const {
  return split(frac, SplitOptions{});
}

std::pair<DataFrame, DataFrame> DataFrame::split(double frac,
                                                 const SplitOptions& options) const {
  PLUSS_CHECK(std::isfinite(frac), "DataFrame::split: frac must be finite");
  PLUSS_CHECK(frac >= 0.0 && frac <= 1.0, "DataFrame::split: frac must be in [0, 1]");

  if (options.axis == Axis::Case) {
    const auto total = ncases();
    const auto rhs_count = static_cast<std::int64_t>(std::llround(frac * static_cast<double>(total)));
    const auto lhs_count = total - rhs_count;
    if (!options.shuffle) {
      return {slice(0, lhs_count), slice(lhs_count, total)};
    }

    std::mt19937_64 rng(options.random_state.value_or(std::random_device{}()));
    std::vector<std::int64_t> order(static_cast<std::size_t>(total));
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);
    std::vector<std::int64_t> lhs_indices(order.begin(), order.begin() + lhs_count);
    std::vector<std::int64_t> rhs_indices(order.begin() + lhs_count, order.end());
    return {take(lhs_indices), take(rhs_indices)};
  }

  const auto total = nfeatures();
  const auto rhs_count = static_cast<std::int64_t>(std::llround(frac * static_cast<double>(total)));
  const auto lhs_count = total - rhs_count;
  if (!options.shuffle) {
    std::vector<std::size_t> lhs_indices(static_cast<std::size_t>(lhs_count));
    std::vector<std::size_t> rhs_indices(static_cast<std::size_t>(rhs_count));
    std::iota(lhs_indices.begin(), lhs_indices.end(), 0);
    std::iota(rhs_indices.begin(), rhs_indices.end(), static_cast<std::size_t>(lhs_count));
    return {take_feature_indices_(*this, lhs_indices), take_feature_indices_(*this, rhs_indices)};
  }

  std::mt19937_64 rng(options.random_state.value_or(std::random_device{}()));
  auto order = sample_feature_indices_(static_cast<std::size_t>(total), static_cast<std::size_t>(total), rng);
  std::vector<std::size_t> lhs_indices(order.begin(), order.begin() + lhs_count);
  std::vector<std::size_t> rhs_indices(order.begin() + lhs_count, order.end());
  return {take_feature_indices_(*this, lhs_indices), take_feature_indices_(*this, rhs_indices)};
}

std::pair<DataFrame, DataFrame> DataFrame::split(double frac,
                                                 Axis axis,
                                                 bool shuffle) const {
  return split(frac, SplitOptions{
                       .axis = axis,
                       .shuffle = shuffle,
                       .random_state = std::nullopt,
                   });
}

std::pair<DataFrame, DataFrame> DataFrame::split(double frac,
                                                 std::uint64_t random_state,
                                                 Axis axis,
                                                 bool shuffle) const {
  return split(frac, SplitOptions{
                       .axis = axis,
                       .shuffle = shuffle,
                       .random_state = random_state,
                   });
}

DataFrame DataFrame::where(const pluss::Series& bool_mask) const {
  return filter(bool_mask);
}

DataFrame DataFrame::where(const Slate& bool_mask) const {
  return filter(bool_mask);
}

DataFrame DataFrame::where(const Slate& bool_mask, const AlignOptions& options) const {
  return filter(bool_mask, options);
}

DataFrame DataFrame::filter(const pluss::Series& bool_mask) const {
  validate_bool_mask(bool_mask, ncases(), "DataFrame::filter");
  const auto& src = impl_ref_();
  std::vector<Slate> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.filter(bool_mask));
  }
  auto out_case = src.cases_.filter(bool_mask);
  auto out_axis = src.case_axis_.defined() ? filter_case_axis_(src.case_axis_, bool_mask) : detail::CaseAxis{};
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), std::move(out_case), src.title_, std::move(out_axis)));
}

DataFrame DataFrame::filter(const Slate& bool_mask) const {
  return filter(bool_mask, row_mask_default_options_());
}

DataFrame DataFrame::filter(const Slate& bool_mask, const AlignOptions& options) const {
  const auto aligned_mask = prepare_mask_slate_(impl_ref_(), bool_mask, "DataFrame::filter", options);
  return filter(bool_series_from_slate_mask_(aligned_mask, ncases(), "DataFrame::filter"));
}

DataFrame DataFrame::query(const pluss::Series& bool_mask) const {
  return where(bool_mask);
}

DataFrame DataFrame::query(const QueryFn& predicate) const {
  PLUSS_CHECK(static_cast<bool>(predicate), "DataFrame::query: predicate is empty");
  return where(predicate(*this));
}

DataFrame DataFrame::query(std::string_view expression) const {
  const auto text = owned_name(expression);
  PLUSS_CHECK(!text.empty(), "DataFrame::query: expression cannot be empty");
  auto parser = QueryParser(*this, tokenize_query(text));
  auto eval = parser.parse();

  auto mask = pluss::Series::zeros({ncases()}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < ncases(); ++i) {
    keep[i] = eval(i);
  }
  return where(mask);
}

DataFrame DataFrame::assign(pluss::msharp::RangeView::Of<std::pair<std::string, Slate>> assignments) const {
  DataFrame out = *this;
  for (const auto& assignment : assignments) {
    PLUSS_CHECK(!assignment.first.empty(), "DataFrame::assign: feature name cannot be empty");
    out = out.set_features(assignment.first, assignment.second);
  }
  return out;
}

DataFrame DataFrame::assign(pluss::msharp::RangeView::Of<DeriveSpec> specs) const {
  DataFrame out = *this;
  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::assign: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::assign: assign expression is empty");
    out = out.set_features(spec.feature, spec.expr(out));
  }
  return out;
}

DataFrame DataFrame::assign(std::string_view name, const Slate& value) const {
  return assign({std::pair<std::string, Slate>{std::string(name), value}});
}

DataFrame DataFrame::assign(std::string_view name, const std::function<Slate(const DataFrame&)>& expr) const {
  return assign({DeriveSpec{.feature = std::string(name), .expr = expr}});
}

Slate DataFrame::transform(
    const std::function<SlateValue(const DataFrame&)>& fn,
    Axis axis) const {
  PLUSS_CHECK(axis == Axis::Case,
              "DataFrame::transform(SlateValue callback): use Axis::Case for row-wise scalar transforms");
  PLUSS_CHECK(static_cast<bool>(fn),
              "DataFrame::transform: callback is empty");
  std::vector<Slate> mapped;
  mapped.reserve(static_cast<std::size_t>(ncases()));
  for (std::int64_t row = 0; row < ncases(); ++row) {
    auto value = fn(slice(row, row + 1));
    PLUSS_CHECK(value.defined() && value.slate().size() == 1,
                "DataFrame::transform: callback must return one value");
    if (!mapped.empty()) {
      PLUSS_CHECK(value.kind() == mapped.front().kind(),
                  "DataFrame::transform: output kinds must be consistent");
    }
    mapped.push_back(value.slate());
  }
  if (mapped.empty()) {
    return Slate(pluss::Series::empty(
        {0}, pluss::codem::ScalarType::Float64));
  }

  std::vector<bool> validity;
  validity.reserve(mapped.size());
  for (const auto& value : mapped) {
    validity.push_back(value.is_valid(0));
  }
  auto mask = bool_series_from_keep(validity);
  if (mapped.front().kind() == SlateKind::Series) {
    std::vector<pluss::Series> values;
    values.reserve(mapped.size());
    for (const auto& value : mapped) {
      values.push_back(value.as_series());
    }
    auto output = pluss::series::unite(values, 0, true);
    if (output.platform() != pluss::codem::Platform::CPU) {
      mask = mask.to(output.platform());
    }
    return Slate(output, mask);
  }
  if (mapped.front().kind() == SlateKind::Note) {
    std::vector<std::string> values;
    values.reserve(mapped.size());
    for (const auto& value : mapped) {
      values.emplace_back(value.as_note().at(0));
    }
    return Slate(pluss::make_note(values), mask);
  }
  if (mapped.front().kind() == SlateKind::Timeline) {
    auto values = mapped.front().as_timeline();
    for (std::size_t i = 1; i < mapped.size(); ++i) {
      values = values.concat(mapped[i].as_timeline());
    }
    if (values.epoch_nanos().platform() !=
        pluss::codem::Platform::CPU) {
      mask = mask.to(values.epoch_nanos().platform());
    }
    return Slate(values, mask);
  }
  PLUSS_CHECK(mapped.front().kind() == SlateKind::Duration,
              "DataFrame::transform: unsupported callback output kind");
  std::vector<std::int64_t> values;
  values.reserve(mapped.size());
  for (const auto& value : mapped) {
    values.push_back(value.as_duration().host().front());
  }
  return Slate(pluss::duration::nanos(values), mask);
}

DataFrame DataFrame::transform(
    const std::function<DataFrame(const DataFrame&)>& fn,
    Axis axis,
    ResultShape shape) const {
  PLUSS_CHECK(axis == Axis::Case,
              "DataFrame::transform(DataFrame callback): use Axis::Case for row-wise frame transforms");
  PLUSS_CHECK(static_cast<bool>(fn),
              "DataFrame::transform: callback is empty");
  DataFrame output;
  std::int64_t callback_cases = -1;
  std::vector<std::string> callback_features;
  for (std::int64_t row = 0; row < ncases(); ++row) {
    auto mapped = fn(slice(row, row + 1));
    PLUSS_CHECK(mapped.defined(),
                "DataFrame::transform: callback returned undefined");
    if (shape == ResultShape::Reduce ||
        shape == ResultShape::Preserve) {
      PLUSS_CHECK(
          mapped.ncases() == 1,
          "DataFrame::transform: selected shape requires one output case");
    }
    if (shape == ResultShape::Expand) {
      if (callback_cases < 0) {
        callback_cases = mapped.ncases();
        callback_features = mapped.features();
      }
      PLUSS_CHECK(
          mapped.ncases() == callback_cases,
          "DataFrame::transform: Expand requires a stable case count");
      PLUSS_CHECK(
          mapped.features() == callback_features,
          "DataFrame::transform: Expand requires a stable feature schema");
    }
    if (shape == ResultShape::Preserve) {
      PLUSS_CHECK(
          mapped.features() == features(),
          "DataFrame::transform: Preserve requires the input feature schema");
    }
    output = output.defined()
                 ? output.extend(
                       mapped, FrameMatchRule::Unite, true, false)
                 : std::move(mapped);
  }
  if (!output.defined()) {
    return DataFrame({}, Slate(pluss::Series::empty(
                             {0}, pluss::codem::ScalarType::Int64)));
  }
  return output;
}

DataFrame DataFrame::map(
    const std::function<SlateValue(const SlateValue&)>& fn) const {
  return map(fn, {});
}

DataFrame DataFrame::map(
    const std::function<SlateValue(const SlateValue&)>& fn,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "DataFrame::map: callback is empty");
  if (features.empty()) {
    return transform(
        [&fn](const Slate& feature) {
          return feature.map(fn);
        });
  }
  std::vector<TransformSpec> specs;
  specs.reserve(features.size());
  for (const auto feature : features) {
    specs.push_back(
        TransformSpec{
            .feature = owned_name(feature),
            .expr = [&fn](const Slate& value) {
              return value.map(fn);
            }});
  }
  return transform(specs);
}

Slate DataFrame::fold(
    const std::function<Slate(const DataFrame&)>& fn,
    ResultShape shape) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "DataFrame::fold: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "DataFrame::fold: callback returned undefined");
  if (shape == ResultShape::Reduce) {
    PLUSS_CHECK(result.size() == 1,
                "DataFrame::fold: Reduce requires one value");
  } else if (shape == ResultShape::Preserve) {
    PLUSS_CHECK(
        result.size() == ncases(),
        "DataFrame::fold: Preserve requires the case count");
  }
  return result;
}

DataFrame DataFrame::flow(
    const std::function<DataFrame(const DataFrame&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "DataFrame::flow: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "DataFrame::flow: callback returned undefined");
  return result;
}

DataFrameNoteAccessor DataFrame::nt() const {
  std::vector<std::string> selected;
  const auto& src = impl_ref_();
  selected.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    PLUSS_CHECK(src.columns_[i].kind() == SlateKind::Note,
                "DataFrame::nt: current frame contains non-Note feature '" +
                    src.names_[i] + "'. Select text feature(s) first, e.g. df.features({\"name\"}).nt()");
    selected.push_back(src.names_[i]);
  }
  PLUSS_CHECK(!selected.empty(), "DataFrame::nt: current frame has no Note features");
  return DataFrameNoteAccessor(*this, std::move(selected));
}

DataFrameNoteAccessor::DataFrameNoteAccessor(DataFrame frame,
                                             std::vector<std::string> features)
    : frame_(std::move(frame)),
      features_(std::move(features)) {}

DataFrame DataFrameNoteAccessor::strip() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().strip(); },
                                  "DataFrame::nt().strip");
}

DataFrame DataFrameNoteAccessor::strip(std::string_view chars) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [chars](const Slate& value) { return value.nt().strip(chars); },
                                  "DataFrame::nt().strip");
}

DataFrame DataFrameNoteAccessor::lstrip() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().lstrip(); },
                                  "DataFrame::nt().lstrip");
}

DataFrame DataFrameNoteAccessor::lstrip(std::string_view chars) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [chars](const Slate& value) { return value.nt().lstrip(chars); },
                                  "DataFrame::nt().lstrip");
}

DataFrame DataFrameNoteAccessor::rstrip() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().rstrip(); },
                                  "DataFrame::nt().rstrip");
}

DataFrame DataFrameNoteAccessor::rstrip(std::string_view chars) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [chars](const Slate& value) { return value.nt().rstrip(chars); },
                                  "DataFrame::nt().rstrip");
}

DataFrame DataFrameNoteAccessor::lower() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().lower(); },
                                  "DataFrame::nt().lower");
}

DataFrame DataFrameNoteAccessor::upper() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().upper(); },
                                  "DataFrame::nt().upper");
}

DataFrame DataFrameNoteAccessor::casefold() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().casefold(); },
                                  "DataFrame::nt().casefold");
}

DataFrame DataFrameNoteAccessor::titlecase() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().titlecase(); },
                                  "DataFrame::nt().titlecase");
}

DataFrame DataFrameNoteAccessor::length() const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [](const Slate& value) { return value.nt().length(); },
                                  "DataFrame::nt().length");
}

DataFrame DataFrameNoteAccessor::starts(std::string_view prefix) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [prefix](const Slate& value) { return value.nt().starts(prefix); },
                                  "DataFrame::nt().starts");
}

DataFrame DataFrameNoteAccessor::ends(std::string_view suffix) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [suffix](const Slate& value) { return value.nt().ends(suffix); },
                                  "DataFrame::nt().ends");
}

DataFrame DataFrameNoteAccessor::has(std::string_view needle,
                                     const TextOptions& options) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [needle, options](const Slate& value) { return value.nt().has(needle, options); },
                                  "DataFrame::nt().has");
}

DataFrame DataFrameNoteAccessor::count(std::string_view pattern) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [pattern](const Slate& value) { return value.nt().count(pattern); },
                                  "DataFrame::nt().count");
}

DataFrame DataFrameNoteAccessor::count(std::string_view pattern,
                                       const TextOptions& options) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [pattern, options](const Slate& value) { return value.nt().count(pattern, options); },
                                  "DataFrame::nt().count");
}

DataFrame DataFrameNoteAccessor::matches(std::string_view pattern, bool full) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [pattern, full](const Slate& value) { return value.nt().matches(pattern, full); },
                                  "DataFrame::nt().matches");
}

DataFrame DataFrameNoteAccessor::fullmatch(std::string_view pattern) const {
  return matches(pattern, true);
}

DataFrame DataFrameNoteAccessor::findall(std::string_view pattern) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().findall requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().findall: feature must be Note");
  return expanded_text_result_(source.nt().findall(pattern), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::findall(
    std::string_view pattern,
    const TextOptions& options) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().findall requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().findall: feature must be Note");
  return expanded_text_result_(source.nt().findall(pattern, options), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().extract requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().extract: feature must be Note");
  return expanded_text_result_(source.nt().extract(pattern, names), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().split requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().split: feature must be Note");
  return expanded_text_result_(source.nt().split(delimiter, limit, std::move(prefix)), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::split(
    std::string_view pattern,
    const TextOptions& options) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().split requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().split: feature must be Note");
  return expanded_text_result_(source.nt().split(pattern, options), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::partition(std::string_view separator,
                                           std::string prefix) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().partition requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().partition: feature must be Note");
  return expanded_text_result_(source.nt().partition(separator, std::move(prefix)), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::pad(std::int64_t width, TextAlign align, char fill) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [=](const Slate& value) { return value.nt().pad(width, align, fill); },
                                  "DataFrame::nt().pad");
}

DataFrame DataFrameNoteAccessor::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [=](const Slate& value) { return value.nt().chars(start, stop, step); },
                                  "DataFrame::nt().chars");
}

DataFrame DataFrameNoteAccessor::normalize(UnicodeForm form) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [form](const Slate& value) { return value.nt().normalize(form); },
                                  "DataFrame::nt().normalize");
}

DataFrame DataFrameNoteAccessor::encode(std::string_view encoding) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [encoding](const Slate& value) { return value.nt().encode(encoding); },
                                  "DataFrame::nt().encode");
}

DataFrame DataFrameNoteAccessor::indicate(std::string_view delimiter,
                                          std::string prefix) const {
  PLUSS_CHECK(features_.size() == 1, "DataFrame::nt().indicate requires one feature");
  const auto source = frame_.features(features_.front());
  PLUSS_CHECK(source.kind() == SlateKind::Note, "DataFrame::nt().indicate: feature must be Note");
  return expanded_text_result_(source.nt().indicate(delimiter, std::move(prefix)), frame_.labels().values(), frame_.title());
}

DataFrame DataFrameNoteAccessor::replace(std::string_view from,
                                         std::string_view to) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [from, to](const Slate& value) { return value.nt().replace(from, to); },
                                  "DataFrame::nt().replace");
}

DataFrame DataFrameNoteAccessor::replace(std::string_view from,
                                         std::string_view to,
                                         const TextOptions& options) const {
  return transform_text_features_(frame_.impl_ref_(), features_,
                                  [from, to, options](const Slate& value) { return value.nt().replace(from, to, options); },
                                  "DataFrame::nt().replace");
}

DataFrameTimelineAccessor DataFrame::tl() const {
  const auto& src = impl_ref_();
  const auto indices = resolve_time_feature_indices_(src, {}, "DataFrame::tl");
  std::vector<std::string> selected;
  selected.reserve(indices.size());
  for (const auto idx : indices) {
    selected.push_back(src.names_[idx]);
  }
  return DataFrameTimelineAccessor(*this, std::move(selected));
}

DataFrameTimelineAccessor::DataFrameTimelineAccessor(DataFrame frame,
                                                     std::vector<std::string> features)
    : frame_(std::move(frame)),
      features_(std::move(features)) {
  PLUSS_CHECK(!features_.empty(), "DataFrame::tl: expected at least one Timeline feature");
}

DataFrame DataFrameTimelineAccessor::map_(
    const std::function<Slate(const Slate&)>& fn,
    std::string_view op) const {
  PLUSS_CHECK(static_cast<bool>(fn), std::string(op) + ": callback is empty");
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(static_cast<std::size_t>(frame_.nfeatures()));
  std::unordered_set<std::string> selected(features_.begin(), features_.end());
  for (const auto& name : frame_.features()) {
    const auto source = frame_.features(name);
    if (selected.contains(name)) {
      PLUSS_CHECK(is_time_like_slate_(source.kind()),
                  std::string(op) + ": feature '" + name + "' is not Timeline");
      columns.emplace_back(name, fn(source));
    } else {
      columns.emplace_back(name, source);
    }
  }
  return DataFrame(std::move(columns), frame_.labels().values()).title(frame_.title());
}

DataFrame DataFrameTimelineAccessor::isnat() const {
  return map_([](const Slate& t) { return t.tl().isnat(); }, "DataFrame::tl().isnat");
}

DataFrame DataFrameTimelineAccessor::valid() const {
  return map_([](const Slate& t) { return t.tl().valid(); }, "DataFrame::tl().valid");
}

DataFrame DataFrameTimelineAccessor::year() const {
  return map_([](const Slate& t) { return t.tl().year(); }, "DataFrame::tl().year");
}

DataFrame DataFrameTimelineAccessor::month() const {
  return map_([](const Slate& t) { return t.tl().month(); }, "DataFrame::tl().month");
}

DataFrame DataFrameTimelineAccessor::day() const {
  return map_([](const Slate& t) { return t.tl().day(); }, "DataFrame::tl().day");
}

DataFrame DataFrameTimelineAccessor::hour() const {
  return map_([](const Slate& t) { return t.tl().hour(); }, "DataFrame::tl().hour");
}

DataFrame DataFrameTimelineAccessor::minute() const {
  return map_([](const Slate& t) { return t.tl().minute(); }, "DataFrame::tl().minute");
}

DataFrame DataFrameTimelineAccessor::second() const {
  return map_([](const Slate& t) { return t.tl().second(); }, "DataFrame::tl().second");
}

DataFrame DataFrameTimelineAccessor::weekday() const {
  return map_([](const Slate& t) { return t.tl().weekday(); }, "DataFrame::tl().weekday");
}

DataFrame DataFrameTimelineAccessor::yearday() const {
  return map_([](const Slate& t) { return t.tl().yearday(); }, "DataFrame::tl().yearday");
}

DataFrame DataFrameTimelineAccessor::quarter() const {
  return map_([](const Slate& t) { return t.tl().quarter(); }, "DataFrame::tl().quarter");
}

DataFrame DataFrameTimelineAccessor::week() const {
  return map_([](const Slate& t) { return t.tl().week(); }, "DataFrame::tl().week");
}

DataFrame DataFrameTimelineAccessor::weekend() const {
  return map_([](const Slate& t) { return t.tl().weekend(); }, "DataFrame::tl().weekend");
}

DataFrame DataFrameTimelineAccessor::month_start() const {
  return map_([](const Slate& t) { return t.tl().month_start(); }, "DataFrame::tl().month_start");
}

DataFrame DataFrameTimelineAccessor::month_end() const {
  return map_([](const Slate& t) { return t.tl().month_end(); }, "DataFrame::tl().month_end");
}

DataFrame DataFrameTimelineAccessor::leap() const {
  return map_([](const Slate& t) { return t.tl().leap(); }, "DataFrame::tl().leap");
}

DataFrame DataFrameTimelineAccessor::weekday_name() const {
  return map_([](const Slate& t) { return t.tl().weekday_name(); }, "DataFrame::tl().weekday_name");
}

DataFrame DataFrameTimelineAccessor::month_name() const {
  return map_([](const Slate& t) { return t.tl().month_name(); }, "DataFrame::tl().month_name");
}

DataFrame DataFrameTimelineAccessor::fmt(std::string_view pattern) const {
  return map_([pattern](const Slate& t) { return t.tl().fmt(pattern); }, "DataFrame::tl().fmt");
}

DataFrame DataFrameTimelineAccessor::iso() const {
  return map_([](const Slate& t) { return t.tl().iso(); }, "DataFrame::tl().iso");
}

DataFrame DataFrameTimelineAccessor::epoch(TimeUnit unit) const {
  return map_([unit](const Slate& t) { return t.tl().epoch(unit); }, "DataFrame::tl().epoch");
}

DataFrame DataFrameTimelineAccessor::span(
    const Slate& other,
    const TimelineSpanOptions& options) const {
  return map_([&](const Slate& t) { return t.tl().span(other, options); }, "DataFrame::tl().span");
}

DataFrame DataFrameTimelineAccessor::span(const Slate& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

DataFrame DataFrameTimelineAccessor::span(
    const pluss::Timeline& other,
    const TimelineSpanOptions& options) const {
  return span(Slate(other), options);
}

DataFrame DataFrameTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

DataFrame DataFrameTimelineAccessor::span(
    const DataFrame& other,
    const TimelineSpanOptions& options) const {
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(static_cast<std::size_t>(frame_.nfeatures()));
  std::unordered_set<std::string> selected(features_.begin(), features_.end());
  for (const auto& name : frame_.features()) {
    const auto source = frame_.features(name);
    if (!selected.contains(name)) {
      columns.emplace_back(name, source);
      continue;
    }
    PLUSS_CHECK(is_time_like_slate_(source.kind()),
                "DataFrame::tl().span: feature '" + name + "' is not Timeline");
    PLUSS_CHECK(other.get(name).has_value(),
                "DataFrame::tl().span: other DataFrame is missing feature '" + name + "'");
    columns.emplace_back(name, source.tl().span(other.features(name), options));
  }
  return DataFrame(std::move(columns), frame_.labels().values()).title(frame_.title());
}

DataFrame DataFrameTimelineAccessor::span(const DataFrame& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

DataFrame DataFrameTimelineAccessor::floor(std::string_view unit) const {
  return map_([unit](const Slate& t) { return t.tl().floor(unit); }, "DataFrame::tl().floor");
}

DataFrame DataFrameTimelineAccessor::ceil(std::string_view unit) const {
  return map_([unit](const Slate& t) { return t.tl().ceil(unit); }, "DataFrame::tl().ceil");
}

DataFrame DataFrameTimelineAccessor::round(std::string_view unit) const {
  return map_([unit](const Slate& t) { return t.tl().round(unit); }, "DataFrame::tl().round");
}

DataFrame DataFrameTimelineAccessor::date() const {
  return map_([](const Slate& t) { return t.tl().date(); }, "DataFrame::tl().date");
}

DataFrame DataFrameTimelineAccessor::time() const {
  return map_([](const Slate& t) { return t.tl().time(); }, "DataFrame::tl().time");
}

DataFrame DataFrameTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return map_([=](const Slate& t) { return t.tl().shift(periods, freq); }, "DataFrame::tl().shift");
}

DataFrame DataFrameTimelineAccessor::period(std::string_view freq) const {
  return map_([freq](const Slate& t) { return t.tl().period(freq); }, "DataFrame::tl().period");
}

DataFrame DataFrameTimelineAccessor::tz(std::string_view name) const {
  return map_([name](const Slate& t) { return t.tl().tz(name); }, "DataFrame::tl().tz");
}

DataFrame DataFrameTimelineAccessor::localize(const TimelineLocalizeOptions& options) const {
  return map_([options](const Slate& t) { return t.tl().localize(options); }, "DataFrame::tl().localize");
}

DataFrame DataFrameTimelineAccessor::utc() const {
  return map_([](const Slate& t) { return t.tl().utc(); }, "DataFrame::tl().utc");
}

DataFrame DataFrame::transform(const std::function<Slate(const Slate&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform: transform function is empty");
  const auto& src = impl_ref_();

  auto out_cols = pluss::tils::transmit([&](const Slate& feature) {
    auto transformed = fn(feature);
    PLUSS_CHECK(transformed.defined(), "DataFrame::transform: transform returned undefined Slate");
    PLUSS_CHECK(transformed.size() == src.nrows_,
                "DataFrame::transform: transformed feature length must match DataFrame case count");
    return transformed;
  }, src.columns_);

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::transform(
    const std::function<Slate(const Slate&)>& fn,
    Axis axis) const {
  PLUSS_CHECK(axis == Axis::Feature,
              "DataFrame::transform(Slate callback): use Axis::Feature for feature-wise transforms; use the DataFrame callback overload with Axis::Case for row-wise transforms");
  return transform(fn);
}

DataFrame DataFrame::transform(std::string_view feature, const std::function<Slate(const DataFrame&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform(feature): transform function is empty");
  const std::array<DeriveSpec, 1> spec{{DeriveSpec{owned_name(feature), fn}}};
  return assign(pluss::msharp::RangeView::Of<DeriveSpec>(spec));
}

DataFrame DataFrame::transform(std::string_view feature, const std::function<Slate(const Slate&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform(feature): transform function is empty");
  const std::array<TransformSpec, 1> spec{{TransformSpec{owned_name(feature), fn}}};
  return transform(pluss::msharp::RangeView::Of<TransformSpec>(spec));
}

DataFrame DataFrame::transform(pluss::msharp::RangeView::Of<TransformSpec> specs) const {
  PLUSS_CHECK(specs.size() > 0, "DataFrame::transform: at least one TransformSpec is required");
  const auto& src = impl_ref_();

  auto out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::transform: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::transform: transform expression is empty");
    PLUSS_CHECK(seen.insert(spec.feature).second,
                "DataFrame::transform: duplicate transform target '" + spec.feature + "'");

    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::transform: unknown feature '" + spec.feature + "'");

    auto transformed = spec.expr(out_cols[it->second]);
    PLUSS_CHECK(transformed.defined(), "DataFrame::transform: transform returned undefined Slate");
    PLUSS_CHECK(transformed.size() == src.nrows_,
                "DataFrame::transform: transformed feature length must match DataFrame case count");
    out_cols[it->second] = std::move(transformed);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::put(const Slate& bool_mask, const Slate& value) const {
  return put(bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(const Slate& bool_mask,
                         const Slate& value,
                         const AlignOptions& options) const {
  const auto& src = impl_ref_();
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put");
  auto out_cols = src.columns_;
  for (auto& col : out_cols) {
    col = col.put(mask, value);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::put(std::string_view feature, const Slate& bool_mask, const Slate& value) const {
  return put(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(std::string_view feature,
                         const Slate& bool_mask,
                         const Slate& value,
                         const AlignOptions& options) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put: feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put: unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::put(const Slate& bool_mask, double value) const {
  return put(bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(const Slate& bool_mask,
                         double value,
                         const AlignOptions& options) const {
  const auto& src = impl_ref_();
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put(double)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put(double)");
  auto out_cols = src.columns_;
  for (auto& col : out_cols) {
    col = col.put(mask, value);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::put(std::string_view feature, const Slate& bool_mask, double value) const {
  return put(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(std::string_view feature,
                         const Slate& bool_mask,
                         double value,
                         const AlignOptions& options) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put(double): feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put(double): unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put(double)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put(double)");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::put(const Slate& bool_mask, std::string_view value) const {
  return put(bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(const Slate& bool_mask,
                         std::string_view value,
                         const AlignOptions& options) const {
  const auto& src = impl_ref_();
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put(string)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put(string)");
  auto out_cols = src.columns_;
  for (auto& col : out_cols) {
    col = col.put(mask, value);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::put(std::string_view feature, const Slate& bool_mask, std::string_view value) const {
  return put(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(std::string_view feature,
                         const Slate& bool_mask,
                         std::string_view value,
                         const AlignOptions& options) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put(string): feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put(string): unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put(string)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put(string)");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::put(const Slate& bool_mask, pluss::null_t value) const {
  return put(bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(const Slate& bool_mask,
                         pluss::null_t value,
                         const AlignOptions& options) const {
  const auto& src = impl_ref_();
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put(null)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put(null)");
  auto out_cols = src.columns_;
  for (auto& col : out_cols) {
    col = col.put(mask, value);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::put(std::string_view feature, const Slate& bool_mask, pluss::null_t value) const {
  return put(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame DataFrame::put(std::string_view feature,
                         const Slate& bool_mask,
                         pluss::null_t value,
                         const AlignOptions& options) const {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put(null): feature name cannot be empty");
  const auto& src = impl_ref_();
  const auto it = src.name_to_idx_.find(name);
  PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::put(null): unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(src, bool_mask, "DataFrame::put(null)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, src.nrows_, "DataFrame::put(null)");
  auto out_cols = src.columns_;
  out_cols[it->second] = out_cols[it->second].put(mask, value);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, const Slate& value) {
  return put_(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame& DataFrame::put_(std::string_view feature,
                           const Slate& bool_mask,
                           const Slate& value,
                           const AlignOptions& options) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_: feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_: unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(w, bool_mask, "DataFrame::put_", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, w.nrows_, "DataFrame::put_");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, double value) {
  return put_(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame& DataFrame::put_(std::string_view feature,
                           const Slate& bool_mask,
                           double value,
                           const AlignOptions& options) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_(double): feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_(double): unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(w, bool_mask, "DataFrame::put_(double)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, w.nrows_, "DataFrame::put_(double)");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, std::string_view value) {
  return put_(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame& DataFrame::put_(std::string_view feature,
                           const Slate& bool_mask,
                           std::string_view value,
                           const AlignOptions& options) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_(string): feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_(string): unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(w, bool_mask, "DataFrame::put_(string)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, w.nrows_, "DataFrame::put_(string)");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame& DataFrame::put_(std::string_view feature, const Slate& bool_mask, pluss::null_t value) {
  return put_(feature, bool_mask, value, row_mask_default_options_());
}

DataFrame& DataFrame::put_(std::string_view feature,
                           const Slate& bool_mask,
                           pluss::null_t value,
                           const AlignOptions& options) {
  const auto name = owned_name(feature);
  PLUSS_CHECK(!name.empty(), "DataFrame::put_(null): feature name cannot be empty");
  auto& w = mut_impl_();
  const auto it = w.name_to_idx_.find(name);
  PLUSS_CHECK(it != w.name_to_idx_.end(), "DataFrame::put_(null): unknown feature '" + name + "'");
  const auto aligned_mask = prepare_mask_slate_(w, bool_mask, "DataFrame::put_(null)", options);
  const auto mask = bool_series_from_slate_mask_(aligned_mask, w.nrows_, "DataFrame::put_(null)");
  w.columns_[it->second] = w.columns_[it->second].put(mask, value);
  return *this;
}

DataFrame DataFrame::has(double value) const {
  const auto& src = impl_ref_();
  std::vector<Slate> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& col : src.columns_) {
    out_cols.push_back(col.has(value));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::has(std::string_view value) const {
  const auto& src = impl_ref_();
  std::vector<Slate> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& col : src.columns_) {
    out_cols.push_back(col.has(value));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}

DataFrame DataFrame::has(const Slate& values) const {
  const auto& src = impl_ref_();
  std::vector<Slate> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& col : src.columns_) {
    out_cols.push_back(col.has(values));
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_, src.case_axis_));
}
