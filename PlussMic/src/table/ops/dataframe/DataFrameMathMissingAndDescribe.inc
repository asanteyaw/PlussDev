DataFrame DataFrame::add(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Add, false);
}

DataFrame DataFrame::add(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Add);
}

DataFrame DataFrame::sub(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Sub, false);
}

DataFrame DataFrame::sub(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Sub);
}

DataFrame DataFrame::mul(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mul, false);
}

DataFrame DataFrame::mul(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Mul);
}

DataFrame DataFrame::div(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Div, false);
}

DataFrame DataFrame::div(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Div);
}

DataFrame DataFrame::truediv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::TrueDiv, false);
}

DataFrame DataFrame::truediv(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::TrueDiv);
}

DataFrame DataFrame::floordiv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::FloorDiv, false);
}

DataFrame DataFrame::floordiv(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::FloorDiv);
}

DataFrame DataFrame::mod(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mod, false);
}

DataFrame DataFrame::mod(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Mod);
}

DataFrame DataFrame::pow(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Pow, false);
}

DataFrame DataFrame::pow(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Pow);
}

DataFrame DataFrame::radd(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Add, true);
}

DataFrame DataFrame::rsub(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Sub, true);
}

DataFrame DataFrame::rmul(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mul, true);
}

DataFrame DataFrame::rdiv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Div, true);
}

DataFrame DataFrame::rtruediv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::TrueDiv, true);
}

DataFrame DataFrame::rfloordiv(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::FloorDiv, true);
}

DataFrame DataFrame::rmod(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mod, true);
}

DataFrame DataFrame::rpow(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Pow, true);
}

DataFrame DataFrame::eq(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Eq);
}

DataFrame DataFrame::eq(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Eq);
}

DataFrame DataFrame::ne(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Ne);
}

DataFrame DataFrame::ne(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Ne);
}

DataFrame DataFrame::lt(double other, bool strict) const {
  return dataframe_cmp_scalar(*this, other, strict ? BinaryCmpOp::Lt : BinaryCmpOp::Le);
}

DataFrame DataFrame::lt(const DataFrame& other, bool strict) const {
  return dataframe_cmp_frame(*this, other, strict ? BinaryCmpOp::Lt : BinaryCmpOp::Le);
}

DataFrame DataFrame::gt(double other, bool strict) const {
  return dataframe_cmp_scalar(*this, other, strict ? BinaryCmpOp::Gt : BinaryCmpOp::Ge);
}

DataFrame DataFrame::gt(const DataFrame& other, bool strict) const {
  return dataframe_cmp_frame(*this, other, strict ? BinaryCmpOp::Gt : BinaryCmpOp::Ge);
}

DataFrame DataFrame::compare(double other) const {
  return ne(other);
}

DataFrame DataFrame::compare(const DataFrame& other) const {
  return ne(other);
}

namespace {

template <class Compatible, class Apply>
std::optional<std::vector<Thread>> dataframe_replace_selected_impl(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
    Compatible&& compatible,
    Apply&& apply,
    std::string_view where) {
  std::unordered_set<std::size_t> target_indices;

  if (features.size() == 0) {
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      if (compatible(src.columns_[i])) {
        target_indices.insert(i);
      }
    }
  } else {
    for (const auto& name : normalize_name_list(features)) {
      const auto it = src.name_to_idx_.find(name);
      PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown feature '" + name + "'");
      target_indices.insert(it->second);
    }
  }

  if (target_indices.empty()) {
    return std::nullopt;
  }

  auto out_cols = src.columns_;
  bool changed = false;
  for (const auto idx : target_indices) {
    if (!compatible(out_cols[idx])) {
      continue;
    }
    out_cols[idx] = apply(out_cols[idx]);
    changed = true;
  }

  if (!changed) {
    return std::nullopt;
  }

  return out_cols;
}

} // namespace

DataFrame DataFrame::replace(double from, double to, pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Series; },
      [from, to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(double, double)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(double from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Series; },
      [from, to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(double, null)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(pluss::null_t from, double to, pluss::msharp::RangeView::Of<std::string_view> features) const {
  return fillna(to, features);
}

DataFrame DataFrame::replace(
    std::string_view from,
    std::string_view to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  const auto from_note = pluss::make_note(from);
  const auto to_note = pluss::make_note(to);
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Note; },
      [&from_note, &to_note](const Thread& col) { return col.replace(from_note, to_note); },
      "DataFrame::replace(string, string)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    std::string_view from,
    pluss::null_t to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  const auto from_note = pluss::make_note(from);
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Note; },
      [&from_note, to](const Thread& col) { return col.replace(from_note, to); },
      "DataFrame::replace(string, null)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    pluss::null_t from,
    std::string_view to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  return fillna(to, features);
}

DataFrame DataFrame::replace(
    const pluss::Series& from,
    const pluss::Series& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Series; },
      [&from, &to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(Series, Series)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    const pluss::Series& from,
    pluss::null_t to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Series; },
      [&from, to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(Series, null)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    pluss::null_t from,
    const pluss::Series& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Series; },
      [&to, from](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(null, Series)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    const pluss::Note& from,
    const pluss::Note& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Note; },
      [&from, &to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(Note, Note)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    const pluss::Note& from,
    pluss::null_t to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Note; },
      [&from, to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(Note, null)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    pluss::null_t from,
    const pluss::Note& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::Note; },
      [&to, from](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(null, Note)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    const pluss::Timeline& from,
    const pluss::Timeline& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) {
        return col.kind() == ThreadKind::Timeline || col.kind() == ThreadKind::TimeIndex;
      },
      [&from, &to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(Timeline, Timeline)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    const pluss::Timeline& from,
    pluss::null_t to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) {
        return col.kind() == ThreadKind::Timeline || col.kind() == ThreadKind::TimeIndex;
      },
      [&from, to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(Timeline, null)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    pluss::null_t from,
    const pluss::Timeline& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  return fillna(to, features);
}

