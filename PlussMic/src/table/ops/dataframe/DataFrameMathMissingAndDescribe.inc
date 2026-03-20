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

DataFrame DataFrame::subtract(double other) const {
  return sub(other);
}

DataFrame DataFrame::subtract(const DataFrame& other) const {
  return sub(other);
}

DataFrame DataFrame::mul(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Mul, false);
}

DataFrame DataFrame::mul(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Mul);
}

DataFrame DataFrame::multiply(double other) const {
  return mul(other);
}

DataFrame DataFrame::multiply(const DataFrame& other) const {
  return mul(other);
}

DataFrame DataFrame::div(double other) const {
  return dataframe_math_scalar(*this, other, BinaryMathOp::Div, false);
}

DataFrame DataFrame::div(const DataFrame& other) const {
  return dataframe_math_frame(*this, other, BinaryMathOp::Div);
}

DataFrame DataFrame::divide(double other) const {
  return div(other);
}

DataFrame DataFrame::divide(const DataFrame& other) const {
  return div(other);
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

DataFrame DataFrame::lt(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Lt);
}

DataFrame DataFrame::lt(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Lt);
}

DataFrame DataFrame::le(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Le);
}

DataFrame DataFrame::le(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Le);
}

DataFrame DataFrame::gt(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Gt);
}

DataFrame DataFrame::gt(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Gt);
}

DataFrame DataFrame::ge(double other) const {
  return dataframe_cmp_scalar(*this, other, BinaryCmpOp::Ge);
}

DataFrame DataFrame::ge(const DataFrame& other) const {
  return dataframe_cmp_frame(*this, other, BinaryCmpOp::Ge);
}

DataFrame DataFrame::compare(double other) const {
  return ne(other);
}

DataFrame DataFrame::compare(const DataFrame& other) const {
  return ne(other);
}

DataFrameView DataFrame::sum(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Sum});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::sum: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "sum", title());
}

DataFrameView DataFrame::mean(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Mean});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::mean: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "mean", title());
}

DataFrameView DataFrame::median(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Median});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::median: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "median", title());
}

DataFrameView DataFrame::min(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Min});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::min: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "min", title());
}

DataFrameView DataFrame::max(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Max});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::max: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "max", title());
}

DataFrameView DataFrame::std(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Std});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::std: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "std", title());
}

DataFrameView DataFrame::var(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& col = src.columns_[i];
    if (col.kind() == ThreadKind::Series && is_numeric_series_type(col.as_series())) {
      specs.push_back(SummarySpec{src.names_[i], src.names_[i], SummaryOp::Var});
    }
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::var: no numeric features");
  return DataFrameView(summarize_specs(specs, false), render, "var", title());
}

DataFrameView DataFrame::count(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    specs.push_back(SummarySpec{name, name, SummaryOp::Count});
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::count: no features");
  return DataFrameView(summarize_specs(specs, false), render, "count", title());
}

