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

DataFrame DataFrame::sum(bool render) const {
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
  return summarize_specs(specs, render);
}

DataFrame DataFrame::mean(bool render) const {
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
  return summarize_specs(specs, render);
}

DataFrame DataFrame::min(bool render) const {
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
  return summarize_specs(specs, render);
}

DataFrame DataFrame::max(bool render) const {
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
  return summarize_specs(specs, render);
}

DataFrame DataFrame::std(bool render) const {
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
  return summarize_specs(specs, render);
}

DataFrame DataFrame::var(bool render) const {
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
  return summarize_specs(specs, render);
}

DataFrame DataFrame::count(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    specs.push_back(SummarySpec{name, name, SummaryOp::Count});
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::count: no features");
  return summarize_specs(specs, render);
}

DataFrame DataFrame::nunique(bool render) const {
  const auto& src = impl_ref_();
  std::vector<SummarySpec> specs;
  specs.reserve(src.names_.size());
  for (const auto& name : src.names_) {
    specs.push_back(SummarySpec{name, name, SummaryOp::Nunique});
  }
  PLUSS_CHECK(!specs.empty(), "DataFrame::nunique: no features");
  return summarize_specs(specs, render);
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
      out_cols.emplace_back(src.names_[idx], Thread(mask));
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

DataFrame& DataFrame::fillna_(double value) {
  auto& w = mut_impl_();
  for (auto& c : w.columns_) {
    if (c.kind() == ThreadKind::Series && c.has_validity_mask()) {
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

DataFrame DataFrame::describe(bool render) const {
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
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::tally(std::string_view feature_name,
                           std::int64_t top_n,
                           bool dropna,
                           bool render) const {
  return count_by(feature_name, top_n, dropna, render);
}

DataFrame DataFrame::view(std::int64_t start, std::int64_t stop, bool render) const {
  const auto n = nrows();
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

  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::copy() const {
  return *this;
}

bool DataFrame::equals(const DataFrame& other) const {
  if (nrows() != other.nrows() || ncols() != other.ncols()) {
    return false;
  }
  if (feature_names() != other.feature_names()) {
    return false;
  }
  if (cases().dtype_name() != other.cases().dtype_name()) {
    return false;
  }
  for (std::int64_t r = 0; r < nrows(); ++r) {
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