DataFrame DataFrame::replace(
    const pluss::TimeIndex& from,
    const pluss::TimeIndex& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::TimeIndex; },
      [&from, &to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(TimeIndex, TimeIndex)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    const pluss::TimeIndex& from,
    pluss::null_t to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  auto out_cols = dataframe_replace_selected_impl(
      src,
      features,
      [](const Thread& col) { return col.kind() == ThreadKind::TimeIndex; },
      [&from, to](const Thread& col) { return col.replace(from, to); },
      "DataFrame::replace(TimeIndex, null)");
  if (!out_cols.has_value()) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(*out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::replace(
    pluss::null_t from,
    const pluss::TimeIndex& to,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  return fillna(to, features);
}

namespace {

std::int64_t normalize_reduce_axis_(Axis axis, std::string_view where) {
  switch (axis) {
    case Axis::Feature:
      return 0;
    case Axis::Case:
      return 1;
  }
  PLUSS_FAIL(std::string(where) + ": axis must be Axis::Feature or Axis::Case");
}

struct ReduceMatrix_ {
  pluss::Series values;
  pluss::Series valid;
  std::vector<std::string> names;
  std::int64_t ncases{0};
};

std::vector<std::size_t> resolve_numeric_reduce_indices_(
    const detail::DataFrameImpl& src,
    std::string_view where) {
  std::vector<std::size_t> indices;
  indices.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      indices.push_back(i);
    }
  }
  PLUSS_CHECK(!indices.empty(), std::string(where) + ": no numeric features");
  return indices;
}

std::vector<std::size_t> resolve_bool_reduce_indices_(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::string_view where) {
  std::vector<std::size_t> indices;
  if (features.size() == 0) {
    indices.reserve(src.columns_.size());
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      indices.push_back(i);
    }
  } else {
    std::unordered_set<std::string> seen;
    seen.reserve(features.size());
    indices.reserve(features.size());
    for (const auto key_sv : features) {
      const auto key = owned_name(key_sv);
      PLUSS_CHECK(!key.empty(), std::string(where) + ": feature name cannot be empty");
      PLUSS_CHECK(seen.insert(key).second, std::string(where) + ": duplicate feature '" + key + "'");
      const auto it = src.name_to_idx_.find(key);
      PLUSS_CHECK(it != src.name_to_idx_.end(), std::string(where) + ": unknown feature '" + key + "'");
      indices.push_back(it->second);
    }
  }

  PLUSS_CHECK(!indices.empty(), std::string(where) + ": no features selected");
  for (const auto idx : indices) {
    const auto& col = src.columns_[idx];
    PLUSS_CHECK(col.kind() == ThreadKind::Series,
                std::string(where) + ": feature '" + src.names_[idx] + "' must be a Bool Series");
    PLUSS_CHECK(col.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
                std::string(where) + ": feature '" + src.names_[idx] + "' must be a Bool Series");
  }
  return indices;
}

std::vector<std::string> reduce_names_(
    const detail::DataFrameImpl& src,
    std::span<const std::size_t> indices) {
  std::vector<std::string> names;
  names.reserve(indices.size());
  std::transform(indices.begin(), indices.end(), std::back_inserter(names),
                 [&](const auto idx) { return src.names_[idx]; });
  return names;
}

pluss::Series validity_matrix_(
    const detail::DataFrameImpl& src,
    std::span<const std::size_t> indices,
    std::string_view where) {
  std::vector<pluss::Series> masks;
  masks.reserve(indices.size());
  std::transform(indices.begin(), indices.end(), std::back_inserter(masks), [&](const auto idx) {
    const auto& col = src.columns_[idx];
    return col.notnull().as_series().reshape({src.nrows_});
  });
  return pack_series_columns(masks, where, true);
}

ReduceMatrix_ numeric_reduce_matrix_(
    const detail::DataFrameImpl& src,
    std::string_view where) {
  const auto indices = resolve_numeric_reduce_indices_(src, where);
  const auto platform = src.columns_[indices.front()].as_series().platform();
  const auto compute_type =
      platform == pluss::codem::Platform::CPU
          ? pluss::codem::ScalarType::Float64
          : pluss::codem::ScalarType::Float32;
  std::vector<pluss::Series> columns;
  columns.reserve(indices.size());
  std::transform(indices.begin(), indices.end(), std::back_inserter(columns), [&](const auto idx) {
    PLUSS_CHECK(src.columns_[idx].as_series().platform() == platform,
                std::string(where) + ": numeric features must share a platform");
    return src.columns_[idx].as_series().reshape({src.nrows_}).to(compute_type);
  });
  return ReduceMatrix_{
      pack_series_columns(columns, where, true),
      validity_matrix_(src, indices, where),
      reduce_names_(src, indices),
      src.nrows_,
  };
}

ReduceMatrix_ bool_reduce_matrix_(
    const detail::DataFrameImpl& src,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::string_view where) {
  const auto indices = resolve_bool_reduce_indices_(src, features, where);
  std::vector<pluss::Series> columns;
  columns.reserve(indices.size());
  std::transform(indices.begin(), indices.end(), std::back_inserter(columns), [&](const auto idx) {
    return src.columns_[idx].as_series().reshape({src.nrows_});
  });
  return ReduceMatrix_{
      pack_series_columns(columns, where, true),
      validity_matrix_(src, indices, where),
      reduce_names_(src, indices),
      src.nrows_,
  };
}

Thread with_reduce_labels_(
    Thread out,
    const detail::DataFrameImpl& src,
    const std::vector<std::string>& names,
    std::int64_t axis) {
  if (axis == 0) {
    return out.with_display_labels(make_note_thread(names));
  }
  return out.with_display_labels(src.cases_);
}

pluss::Series reduced_validity_(
    const ReduceMatrix_& matrix,
    std::int64_t axis,
    bool skipna,
    bool identity_defined_for_empty) {
  const auto counts = matrix.valid.to(pluss::codem::ScalarType::Int64).sum(axis);
  if (!skipna) {
    const auto reduced_size =
        axis == 0 ? matrix.ncases : static_cast<std::int64_t>(matrix.names.size());
    return counts.eq(static_cast<double>(reduced_size));
  }
  if (identity_defined_for_empty) {
    return pluss::Series::ones(counts.sizes(), pluss::codem::ScalarType::Bool);
  }
  return counts.gt(0.0);
}