namespace {

std::int64_t normalize_bool_reduce_axis_(std::int64_t axis, std::string_view where) {
  auto out = axis;
  if (out < 0) {
    out += 2;
  }
  PLUSS_CHECK(out == 0 || out == 1, std::string(where) + ": axis must be 0 (features) or 1 (cases)");
  return out;
}

std::vector<std::size_t> resolve_bool_reduce_indices_(
    const detail::DataFrameImpl& src,
    std::initializer_list<std::string_view> features,
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

Thread prepare_bool_feature_for_reduce_(
    const Thread& col,
    bool use_all,
    bool skipna,
    std::string_view where) {
  PLUSS_CHECK(col.kind() == ThreadKind::Series, std::string(where) + ": feature must be a Bool Series");
  PLUSS_CHECK(col.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
              std::string(where) + ": feature must be a Bool Series");
  if (!col.has_validity_mask()) {
    return col;
  }
  PLUSS_CHECK(skipna,
              std::string(where)
                  + ": skipna=false is not supported for nullable bool features (use skipna=true)");
  const auto valid = col.notnull().as_series();
  const auto prepared =
      use_all ? col.as_series().logical_or(valid.logical_not()) : col.as_series().logical_and(valid);
  return Thread(prepared);
}

Thread reduce_bool_rows_(
    const detail::DataFrameImpl& src,
    std::initializer_list<std::string_view> features,
    bool use_all,
    bool skipna,
    std::string_view where) {
  const auto indices = resolve_bool_reduce_indices_(src, features, where);
  auto mask = prepare_bool_feature_for_reduce_(src.columns_[indices.front()], use_all, skipna, where);
  for (std::size_t i = 1; i < indices.size(); ++i) {
    const auto col = prepare_bool_feature_for_reduce_(src.columns_[indices[i]], use_all, skipna, where);
    mask = use_all ? (mask & col) : (mask | col);
  }
  return mask;
}

Thread reduce_bool_features_(
    const detail::DataFrameImpl& src,
    std::initializer_list<std::string_view> features,
    bool use_all,
    bool skipna,
    std::string_view where) {
  const auto indices = resolve_bool_reduce_indices_(src, features, where);

  std::vector<bool> reduced;
  reduced.reserve(indices.size());
  std::vector<std::string> labels;
  labels.reserve(indices.size());

  for (const auto idx : indices) {
    const auto prepared = prepare_bool_feature_for_reduce_(src.columns_[idx], use_all, skipna, where);
    const bool value = use_all ? prepared.all(skipna).item<bool>() : prepared.any(skipna).item<bool>();
    reduced.push_back(value);
    labels.push_back(src.names_[idx]);
  }

  auto out = Thread(bool_series_from_keep(reduced));
  if (!labels.empty()) {
    out = out.with_display_labels(make_note_thread(labels));
  }
  return out;
}

} // namespace

Thread DataFrame::any(std::initializer_list<std::string_view> features, bool skipna) const {
  return any(1, features, skipna);
}

Thread DataFrame::any(std::int64_t axis, std::initializer_list<std::string_view> features, bool skipna) const {
  const auto normalized_axis = normalize_bool_reduce_axis_(axis, "DataFrame::any");
  if (normalized_axis == 0) {
    return reduce_bool_features_(impl_ref_(), features, false, skipna, "DataFrame::any");
  }
  return reduce_bool_rows_(impl_ref_(), features, false, skipna, "DataFrame::any");
}

Thread DataFrame::all(std::initializer_list<std::string_view> features, bool skipna) const {
  return all(1, features, skipna);
}

Thread DataFrame::all(std::int64_t axis, std::initializer_list<std::string_view> features, bool skipna) const {
  const auto normalized_axis = normalize_bool_reduce_axis_(axis, "DataFrame::all");
  if (normalized_axis == 0) {
    return reduce_bool_features_(impl_ref_(), features, true, skipna, "DataFrame::all");
  }
  return reduce_bool_rows_(impl_ref_(), features, true, skipna, "DataFrame::all");
}

DataFrameView DataFrame::count_distinct(std::initializer_list<std::string_view> features, bool dropna, bool render) const {
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
      PLUSS_CHECK(!key.empty(), "DataFrame::count_distinct: feature name cannot be empty");
      if (!seen.insert(key).second) {
        continue;
      }
      const auto it = src.name_to_idx_.find(key);
      PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::count_distinct: unknown feature '" + key + "'");
      names.push_back(key);
    }
  }

  PLUSS_CHECK(!names.empty(), "DataFrame::count_distinct: no features");

  std::vector<std::pair<std::string, Thread>> out_cols;
  out_cols.reserve(names.size());
  for (const auto& name : names) {
    const auto it = src.name_to_idx_.find(name);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::count_distinct: unknown feature '" + name + "'");
    const auto distinct_count = src.columns_[it->second].distinct(dropna).size();
    out_cols.emplace_back(name, make_i64_thread({distinct_count}));
  }

  auto out = DataFrame(std::move(out_cols), make_note_thread({"summary"}));
  return DataFrameView(std::move(out), render, "count_distinct", title());
}

DataFrameView DataFrame::corr(CorrMethod method, bool render) const {
  return corr({}, method, render);
}

DataFrameView DataFrame::corr(bool render) const {
  return corr({}, CorrMethod::Pearson, render);
}

DataFrameView DataFrame::corr(std::initializer_list<std::string_view> features, bool render) const {
  return corr(features, CorrMethod::Pearson, render);
}