enum class NumericAxisOp_ : std::uint8_t {
  Sum,
  Min,
  Max,
  Mean,
  Median,
  Std,
  Var,
  Prod,
};

Thread reduce_numeric_(
    const detail::DataFrameImpl& src,
    std::int64_t axis,
    bool skipna,
    NumericAxisOp_ op,
    std::string_view where) {
  const auto matrix = numeric_reduce_matrix_(src, where);
  const auto output_size =
      axis == 0 ? static_cast<std::int64_t>(matrix.names.size()) : matrix.ncases;
  const auto reduced_size =
      axis == 0 ? matrix.ncases : static_cast<std::int64_t>(matrix.names.size());
  const bool identity_defined =
      op == NumericAxisOp_::Sum || op == NumericAxisOp_::Prod;

  if (reduced_size == 0) {
    const auto fill = op == NumericAxisOp_::Sum
                          ? 0.0
                          : op == NumericAxisOp_::Prod
                                ? 1.0
                                : std::numeric_limits<double>::quiet_NaN();
    auto values = pluss::Series::full(
        {output_size}, fill, pluss::codem::ScalarType::Float64);
    auto valid = identity_defined
                     ? pluss::Series::ones({output_size}, pluss::codem::ScalarType::Bool)
                     : pluss::Series::zeros({output_size}, pluss::codem::ScalarType::Bool);
    return with_reduce_labels_(Thread(values, valid), src, matrix.names, axis);
  }

  const auto invalid = matrix.valid.logical_not();
  const auto counts = matrix.valid.to(pluss::codem::ScalarType::Int64).sum(axis);
  pluss::Series values;
  switch (op) {
    case NumericAxisOp_::Sum:
      values = matrix.values.where(invalid, 0.0).sum(axis);
      break;
    case NumericAxisOp_::Prod:
      values = matrix.values.where(invalid, 1.0).prod(axis);
      break;
    case NumericAxisOp_::Min:
      values = matrix.values
                   .where(invalid, std::numeric_limits<double>::infinity())
                   .min(axis);
      break;
    case NumericAxisOp_::Max:
      values = matrix.values
                   .where(invalid, -std::numeric_limits<double>::infinity())
                   .max(axis);
      break;
    case NumericAxisOp_::Mean:
      values = matrix.values.where(invalid, 0.0).sum(axis)
               / counts.to(pluss::codem::ScalarType::Float64);
      break;
    case NumericAxisOp_::Median:
      values = matrix.values
                   .where(invalid, std::numeric_limits<double>::quiet_NaN())
                   .nanmedian(axis);
      break;
    case NumericAxisOp_::Std:
    case NumericAxisOp_::Var: {
      const auto count_values = counts.to(pluss::codem::ScalarType::Float64);
      const auto means = matrix.values.where(invalid, 0.0).sum(axis) / count_values;
      const auto means_2d =
          axis == 0
              ? means.reshape({1, static_cast<std::int64_t>(matrix.names.size())})
              : means.reshape({matrix.ncases, 1});
      const auto centered = matrix.values - means_2d;
      const auto sum_squares = (centered * centered).where(invalid, 0.0).sum(axis);
      values = sum_squares / (count_values - 1.0);
      if (op == NumericAxisOp_::Std) {
        values = values.sqrt();
      }
      break;
    }
  }

  const auto valid = reduced_validity_(matrix, axis, skipna, identity_defined);
  return with_reduce_labels_(Thread(values, valid), src, matrix.names, axis);
}

Thread reduce_bool_(
    const detail::DataFrameImpl& src,
    std::int64_t axis,
    pluss::msharp::RangeView::Of<std::string_view> features,
    bool use_all,
    bool skipna,
    std::string_view where) {
  const auto matrix = bool_reduce_matrix_(src, features, where);
  const auto prepared =
      use_all ? matrix.values.logical_or(matrix.valid.logical_not())
              : matrix.values.logical_and(matrix.valid);
  const auto values = use_all ? prepared.all(axis) : prepared.any(axis);
  const auto valid =
      skipna ? pluss::Series::ones(values.sizes(), pluss::codem::ScalarType::Bool)
             : matrix.valid.all(axis);
  return with_reduce_labels_(Thread(values, valid), src, matrix.names, axis);
}

} // namespace

Thread DataFrame::sum(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::sum");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Sum, "DataFrame::sum");
}

Thread DataFrame::mean(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::mean");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Mean, "DataFrame::mean");
}

Thread DataFrame::median(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::median");
  return reduce_numeric_(
      impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Median, "DataFrame::median");
}

Thread DataFrame::min(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::min");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Min, "DataFrame::min");
}

Thread DataFrame::max(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::max");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Max, "DataFrame::max");
}

Thread DataFrame::std(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::std");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Std, "DataFrame::std");
}

Thread DataFrame::var(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::var");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Var, "DataFrame::var");
}

Thread DataFrame::prod(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::prod");
  return reduce_numeric_(impl_ref_(), normalized_axis, skipna, NumericAxisOp_::Prod, "DataFrame::prod");
}

Thread DataFrame::count(Axis axis) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::count");
  const auto& src = impl_ref_();
  PLUSS_CHECK(!src.columns_.empty(), "DataFrame::count: no features");
  std::vector<std::size_t> indices(src.columns_.size());
  std::iota(indices.begin(), indices.end(), std::size_t{0});
  const auto names = reduce_names_(src, indices);
  const auto valid = validity_matrix_(src, indices, "DataFrame::count");
  auto out = Thread(valid.to(pluss::codem::ScalarType::Int64).sum(normalized_axis));
  return with_reduce_labels_(std::move(out), src, names, normalized_axis);
}

Thread DataFrame::any(pluss::msharp::RangeView::Of<std::string_view> features, bool skipna) const {
  return any(Axis::Feature, features, skipna);
}

Thread DataFrame::any(Axis axis,
                      pluss::msharp::RangeView::Of<std::string_view> features,
                      bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::any");
  return reduce_bool_(
      impl_ref_(), normalized_axis, features, false, skipna, "DataFrame::any");
}

Thread DataFrame::all(pluss::msharp::RangeView::Of<std::string_view> features, bool skipna) const {
  return all(Axis::Feature, features, skipna);
}

Thread DataFrame::all(Axis axis,
                      pluss::msharp::RangeView::Of<std::string_view> features,
                      bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::all");
  return reduce_bool_(
      impl_ref_(), normalized_axis, features, true, skipna, "DataFrame::all");
}

Thread DataFrame::csmin(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::csmin");
  const auto& src = impl_ref_();
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::csmin");
  const auto reduced_size =
      normalized_axis == 0 ? matrix.ncases : static_cast<std::int64_t>(matrix.names.size());
  PLUSS_CHECK(reduced_size > 0, "DataFrame::csmin: cannot reduce an empty axis");
  const auto indices = matrix.values
                           .where(matrix.valid.logical_not(),
                                  std::numeric_limits<double>::infinity())
                           .argmin(normalized_axis)
                           .to_vec<std::int64_t>();
  auto out = normalized_axis == 0
                 ? src.cases_.take(indices)
                 : make_note_thread(matrix.names).take(indices);
  out = out.with_validity_mask(
      reduced_validity_(matrix, normalized_axis, skipna, false));
  return with_reduce_labels_(std::move(out), src, matrix.names, normalized_axis);
}

Thread DataFrame::csmax(Axis axis, bool skipna) const {
  const auto normalized_axis = normalize_reduce_axis_(axis, "DataFrame::csmax");
  const auto& src = impl_ref_();
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::csmax");
  const auto reduced_size =
      normalized_axis == 0 ? matrix.ncases : static_cast<std::int64_t>(matrix.names.size());
  PLUSS_CHECK(reduced_size > 0, "DataFrame::csmax: cannot reduce an empty axis");
  const auto indices = matrix.values
                           .where(matrix.valid.logical_not(),
                                  -std::numeric_limits<double>::infinity())
                           .argmax(normalized_axis)
                           .to_vec<std::int64_t>();
  auto out = normalized_axis == 0
                 ? src.cases_.take(indices)
                 : make_note_thread(matrix.names).take(indices);
  out = out.with_validity_mask(
      reduced_validity_(matrix, normalized_axis, skipna, false));
  return with_reduce_labels_(std::move(out), src, matrix.names, normalized_axis);
}

Thread DataFrame::ndistinct(pluss::msharp::RangeView::Of<std::string_view> features,
                            bool dropna) const {
  const auto& src = impl_ref_();

  std::vector<std::string> names;
  if (features.size() == 0) {
    names = src.names_;
  } else {
    std::unordered_set<std::string> seen;
    seen.reserve(features.size());
    names.reserve(features.size());
    for (const auto feature : features) {
      const auto key = owned_name(feature);
      PLUSS_CHECK(!key.empty(), "DataFrame::ndistinct: feature name cannot be empty");
      if (!seen.insert(key).second) {
        continue;
      }
      const auto it = src.name_to_idx_.find(key);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::ndistinct: unknown feature '" + key + "'");
      names.push_back(key);
    }
  }

  PLUSS_CHECK(!names.empty(), "DataFrame::ndistinct: no features");

  std::vector<std::int64_t> values;
  values.reserve(names.size());
  for (const auto& name : names) {
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::ndistinct: unknown feature '" + name + "'");
    values.push_back(src.columns_[it->second].ndistinct(dropna).as_series().item<std::int64_t>());
  }

  return make_i64_thread(values)
      .with_display_labels(make_note_thread(names))
      .with_display_name("ndistinct");
}