DataFrameView DataFrame::corr(std::initializer_list<std::string_view> features, CorrMethod method, bool render) const {
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

    bool has_any_mask = false;
    std::vector<bool> keep(
        static_cast<std::size_t>(src.nrows_),
        spec.op == MissingOp::DropAny);

    for (const auto idx : target_indices) {
      const auto& col = src.columns_[idx];
      if (!col.has_validity_mask()) {
        if (spec.op == MissingOp::DropAll) {
          std::fill(keep.begin(), keep.end(), true);
        }
        continue;
      }

      has_any_mask = true;
      const auto valid = col.validity_mask()->to_vec<bool>();
      for (std::int64_t r = 0; r < src.nrows_; ++r) {
        const auto k = static_cast<std::size_t>(r);
        if (spec.op == MissingOp::DropAny) {
          keep[k] = keep[k] && valid[k];
        } else {
          keep[k] = keep[k] || valid[k];
        }
      }
    }

    if (!has_any_mask) {
      return *this;
    }
    return filter(bool_series_from_keep(keep));
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
        if (col.kind() == ThreadKind::Series && col.has_validity_mask()) {
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

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::missing(
    MissingOp op,
    std::initializer_list<std::string_view> features,
    std::optional<double> value) const {
  MissingSpec spec;
  spec.op = op;
  spec.features = normalize_name_list(features);
  spec.value = value;
  return missing(spec);
}

DataFrame DataFrame::isna(std::initializer_list<std::string_view> features) const {
  return missing(MissingOp::IsNull, features);
}

DataFrame DataFrame::isnull(std::initializer_list<std::string_view> features) const {
  return isna(features);
}

DataFrame DataFrame::notna(std::initializer_list<std::string_view> features) const {
  return missing(MissingOp::NotNull, features);
}

DataFrame DataFrame::notnull(std::initializer_list<std::string_view> features) const {
  return notna(features);
}

DataFrame DataFrame::cast(std::initializer_list<CastSpec> specs) const {
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
    out_cols[it->second] = src.columns_[it->second].cast(spec.target, spec.series_dtype);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::cast(
    std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const {
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::astype(
    std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const {
  return cast(feature_dtypes);
}

DataFrame DataFrame::window(std::initializer_list<WindowSpec> specs, bool keep_input) const {
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

DataFrame DataFrame::rolling(std::initializer_list<WindowSpec> specs, bool keep_input) const {
  return window(specs, keep_input);
}

DataFrame DataFrame::fillna(double value) const {
  MissingSpec spec;
  spec.op = MissingOp::Fill;
  spec.value = value;
  return missing(spec);
}

DataFrame DataFrame::fillna(double value, std::initializer_list<std::string_view> features) const {
  MissingSpec spec;
  spec.op = MissingOp::Fill;
  spec.value = value;
  spec.features = normalize_name_list(features);
  return missing(spec);
}

DataFrame DataFrame::fillna(std::string_view value) const {
  return fillna(value, {});
}

DataFrame DataFrame::fillna(std::string_view value, std::initializer_list<std::string_view> features) const {
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::fillna(const pluss::Timeline& value) const {
  return fillna(value, {});
}

DataFrame DataFrame::fillna(
    const pluss::Timeline& value,
    std::initializer_list<std::string_view> features) const {
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::fillna(const pluss::TimeIndex& value) const {
  return fillna(value, {});
}

DataFrame DataFrame::fillna(
    const pluss::TimeIndex& value,
    std::initializer_list<std::string_view> features) const {
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
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
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

std::int64_t DataFrame::count_null_rows() const {
  const auto& src = impl_ref_();
  if (src.nrows_ == 0) {
    return 0;
  }

  std::vector<bool> has_null(static_cast<std::size_t>(src.nrows_), false);
  for (const auto& c : src.columns_) {
    if (!c.has_validity_mask()) {
      continue;
    }
    const auto valid = c.validity_mask()->to_vec<bool>();
    for (std::int64_t i = 0; i < src.nrows_; ++i) {
      if (!valid[static_cast<std::size_t>(i)]) {
        has_null[static_cast<std::size_t>(i)] = true;
      }
    }
  }

  std::int64_t n = 0;
  for (const bool v : has_null) {
    if (v) {
      ++n;
    }
  }
  return n;
}

DataFrame DataFrame::dropna() const {
  return missing(MissingOp::DropAny);
}

DataFrame DataFrame::dropna_all(std::initializer_list<std::string_view> features) const {
  return missing(MissingOp::DropAll, features);
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

    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(col.size()));
    if (col.has_validity_mask()) {
      const auto valid = col.validity_mask()->to_vec<bool>();
      for (std::int64_t r = 0; r < col.size(); ++r) {
        if (!valid[static_cast<std::size_t>(r)]) {
          continue;
        }
        double v = 0.0;
        if (extract_numeric_value(s, r, v)) {
          values.push_back(v);
        }
      }
    } else {
      for (std::int64_t r = 0; r < col.size(); ++r) {
        double v = 0.0;
        if (extract_numeric_value(s, r, v)) {
          values.push_back(v);
        }
      }
    }

    const double count = static_cast<double>(values.size());
    double mean = std::numeric_limits<double>::quiet_NaN();
    double stdv = std::numeric_limits<double>::quiet_NaN();
    double minv = std::numeric_limits<double>::quiet_NaN();
    double q25 = std::numeric_limits<double>::quiet_NaN();
    double q50 = std::numeric_limits<double>::quiet_NaN();
    double q75 = std::numeric_limits<double>::quiet_NaN();
    double maxv = std::numeric_limits<double>::quiet_NaN();

    if (!values.empty()) {
      std::sort(values.begin(), values.end());
      minv = values.front();
      maxv = values.back();
      const double sum = std::accumulate(values.begin(), values.end(), 0.0);
      mean = sum / count;
      q25 = quantile_sorted(values, 0.25);
      q50 = quantile_sorted(values, 0.50);
      q75 = quantile_sorted(values, 0.75);

      if (values.size() > 1) {
        double sq = 0.0;
        for (const double v : values) {
          const double d = v - mean;
          sq += d * d;
        }
        stdv = std::sqrt(sq / static_cast<double>(values.size() - 1));
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

DataFrameView DataFrame::tally(std::initializer_list<std::string_view> feature_names,
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