Thread DataFrame::freq(pluss::msharp::RangeView::Of<std::string_view> features,
                       std::int64_t top_n,
                       bool dropna) const {
  const auto scoped = features.empty() ? *this : this->features(features);
  if (scoped.nfeatures() == 1) {
    return scoped.features(scoped.feature_names().front()).freq(top_n, dropna);
  }

  std::unordered_map<std::string, std::int64_t> counts;
  std::vector<std::string> order;
  for (std::int64_t row = 0; row < scoped.ncases(); ++row) {
    bool observed = true;
    std::string key;
    for (const auto& name : scoped.feature_names()) {
      const auto column = scoped.features(name);
      if (!column.is_valid(row)) {
        observed = false;
        if (dropna) break;
      }
      if (!key.empty()) key += "|";
      key += observed ? thread_scalar_string_at(column, row) : "NA";
    }
    if (!observed && dropna) continue;
    if (!counts.contains(key)) order.push_back(key);
    counts[key] += 1;
  }
  std::sort(order.begin(), order.end(), [&](const auto& lhs, const auto& rhs) {
    if (counts[lhs] != counts[rhs]) return counts[lhs] > counts[rhs];
    return lhs < rhs;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < order.size()) {
    order.resize(static_cast<std::size_t>(top_n));
  }
  std::vector<std::string> keys;
  std::vector<std::int64_t> values;
  keys.reserve(order.size());
  values.reserve(order.size());
  for (const auto& key : order) {
    keys.push_back(key);
    values.push_back(counts[key]);
  }
  return make_i64_thread(values)
      .with_display_labels(Thread(pluss::make_note(keys)))
      .with_display_name("freq");
}

Thread DataFrame::duplicated(pluss::msharp::RangeView::Of<std::string_view> features,
                             bool keep_first,
                             bool keep_last) const {
  PLUSS_CHECK(!(keep_first && keep_last),
              "DataFrame::duplicated: keep_first and keep_last cannot both be true");
  const auto scoped = features.empty() ? *this : this->features(features);
  if (scoped.nfeatures() == 1) {
    const auto name = scoped.feature_names().front();
    return scoped.features(name)
        .duplicated(keep_first, keep_last)
        .with_display_labels(scoped.cases())
        .with_display_name("duplicated");
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_key;
  rows_by_key.reserve(static_cast<std::size_t>(scoped.ncases()));
  for (std::int64_t row = 0; row < scoped.ncases(); ++row) {
    std::string key;
    for (const auto& name : scoped.feature_names()) {
      const auto column = scoped.features(name);
      if (!key.empty()) key += "|";
      key += column.is_valid(row) ? thread_scalar_string_at(column, row) : "NA";
    }
    rows_by_key[key].push_back(row);
  }
  std::vector<bool> mask(static_cast<std::size_t>(scoped.ncases()), false);
  for (const auto& [_, rows] : rows_by_key) {
    if (rows.size() <= 1) continue;
    const std::size_t start = keep_first ? 1 : 0;
    const std::size_t stop = keep_last ? rows.size() - 1 : rows.size();
    for (std::size_t i = start; i < stop; ++i) {
      mask[static_cast<std::size_t>(rows[i])] = true;
    }
  }
  return Thread(bool_series_from_keep(mask))
      .with_display_labels(scoped.cases())
      .with_display_name("duplicated");
}

DataFrame DataFrame::drop_duplicates(pluss::msharp::RangeView::Of<std::string_view> features,
                                     bool keep_first,
                                     bool keep_last) const {
  const auto duplicated_mask = duplicated(features, keep_first, keep_last).as_series();
  return filter(duplicated_mask.logical_not());
}

DataFrameView DataFrame::corr(CorrMethod method, bool render) const {
  return corr({}, method, render);
}

DataFrameView DataFrame::corr(bool render) const {
  return corr({}, CorrMethod::Pearson, render);
}

DataFrameView DataFrame::corr(pluss::msharp::RangeView::Of<std::string_view> features, bool render) const {
  return corr(features, CorrMethod::Pearson, render);
}

DataFrameView DataFrame::corr(pluss::msharp::RangeView::Of<std::string_view> features,
                              CorrMethod method,
                              bool render) const {
  const auto& src = impl_ref_();

  std::vector<std::string> names;
  if (features.size() == 0) {
    names.reserve(src.names_.size());
    for (std::size_t i = 0; i < src.names_.size(); ++i) {
      const auto& col = src.columns_[i];
      if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
        names.push_back(src.names_[i]);
      }
    }
  } else {
    std::unordered_set<std::string> seen;
    seen.reserve(features.size());
    names.reserve(features.size());
    for (const auto feature : features) {
      const auto key = owned_name(feature);
      PLUSS_CHECK(!key.empty(), "DataFrame::corr: feature name cannot be empty");
      if (!seen.insert(key).second) {
        continue;
      }
      const auto it = src.name_to_idx_.find(key);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::corr: unknown feature '" + key + "'");
      const auto& col = src.columns_[it->second];
      PLUSS_CHECK(col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series()),
                  "DataFrame::corr: feature '" + key + "' must be numeric");
      names.push_back(key);
    }
  }

  PLUSS_CHECK(!names.empty(), "DataFrame::corr: no numeric features");

  const auto feature_count = names.size();
  std::vector<Thread> numeric_threads;
  numeric_threads.reserve(feature_count);
  for (const auto& name : names) {
    numeric_threads.push_back(src.columns_[src.name_to_idx_.at(name)]);
  }

  ThreadCorrMethod thread_method = ThreadCorrMethod::Pearson;
  switch (method) {
    case CorrMethod::Pearson:
      thread_method = ThreadCorrMethod::Pearson;
      break;
    case CorrMethod::Spearman:
      thread_method = ThreadCorrMethod::Spearman;
      break;
    case CorrMethod::KendallTau:
      thread_method = ThreadCorrMethod::KendallTau;
      break;
    default:
      thread_method = ThreadCorrMethod::Pearson;
      break;
  }

  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> corr_matrix(feature_count * feature_count, nan);
  for (std::size_t i = 0; i < feature_count; ++i) {
    for (std::size_t j = i; j < feature_count; ++j) {
      const double corr = numeric_threads[i].corr(numeric_threads[j], thread_method);
      corr_matrix[i * feature_count + j] = corr;
      corr_matrix[j * feature_count + i] = corr;
    }
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(feature_count);
  for (std::size_t c = 0; c < feature_count; ++c) {
    std::vector<double> col(feature_count, nan);
    for (std::size_t r = 0; r < feature_count; ++r) {
      col[r] = corr_matrix[r * feature_count + c];
    }
    out_cols.emplace_back(names[c], make_f64_thread(col));
  }

  auto out = DataFrame(std::move(out_cols), make_note_thread(names));
  return DataFrameView(std::move(out), render, "corr", title());
}

DataFrame DataFrame::missing(const MissingSpec& spec) const {
  const auto& src = impl_ref_();
  std::vector<std::size_t> target_indices;
  target_indices.reserve(spec.features.empty() ? src.names_.size() : spec.features.size());

  if (spec.features.empty()) {
    for (std::size_t i = 0; i < src.names_.size(); ++i) {
      target_indices.push_back(i);
    }
  } else {
    std::unordered_set<std::size_t> seen;
    seen.reserve(spec.features.size());
    for (const auto& name : spec.features) {
      const auto it = src.name_to_idx_.find(name);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::missing: unknown feature '" + name + "'");
      if (seen.insert(it->second).second) {
        target_indices.push_back(it->second);
      }
    }
  }

  std::vector<bool> is_target(src.names_.size(), false);
  for (const auto idx : target_indices) {
    is_target[idx] = true;
  }

  if (spec.op == MissingOp::IsNull || spec.op == MissingOp::NotNull) {
    std::vector<std::pair<std::string, Thread>> out_cols;
    out_cols.reserve(target_indices.size());
    for (const auto idx : target_indices) {
      const auto mask = (spec.op == MissingOp::IsNull) ? src.columns_[idx].isnull() : src.columns_[idx].notnull();
      out_cols.emplace_back(src.names_[idx], mask);
    }
    return DataFrame(std::move(out_cols), src.cases_);
  }

  if (spec.op == MissingOp::DropAny || spec.op == MissingOp::DropAll) {
    if (src.nrows_ == 0 || target_indices.empty()) {
      return *this;
    }

    std::optional<pluss::Series> keep = std::nullopt;
    for (const auto idx : target_indices) {
      const auto valid = src.columns_[idx].notnull().as_series();
      if (!keep.has_value()) {
        keep = valid;
        continue;
      }
      keep = spec.op == MissingOp::DropAny ? keep->logical_and(valid) : keep->logical_or(valid);
    }

    if (!keep.has_value()) {
      return *this;
    }
    return filter(*keep);
  }

  if (spec.op == MissingOp::Fill) {
    PLUSS_CHECK(spec.value.has_value(), "DataFrame::missing(fill): value must be provided");
  }

  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (!is_target[i]) {
      out_cols.push_back(col);
      continue;
    }

    switch (spec.op) {
      case MissingOp::Fill:
        if (col.kind() == ThreadKind::Series) {
          out_cols.push_back(col.fillna(*spec.value));
        } else {
          out_cols.push_back(col);
        }
        break;
      case MissingOp::ForwardFill:
        out_cols.push_back(col.ffill());
        break;
      case MissingOp::BackwardFill:
        out_cols.push_back(col.bfill());
        break;
      default:
        out_cols.push_back(col);
        break;
    }
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::missing(
    MissingOp op,
    pluss::msharp::RangeView::Of<std::string_view> features,
    std::optional<double> value) const {
  MissingSpec spec;
  spec.op = op;
  spec.features = normalize_name_list(features);
  spec.value = value;
  return missing(spec);
}

DataFrame DataFrame::isnull(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return missing(MissingOp::IsNull, features);
}

DataFrame DataFrame::notnull(pluss::msharp::RangeView::Of<std::string_view> features) const {
  return missing(MissingOp::NotNull, features);
}

DataFrame DataFrame::cast(pluss::msharp::RangeView::Of<CastSpec> specs) const {
  if (specs.size() == 0) {
    return *this;
  }

  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::cast: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast: unknown feature '" + spec.feature + "'");
    if (!seen.insert(spec.feature).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].cast(spec.target, spec.options);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::cast(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(feature_dtypes.size());
  for (const auto& [name, dtype] : feature_dtypes) {
    const auto key = owned_name(name);
    PLUSS_CHECK(!key.empty(), "DataFrame::cast: feature name cannot be empty");
    const auto it = src.name_to_idx_.find(key);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::cast: unknown feature '" + key + "'");
    if (!seen.insert(key).second) {
      continue;
    }
    out_cols[it->second] = src.columns_[it->second].astype(dtype);
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::astype(
    pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const {
  return cast(feature_dtypes);
}

DataFrame DataFrame::window(pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input) const {
  PLUSS_CHECK(specs.size() != 0, "DataFrame::window: at least one WindowSpec is required");
  const auto& src = impl_ref_();

  std::vector<std::pair<std::string, Thread>> computed;
  computed.reserve(specs.size());

  std::unordered_set<std::string> names_seen;
  if (keep_input) {
    names_seen.insert(src.names_.begin(), src.names_.end());
  }

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::window: WindowSpec.feature cannot be empty");
    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::window: unknown feature '" + spec.feature + "'");
    PLUSS_CHECK(spec.window > 0, "DataFrame::window: WindowSpec.window must be > 0");
    PLUSS_CHECK(spec.min_periods > 0, "DataFrame::window: WindowSpec.min_periods must be > 0");

    auto out_name = spec.output;
    if (out_name.empty()) {
      out_name = fmt::format("{}_{}_{}", spec.feature, window_op_name(spec.op), spec.window);
    }

    if (!keep_input) {
      PLUSS_CHECK(names_seen.insert(out_name).second,
                  "DataFrame::window: duplicate output feature '" + out_name + "'");
    }

    computed.emplace_back(
        out_name,
        src.columns_[it->second].window(spec.window, spec.op, spec.min_periods));
  }

  if (!keep_input) {
    return DataFrame(std::move(computed), src.cases_);
  }

  auto out_names = src.names_;
  auto out_cols = src.columns_;
  auto out_name_to_idx = src.name_to_idx_;

  for (auto& [name, thread] : computed) {
    const auto it = out_name_to_idx.find(name);
    if (it != out_name_to_idx.end()) {
      out_cols[it->second] = std::move(thread);
      continue;
    }
    const auto next = out_cols.size();
    out_name_to_idx.emplace(name, next);
    out_names.push_back(name);
    out_cols.push_back(std::move(thread));
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      std::move(out_names), std::move(out_cols), src.cases_));
}

DataFrame DataFrame::rolling(pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input) const {
  return window(specs, keep_input);
}

DataFrame DataFrame::fillna(double value) const {
  MissingSpec spec;
  spec.op = MissingOp::Fill;
  spec.value = value;
  return missing(spec);
}

DataFrame DataFrame::fillna(double value, pluss::msharp::RangeView::Of<std::string_view> features) const {
  MissingSpec spec;
  spec.op = MissingOp::Fill;
  spec.value = value;
  spec.features = normalize_name_list(features);
  return missing(spec);
}

DataFrame DataFrame::fillna(std::string_view value) const {
  return fillna(value, {});
}

DataFrame DataFrame::fillna(std::string_view value, pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  std::unordered_set<std::size_t> target_indices;

  if (features.size() == 0) {
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      if (src.columns_[i].kind() == ThreadKind::Note) {
        target_indices.insert(i);
      }
    }
  } else {
    for (const auto& name : normalize_name_list(features)) {
      const auto it = src.name_to_idx_.find(name);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::fillna(string): unknown feature '" + name + "'");
      target_indices.insert(it->second);
    }
  }

  if (target_indices.empty()) {
    return *this;
  }

  std::vector<Thread> out_cols = src.columns_;
  bool changed = false;
  for (const auto idx : target_indices) {
    auto& col = out_cols[idx];
    if (col.kind() == ThreadKind::Note && col.has_validity_mask()) {
      col = col.fillna(value);
      changed = true;
    }
  }

  if (!changed) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::fillna(const pluss::Timeline& value) const {
  return fillna(value, {});
}

DataFrame DataFrame::fillna(
    const pluss::Timeline& value,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  std::unordered_set<std::size_t> target_indices;

  if (features.size() == 0) {
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      const auto kind = src.columns_[i].kind();
      if (kind == ThreadKind::Timeline || kind == ThreadKind::TimeIndex) {
        target_indices.insert(i);
      }
    }
  } else {
    for (const auto& name : normalize_name_list(features)) {
      const auto it = src.name_to_idx_.find(name);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::fillna(Timeline): unknown feature '" + name + "'");
      target_indices.insert(it->second);
    }
  }

  if (target_indices.empty()) {
    return *this;
  }

  std::vector<Thread> out_cols = src.columns_;
  bool changed = false;
  for (const auto idx : target_indices) {
    auto& col = out_cols[idx];
    const auto kind = col.kind();
    if ((kind == ThreadKind::Timeline || kind == ThreadKind::TimeIndex) && col.has_validity_mask()) {
      col = col.fillna(value);
      changed = true;
    }
  }

  if (!changed) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame DataFrame::fillna(const pluss::TimeIndex& value) const {
  return fillna(value, {});
}

DataFrame DataFrame::fillna(
    const pluss::TimeIndex& value,
    pluss::msharp::RangeView::Of<std::string_view> features) const {
  const auto& src = impl_ref_();
  std::unordered_set<std::size_t> target_indices;

  if (features.size() == 0) {
    for (std::size_t i = 0; i < src.columns_.size(); ++i) {
      if (src.columns_[i].kind() == ThreadKind::TimeIndex) {
        target_indices.insert(i);
      }
    }
  } else {
    for (const auto& name : normalize_name_list(features)) {
      const auto it = src.name_to_idx_.find(name);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::fillna(TimeIndex): unknown feature '" + name + "'");
      target_indices.insert(it->second);
    }
  }

  if (target_indices.empty()) {
    return *this;
  }

  std::vector<Thread> out_cols = src.columns_;
  bool changed = false;
  for (const auto idx : target_indices) {
    auto& col = out_cols[idx];
    if (col.kind() == ThreadKind::TimeIndex && col.has_validity_mask()) {
      col = col.fillna(value);
      changed = true;
    }
  }

  if (!changed) {
    return *this;
  }
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(
      src.names_, std::move(out_cols), src.cases_, src.title_));
}

DataFrame& DataFrame::fillna_(double value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    if (c.kind() == ThreadKind::Series && c.has_validity_mask()) {
      c.fillna_(value);
    }
  }
  return *this;
}

DataFrame& DataFrame::fillna_(std::string_view value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    if (c.kind() == ThreadKind::Note && c.has_validity_mask()) {
      c.fillna_(value);
    }
  }
  return *this;
}

DataFrame& DataFrame::fillna_(const pluss::Timeline& value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    const auto kind = c.kind();
    if ((kind == ThreadKind::Timeline || kind == ThreadKind::TimeIndex) && c.has_validity_mask()) {
      c.fillna_(value);
    }
  }
  return *this;
}

DataFrame& DataFrame::fillna_(const pluss::TimeIndex& value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    if (c.kind() == ThreadKind::TimeIndex && c.has_validity_mask()) {
      c.fillna_(value);
    }
  }
  return *this;
}

DataFrame DataFrame::dropna(pluss::msharp::RangeView::Of<std::string_view> features,
                            bool all) const {
  return missing(all ? MissingOp::DropAll : MissingOp::DropAny, features);
}

DataFrame DataFrame::ffill() const {
  return missing(MissingOp::ForwardFill);
}

DataFrame DataFrame::bfill() const {
  return missing(MissingOp::BackwardFill);
}

DataFrameView DataFrame::describe(bool render) const {
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(src.columns_.size());

  for (std::size_t i = 0; i < src.columns_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() != ThreadKind::Series) {
      continue;
    }
    const auto& s = col.as_series();
    if (!is_numeric_series_type(s)) {
      continue;
    }

    Thread observed = col;
    if (col.has_validity_mask()) {
      observed = col.filter(*col.validity_mask());
    }
    const auto observed_series = observed.as_series().to(pluss::codem::ScalarType::Float64);
    const auto observed_n = observed_series.numel();
    const double count = static_cast<double>(observed_n);
    double mean = std::numeric_limits<double>::quiet_NaN();
    double stdv = std::numeric_limits<double>::quiet_NaN();
    double minv = std::numeric_limits<double>::quiet_NaN();
    double q25 = std::numeric_limits<double>::quiet_NaN();
    double q50 = std::numeric_limits<double>::quiet_NaN();
    double q75 = std::numeric_limits<double>::quiet_NaN();
    double maxv = std::numeric_limits<double>::quiet_NaN();

    if (observed_n > 0) {
      mean = observed_series.mean().item<double>();
      minv = observed_series.min().item<double>();
      q25 = observed_series.quantile(0.25).item<double>();
      q50 = observed_series.quantile(0.50).item<double>();
      q75 = observed_series.quantile(0.75).item<double>();
      maxv = observed_series.max().item<double>();
      if (observed_n > 1) {
        stdv = observed_series.std(true).item<double>();
      }
    }

    std::vector<double> stats{count, mean, stdv, minv, q25, q50, q75, maxv};
    out_cols.emplace_back(src.names_[i], make_f64_thread(stats));
  }

  PLUSS_CHECK(!out_cols.empty(), "DataFrame::describe: no numeric columns to describe");

  auto out = DataFrame(
      std::move(out_cols),
      make_note_thread({"count", "mean", "std", "min", "25%", "50%", "75%", "max"}));
  return DataFrameView(std::move(out), render, "describe", title());
}

DataFrameView DataFrame::tally(std::int64_t top_n, bool dropna, bool render) const {
  return tally_impl(impl_ref_().names_, top_n, dropna, render);
}

DataFrameView DataFrame::tally(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                               std::int64_t top_n,
                               bool dropna,
                               bool render) const {
  return tally_impl(normalize_name_list(feature_names), top_n, dropna, render);
}

DataFrameView DataFrame::tally(std::string_view feature_name,
                               std::int64_t top_n,
                               bool dropna,
                               bool render) const {
  return tally_impl({owned_name(feature_name)}, top_n, dropna, render);
}

DataFrameView DataFrame::tally_impl(const std::vector<std::string>& feature_names,
                                    std::int64_t top_n,
                                    bool dropna,
                                    bool render) const {
  PLUSS_CHECK(!feature_names.empty(), "DataFrame::tally: at least one feature is required");
  PLUSS_CHECK(top_n >= 0, "DataFrame::tally: top_n must be >= 0");

  const auto& src = impl_ref_();
  std::vector<std::string> keys;
  keys.reserve(feature_names.size());
  std::unordered_set<std::string> seen_keys;
  seen_keys.reserve(feature_names.size());

  struct KeyInfo {
    std::string name;
    Thread value_col;
    std::optional<std::vector<bool>> valid;
  };

  std::vector<KeyInfo> key_infos;
  key_infos.reserve(feature_names.size());
  for (const auto& raw : feature_names) {
    const auto name = owned_name(raw);
    PLUSS_CHECK(!name.empty(), "DataFrame::tally: feature name cannot be empty");
    PLUSS_CHECK(seen_keys.insert(name).second, "DataFrame::tally: duplicate feature '" + name + "'");
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::tally: unknown feature '" + name + "'");
    const auto& col = src.columns_[it->second];
    key_infos.push_back({
        name,
        col.has_validity_mask() ? col.clear_validity_mask() : col,
        col.has_validity_mask() ? std::optional<std::vector<bool>>(col.validity_mask()->to_vec<bool>()) : std::nullopt,
    });
    keys.push_back(name);
  }

  struct Row {
    std::vector<std::string> key_values;
    std::int64_t count{0};
  };

  std::vector<Row> rows;
  rows.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, ncases() / 32)));
  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(rows.capacity());

  for (std::int64_t r = 0; r < ncases(); ++r) {
    std::vector<std::string> key_values;
    key_values.reserve(key_infos.size());

    bool skip = false;
    for (const auto& key_info : key_infos) {
      const bool valid =
          !key_info.valid.has_value() || (*key_info.valid)[static_cast<std::size_t>(r)];
      if (!valid && dropna) {
        skip = true;
        break;
      }
      key_values.push_back(valid ? thread_scalar_string_at(key_info.value_col, r) : "NA");
    }
    if (skip) {
      continue;
    }

    std::string composite;
    for (std::size_t i = 0; i < key_values.size(); ++i) {
      if (i != 0) {
        composite.push_back('\x1f');
      }
      composite += key_values[i];
    }

    const auto [it, inserted] = index_by_key.emplace(composite, rows.size());
    if (inserted) {
      rows.push_back(Row{std::move(key_values), 1});
    } else {
      rows[it->second].count += 1;
    }
  }

  std::sort(rows.begin(), rows.end(), [](const Row& lhs, const Row& rhs) {
    if (lhs.count != rhs.count) {
      return lhs.count > rhs.count;
    }
    return lhs.key_values < rhs.key_values;
  });
  if (top_n > 0 && static_cast<std::size_t>(top_n) < rows.size()) {
    rows.resize(static_cast<std::size_t>(top_n));
  }

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(keys.size() + 1);
  for (std::size_t k = 0; k < keys.size(); ++k) {
    std::vector<std::string> values;
    values.reserve(rows.size());
    for (const auto& row : rows) {
      values.push_back(row.key_values[k]);
    }
    out_cols.emplace_back(keys[k], make_note_thread(values));
  }

  std::vector<std::int64_t> counts;
  counts.reserve(rows.size());
  for (const auto& row : rows) {
    counts.push_back(row.count);
  }
  out_cols.emplace_back("count", make_i64_thread(counts));

  auto out = DataFrame(std::move(out_cols));
  return DataFrameView(std::move(out), render, "tally", title());
}

DataFrameView DataFrame::view(std::int64_t start, std::int64_t stop, bool render) const {
  const auto n = ncases();
  const auto s = normalize_view_start(start, n);
  const auto e = normalize_view_stop(stop, n);

  DataFrame out;
  if (e <= s) {
    out = slice(s, s);
  } else if (s == 0) {
    out = head(e, false);
  } else if (e == n) {
    out = tail(n - s, false);
  } else {
    auto after_start = tail(n - s, false);
    out = after_start.head(e - s, false);
  }

  return DataFrameView(std::move(out), render, "view", title());
}

DataFrame DataFrame::copy() const {
  return *this;
}

bool DataFrame::equals(const DataFrame& other) const {
  if (ncases() != other.ncases() || nfeats() != other.nfeats()) {
    return false;
  }
  if (feature_names() != other.feature_names()) {
    return false;
  }
  if (cases().dtype_name() != other.cases().dtype_name()) {
    return false;
  }
  for (std::int64_t r = 0; r < ncases(); ++r) {
    if (cases().is_valid(r) != other.cases().is_valid(r)) {
      return false;
    }
    if (!cases().is_valid(r)) {
      continue;
    }
    if (thread_scalar_string_at(cases(), r) != thread_scalar_string_at(other.cases(), r)) {
      return false;
    }
  }
  for (const auto& name : feature_names()) {
    const auto lhs = features(name);
    const auto rhs = other.features(name);
    if (lhs.kind() != rhs.kind() || lhs.size() != rhs.size()) {
      return false;
    }
    for (std::int64_t r = 0; r < lhs.size(); ++r) {
      if (lhs.is_valid(r) != rhs.is_valid(r)) {
        return false;
      }
      if (!lhs.is_valid(r)) {
        continue;
      }
      if (thread_scalar_string_at(lhs, r) != thread_scalar_string_at(rhs, r)) {
        return false;
      }
    }
  }
  return true;
}
