namespace {

pluss::codem::ScalarType statistics_compute_type_(const pluss::Series& values) {
  return values.platform() == pluss::codem::Platform::CPU
             ? pluss::codem::ScalarType::Float64
             : pluss::codem::ScalarType::Float32;
}

pluss::Series column_from_matrix_(const pluss::Series& matrix, std::int64_t column) {
  auto index = pluss::Series::arange(
      column,
      column + 1,
      1,
      pluss::codem::ScalarType::Int64,
      matrix.platform());
  return matrix.index_select(1, index).reshape({matrix.sizes()[0]});
}

DataFrame frame_from_matrix_(const pluss::Series& matrix,
                             const pluss::Series& valid,
                             const std::vector<std::string>& names,
                             const Slate& cases) {
  PLUSS_CHECK(matrix.dim() == 2, "frame_from_matrix: expected a two-dimensional value matrix");
  PLUSS_CHECK(matrix.sizes()[1] == static_cast<std::int64_t>(names.size()),
              "frame_from_matrix: feature count mismatch");
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto value_column = column_from_matrix_(matrix, static_cast<std::int64_t>(i));
    const auto valid_column =
        valid.dim() == 1 ? valid : column_from_matrix_(valid, static_cast<std::int64_t>(i));
    columns.emplace_back(names[i], Slate(value_column, valid_column));
  }
  return DataFrame(std::move(columns), cases);
}

pluss::Series statistic_validity_(const ReduceMatrix_& matrix,
                                  std::int64_t axis,
                                  std::int64_t min_count) {
  PLUSS_CHECK(min_count >= 0, "statistics: min_count must be >= 0");
  return matrix.valid.to(pluss::codem::ScalarType::Int64).sum(axis).ge(
      static_cast<double>(min_count));
}

Slate labelled_statistic_(pluss::Series values,
                           pluss::Series valid,
                           const detail::DataFrameImpl& src,
                           const ReduceMatrix_& matrix,
                           std::int64_t axis) {
  return with_reduce_labels_(
      Slate(std::move(values), std::move(valid)), src, matrix.names, axis);
}

template <class Operation>
DataFrame delegate_numeric_slates_(const detail::DataFrameImpl& src,
                                    Operation&& operation) {
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    columns.emplace_back(
        src.names_[i],
        source.kind() == SlateKind::Series &&
                is_numeric_series_type(source.as_series())
            ? operation(source)
            : source);
  }
  return DataFrame(std::move(columns), src.cases_);
}

template <class Operation>
Slate reduce_numeric_slates_(const detail::DataFrameImpl& src,
                               Operation&& operation,
                               std::string_view where) {
  std::vector<std::string> names;
  std::vector<pluss::Series> values;
  std::vector<pluss::Series> validity;
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() != SlateKind::Series ||
        !is_numeric_series_type(source.as_series())) {
      continue;
    }
    auto reduced = operation(source).slate();
    PLUSS_CHECK(reduced.kind() == SlateKind::Series && reduced.size() == 1,
                std::string(where) +
                    ": Slate reduction must return one numeric value");
    names.push_back(src.names_[i]);
    values.push_back(reduced.as_series());
    validity.push_back(reduced.notnull().as_series());
  }
  PLUSS_CHECK(!values.empty(), std::string(where) + ": no numeric features");
  return Slate(
             pluss::series::pile(values, 0).reshape(
                 {static_cast<std::int64_t>(values.size())}),
             pluss::series::pile(validity, 0).reshape(
                 {static_cast<std::int64_t>(validity.size())}))
      .label().set(make_note_slate(names));
}

Slate pad_modes_(const Slate& source, std::int64_t size) {
  PLUSS_CHECK(source.size() <= size, "pad_modes: invalid output size");
  if (source.size() == size) {
    return source;
  }
  const auto padding = size - source.size();
  auto valid_padding = pluss::Series::zeros(
      {padding}, pluss::codem::ScalarType::Bool);
  if (source.kind() == SlateKind::Series) {
    const auto platform = source.as_series().platform();
    auto values = pluss::Series::zeros(
        {padding}, source.as_series().scalar_type(), platform);
    if (platform != pluss::codem::Platform::CPU) {
      valid_padding = valid_padding.to(platform);
    }
    return Slate(
        source.as_series().unite({values}, 0, true),
        source.notnull().as_series().unite({valid_padding}, 0, true));
  }
  if (source.kind() == SlateKind::Timeline || source.kind() == SlateKind::Duration) {
    std::vector<std::int64_t> indices;
    indices.reserve(static_cast<std::size_t>(size));
    for (std::int64_t i = 0; i < source.size(); ++i) {
      indices.push_back(i);
    }
    indices.resize(static_cast<std::size_t>(size), 0);
    auto output = source.take(indices);
    auto output_valid =
        source.notnull().as_series().unite(
            {valid_padding}, 0, true);
    const auto platform = output.kind() == SlateKind::Timeline
                              ? output.as_timeline().epoch_nanos().platform()
                              : pluss::codem::Platform::CPU;
    if (output_valid.platform() != platform) {
      output_valid = output_valid.to(platform);
    }
    return detail::with_validity_mask(output, output_valid);
  }
  std::vector<std::string> padded;
  padded.reserve(static_cast<std::size_t>(size));
  for (const auto value : source.as_note().vector()) {
    padded.emplace_back(value);
  }
  padded.resize(static_cast<std::size_t>(size));
  return Slate(
      pluss::make_note(padded),
      source.notnull().as_series().unite({valid_padding}, 0, true));
}

enum class RunningOp_ : std::uint8_t { Sum, Product, Minimum, Maximum };

DataFrame running_statistic_(const detail::DataFrameImpl& src,
                             RunningOp_ op,
                             bool skip_nulls,
                             std::string_view where) {
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() != SlateKind::Series ||
        !is_numeric_series_type(source.as_series())) {
      columns.emplace_back(src.names_[i], source);
      continue;
    }
    auto values = source.as_series().to(statistics_compute_type_(source.as_series()));
    auto valid = source.notnull().as_series().to(values.platform());
    const auto invalid = valid.logical_not();
    pluss::Series result = pluss::series::undefined();
    switch (op) {
      case RunningOp_::Sum:
        result = values.where(invalid, 0.0).cumsum(0);
        break;
      case RunningOp_::Product:
        result = values.where(invalid, 1.0).cumprod(0);
        break;
      case RunningOp_::Minimum:
        result = values.where(invalid, std::numeric_limits<double>::infinity()).cummin(0);
        break;
      case RunningOp_::Maximum:
        result = values.where(invalid, -std::numeric_limits<double>::infinity()).cummax(0);
        break;
    }
    auto out_valid = skip_nulls
                         ? valid
                         : valid.to(pluss::codem::ScalarType::Int64)
                               .cummin(0)
                               .to(pluss::codem::ScalarType::Bool);
    columns.emplace_back(src.names_[i], Slate(result, out_valid));
  }
  PLUSS_CHECK(!columns.empty(), std::string(where) + ": no features");
  return DataFrame(std::move(columns), src.cases_);
}

RunningOp_ running_op_from_scan_(ScanStatistic statistic, std::string_view where) {
  switch (statistic) {
    case ScanStatistic::Sum:
      return RunningOp_::Sum;
    case ScanStatistic::Product:
      return RunningOp_::Product;
    case ScanStatistic::Minimum:
      return RunningOp_::Minimum;
    case ScanStatistic::Maximum:
      return RunningOp_::Maximum;
  }
  PLUSS_FAIL(std::string(where) + ": unsupported scan statistic");
}

DataFrame running_statistic_matrix_(const detail::DataFrameImpl& src,
                                    RunningOp_ op,
                                    std::int64_t axis,
                                    bool skip_nulls,
                                    std::string_view where) {
  const auto matrix = numeric_reduce_matrix_(src, where);
  auto values = matrix.values;
  auto valid = matrix.valid;
  if (valid.platform() != values.platform()) {
    valid = valid.to(values.platform());
  }
  const auto invalid = valid.logical_not();

  pluss::Series result = pluss::series::undefined();
  switch (op) {
    case RunningOp_::Sum:
      result = values.where(invalid, 0.0).cumsum(axis);
      break;
    case RunningOp_::Product:
      result = values.where(invalid, 1.0).cumprod(axis);
      break;
    case RunningOp_::Minimum:
      result = values.where(invalid, std::numeric_limits<double>::infinity()).cummin(axis);
      break;
    case RunningOp_::Maximum:
      result = values.where(invalid, -std::numeric_limits<double>::infinity()).cummax(axis);
      break;
  }

  auto out_valid = skip_nulls
                       ? valid
                       : valid.to(pluss::codem::ScalarType::Int64)
                             .cummin(axis)
                             .to(pluss::codem::ScalarType::Bool);
  return frame_from_matrix_(result, out_valid, matrix.names, src.cases_);
}

enum class MovingOp_ : std::uint8_t {
  Sum,
  Mean,
  Minimum,
  Maximum,
  Variance,
  Deviation,
};

enum class MovingMomentOp_ : std::uint8_t {
  MeanError,
  Asymmetry,
  TailWeight,
};

Slate moving_statistic_(const Slate& source,
                         std::int64_t window,
                         const WindowOptions& options,
                         MovingOp_ op,
                         std::string_view where) {
  PLUSS_CHECK(source.kind() == SlateKind::Series,
              std::string(where) + ": expected a Series feature");
  PLUSS_CHECK(is_numeric_series_type(source.as_series()),
              std::string(where) + ": expected a numeric feature");
  PLUSS_CHECK(window > 0, std::string(where) + ": window must be > 0");
  PLUSS_CHECK(options.min_count > 0 && options.min_count <= window,
              std::string(where) + ": min_count must be in [1, window]");
  PLUSS_CHECK(options.ddof >= 0, std::string(where) + ": ddof must be >= 0");

  const auto n = source.size();
  const auto& input = source.as_series();
  const auto dtype = statistics_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Slate(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  auto values = input.to(dtype);
  auto valid = source.notnull().as_series().to(platform);
  auto prepared = values.where(valid.logical_not(), 0.0);
  auto counts = valid.to(dtype);

  const auto prefix_difference = [window, n, dtype, platform](
                                     const pluss::Series& input_values) {
    auto cumulative = input_values.cumsum(0);
    auto prior = pluss::Series::zeros(
        {std::min(window, n)}, dtype, platform);
    if (n > window) {
      prior = prior.unite(
          {cumulative.narrow_copy(0, 0, n - window)}, 0, true);
    }
    return cumulative - prior;
  };

  auto rolling_count = prefix_difference(counts);
  auto out_valid = rolling_count.ge(static_cast<double>(options.min_count));
  pluss::Series result = pluss::series::undefined();
  if (op == MovingOp_::Sum || op == MovingOp_::Mean ||
      op == MovingOp_::Variance || op == MovingOp_::Deviation) {
    auto sums = prefix_difference(prepared);
    if (op == MovingOp_::Sum) {
      result = sums;
    } else if (op == MovingOp_::Mean) {
      result = sums / rolling_count;
    } else {
      const auto total_count = counts.sum();
      const auto origin = prepared.sum() / total_count;
      const auto shifted =
          (values - origin).where(valid.logical_not(), 0.0);
      const auto shifted_sums = prefix_difference(shifted);
      auto square_sums = prefix_difference(shifted * shifted);
      auto variance =
          (square_sums -
           (shifted_sums * shifted_sums / rolling_count)) /
          (rolling_count - static_cast<double>(options.ddof));
      out_valid = out_valid.logical_and(
          rolling_count.gt(static_cast<double>(options.ddof)));
      result = op == MovingOp_::Deviation ? variance.sqrt() : variance;
    }
  } else {
    const auto fill = op == MovingOp_::Minimum
                          ? std::numeric_limits<double>::infinity()
                          : -std::numeric_limits<double>::infinity();
    auto padding = pluss::Series::full(
        {window - 1}, fill, dtype, platform);
    auto padded = padding.unite(
        {values.where(valid.logical_not(), fill)}, 0, true);
    auto windows = padded.unfold(0, window, 1);
    result = op == MovingOp_::Minimum ? windows.min(1) : windows.max(1);
  }

  if (options.centered && n > 0) {
    const auto offset = (window - 1) / 2;
    if (offset > 0) {
      result = Slate(result, out_valid).shift(-offset).as_series();
      out_valid = Slate(out_valid).shift(-offset).fillnull(0.0).as_series();
    }
  }
  return Slate(result, out_valid);
}

DataFrame moving_frame_(const detail::DataFrameImpl& src,
                        std::int64_t window,
                        const WindowOptions& options,
                        MovingOp_ op,
                        std::string_view where) {
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() == SlateKind::Series &&
        is_numeric_series_type(source.as_series())) {
      columns.emplace_back(
          src.names_[i], moving_statistic_(source, window, options, op, where));
    } else {
      columns.emplace_back(src.names_[i], source);
    }
  }
  return DataFrame(std::move(columns), src.cases_);
}

Slate moving_moment_(const Slate& source,
                      std::int64_t window,
                      const WindowOptions& options,
                      MovingMomentOp_ op,
                      std::string_view where) {
  PLUSS_CHECK(source.kind() == SlateKind::Series &&
                  is_numeric_series_type(source.as_series()),
              std::string(where) + ": expected a numeric Series feature");
  PLUSS_CHECK(window > 0, std::string(where) + ": window must be > 0");
  PLUSS_CHECK(options.ddof >= 0, std::string(where) + ": ddof must be >= 0");
  const auto required =
      op == MovingMomentOp_::Asymmetry
          ? std::int64_t{3}
          : op == MovingMomentOp_::TailWeight
                ? std::int64_t{4}
                : options.ddof + 1;
  const auto min_count = std::max(options.min_count, required);
  PLUSS_CHECK(min_count <= window,
              std::string(where) + ": window is smaller than the required observations");

  const auto n = source.size();
  const auto dtype = statistics_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  if (n == 0) {
    return Slate(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  const auto values = source.as_series().to(dtype);
  const auto valid = source.notnull().as_series().to(platform);
  const auto value_padding =
      pluss::Series::zeros({window - 1}, dtype, platform);
  const auto valid_padding =
      pluss::Series::zeros(
          {window - 1}, pluss::codem::ScalarType::Bool, platform);
  const auto window_values =
      value_padding
          .unite({values.where(valid.logical_not(), 0.0)}, 0, true)
          .unfold(0, window, 1);
  const auto window_valid =
      valid_padding.unite({valid}, 0, true).unfold(0, window, 1);
  const auto counts = window_valid.to(dtype).sum(1);
  const auto means = window_values.sum(1) / counts;
  const auto centered =
      (window_values - means.reshape({n, 1}))
          .where(window_valid.logical_not(), 0.0);
  const auto squares = centered * centered;
  pluss::Series result = pluss::series::undefined();
  if (op == MovingMomentOp_::MeanError) {
    result =
        (squares.sum(1) /
         (counts - static_cast<double>(options.ddof))).sqrt() /
        counts.sqrt();
  } else if (op == MovingMomentOp_::Asymmetry) {
    const auto m2 = squares.sum(1) / counts;
    const auto m3 = (squares * centered).sum(1) / counts;
    result = (m3 / (m2 * m2.sqrt())) *
             ((counts * (counts - 1.0)).sqrt() / (counts - 2.0));
  } else {
    const auto m2 = squares.sum(1) / counts;
    const auto m4 = (squares * squares).sum(1) / counts;
    const auto raw = m4 / (m2 * m2) - 3.0;
    result = ((counts - 1.0) /
              ((counts - 2.0) * (counts - 3.0))) *
             ((counts + 1.0) * raw + 6.0);
  }
  auto output_valid = counts.ge(static_cast<double>(min_count));
  if (options.centered && n > 0) {
    const auto offset = (window - 1) / 2;
    if (offset > 0) {
      result = Slate(result, output_valid).shift(-offset).as_series();
      output_valid =
          Slate(output_valid).shift(-offset).fillnull(0.0).as_series();
    }
  }
  return Slate(result, output_valid);
}

DataFrame moving_moment_frame_(const detail::DataFrameImpl& src,
                               std::int64_t window,
                               const WindowOptions& options,
                               MovingMomentOp_ op,
                               std::string_view where) {
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() == SlateKind::Series &&
        is_numeric_series_type(source.as_series())) {
      columns.emplace_back(
          src.names_[i], moving_moment_(source, window, options, op, where));
    } else {
      columns.emplace_back(src.names_[i], source);
    }
  }
  return DataFrame(std::move(columns), src.cases_);
}

} // namespace

DataFrame DataFrame::abs() const {
  return delegate_numeric_slates_(impl_ref_(), [](const Slate& source) {
    return source.abs();
  });
}

DataFrame DataFrame::clip(double min_value, double max_value) const {
  return delegate_numeric_slates_(impl_ref_(), [min_value, max_value](const Slate& source) {
    return source.clip(min_value, max_value);
  });
}

DataFrame DataFrame::round(std::int64_t decimals) const {
  return delegate_numeric_slates_(impl_ref_(), [decimals](const Slate& source) {
    return source.round(decimals);
  });
}

DataFrame DataFrame::log() const {
  return delegate_numeric_slates_(impl_ref_(), [](const Slate& source) {
    return source.log();
  });
}

DataFrame DataFrame::exp() const {
  return delegate_numeric_slates_(impl_ref_(), [](const Slate& source) {
    return source.exp();
  });
}

DataFrameView DataFrame::covar(const StatisticOptions& options, bool render) const {
  PLUSS_CHECK(options.ddof >= 0, "DataFrame::covar: ddof must be >= 0");
  PLUSS_CHECK(options.min_count >= 0, "DataFrame::covar: min_count must be >= 0");
  const auto& src = impl_ref_();
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::covar");
  const auto dtype = matrix.values.scalar_type();
  const auto valid_numeric = matrix.valid.to(dtype);
  const auto feature_counts = valid_numeric.sum(0);
  const auto origins =
      matrix.values.where(matrix.valid.logical_not(), 0.0).sum(0) /
      feature_counts;
  const auto values =
      (matrix.values - origins.reshape(
                           {1, static_cast<std::int64_t>(matrix.names.size())}))
          .where(matrix.valid.logical_not(), 0.0);
  const auto transposed_values = values.transpose(0, 1);
  const auto transposed_valid = valid_numeric.transpose(0, 1);
  const auto counts = transposed_valid.matmul(valid_numeric);
  const auto left_sums = transposed_values.matmul(valid_numeric);
  const auto cross = transposed_values.matmul(values);
  const auto centered = cross - (left_sums * left_sums.transpose(0, 1) / counts);
  const auto covariance = centered / (counts - static_cast<double>(options.ddof));
  const auto valid = counts.ge(static_cast<double>(
      std::max(options.min_count, options.ddof + 1)));

  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(matrix.names.size());
  for (std::size_t i = 0; i < matrix.names.size(); ++i) {
    columns.emplace_back(
        matrix.names[i],
        Slate(column_from_matrix_(covariance, static_cast<std::int64_t>(i)),
               column_from_matrix_(valid, static_cast<std::int64_t>(i))));
  }
  auto result = DataFrame(std::move(columns), make_note_slate(matrix.names));
  return DataFrameView(std::move(result), render, "covar", title());
}

DataFrame DataFrame::percentiles(pluss::msharp::RangeView::Double points,
                                 std::int64_t min_count) const {
  PLUSS_CHECK(!points.empty(), "DataFrame::percentiles: expected at least one point");
  PLUSS_CHECK(min_count >= 0, "DataFrame::percentiles: min_count must be >= 0");
  std::vector<std::string> labels;
  labels.reserve(points.size());
  for (const auto point : points) {
    PLUSS_CHECK(point >= 0.0 && point <= 1.0,
                "DataFrame::percentiles: points must be in [0, 1]");
    labels.push_back(fmt::format("{}%", point * 100.0));
  }

  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Slate>> columns;
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() != SlateKind::Series ||
        !is_numeric_series_type(source.as_series())) {
      continue;
    }
    columns.emplace_back(
        src.names_[i], source.percentiles(points, min_count));
  }
  PLUSS_CHECK(!columns.empty(), "DataFrame::percentiles: no numeric features");
  return DataFrame(std::move(columns), make_note_slate(labels));
}

DataFrame DataFrame::mode(bool drop_nulls) const {
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Slate>> modes;
  std::int64_t output_size = 0;
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if ((source.kind() == SlateKind::Series &&
         is_numeric_series_type(source.as_series())) ||
        source.kind() == SlateKind::Note ||
        source.kind() == SlateKind::Timeline ||
        source.kind() == SlateKind::Duration) {
      auto values = source.mode(drop_nulls);
      output_size = std::max(output_size, values.size());
      modes.emplace_back(src.names_[i], std::move(values));
    }
  }
  PLUSS_CHECK(!modes.empty(), "DataFrame::mode: no supported features");

  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(modes.size());
  for (auto& [name, values] : modes) {
    columns.emplace_back(name, pad_modes_(values, output_size));
  }
  std::vector<std::string> case_labels;
  case_labels.reserve(output_size);
  for (std::int64_t i = 0; i < output_size; ++i) {
    case_labels.push_back(fmt::format("mode_{}", i));
  }
  return DataFrame(std::move(columns), make_note_slate(case_labels));
}

DataFrame DataFrame::rank(RankTieRule ties,
                          RankNullRule nulls,
                          bool ascending,
                          bool percentage) const {
  return delegate_numeric_slates_(
      impl_ref_(), [&](const Slate& source) {
        return source.rank(ties, nulls, ascending, percentage);
      });
}

Slate DataFrame::skew(Axis axis, std::int64_t min_count) const {
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::skew");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_slates_(
        src,
        [min_count](const Slate& source) {
          return source.skew(min_count);
        },
        "DataFrame::skew");
  }
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::skew");
  auto counts = matrix.valid.to(matrix.values.scalar_type()).sum(normalized);
  auto values = matrix.values.where(matrix.valid.logical_not(), 0.0);
  auto means = values.sum(normalized) / counts;
  auto means_2d = normalized == 0
                      ? means.reshape({1, static_cast<std::int64_t>(matrix.names.size())})
                      : means.reshape({matrix.ncases, 1});
  auto centered = (matrix.values - means_2d).where(matrix.valid.logical_not(), 0.0);
  auto m2 = (centered * centered).sum(normalized) / counts;
  auto m3 = (centered * centered * centered).sum(normalized) / counts;
  auto skew = (m3 / (m2 * m2.sqrt())) *
              ((counts * (counts - 1.0)).sqrt() / (counts - 2.0));
  skew = skew.where(m2.eq(0.0), 0.0);
  auto valid = statistic_validity_(matrix, normalized, std::max<std::int64_t>(3, min_count));
  return labelled_statistic_(std::move(skew), std::move(valid), src, matrix, normalized);
}

Slate DataFrame::kurtosis(Axis axis, std::int64_t min_count) const {
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::kurtosis");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_slates_(
        src,
        [min_count](const Slate& source) {
          return source.kurtosis(min_count);
        },
        "DataFrame::kurtosis");
  }
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::kurtosis");
  auto counts = matrix.valid.to(matrix.values.scalar_type()).sum(normalized);
  auto values = matrix.values.where(matrix.valid.logical_not(), 0.0);
  auto means = values.sum(normalized) / counts;
  auto means_2d = normalized == 0
                      ? means.reshape({1, static_cast<std::int64_t>(matrix.names.size())})
                      : means.reshape({matrix.ncases, 1});
  auto centered = (matrix.values - means_2d).where(matrix.valid.logical_not(), 0.0);
  auto m2 = (centered * centered).sum(normalized) / counts;
  auto m4 = (centered * centered * centered * centered).sum(normalized) / counts;
  auto raw_excess = m4 / (m2 * m2) - 3.0;
  auto kurt = ((counts - 1.0) /
               ((counts - 2.0) * (counts - 3.0))) *
              ((counts + 1.0) * raw_excess + 6.0);
  kurt = kurt.where(m2.eq(0.0), 0.0);
  auto valid = statistic_validity_(matrix, normalized, std::max<std::int64_t>(4, min_count));
  return labelled_statistic_(std::move(kurt), std::move(valid), src, matrix, normalized);
}

Slate DataFrame::var(
    Axis axis,
    const StatisticOptions& options) const {
  const auto normalized =
      normalize_reduce_axis_(axis, "DataFrame::var");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "DataFrame::var: ddof and min_count must be >= 0");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_slates_(
        src,
        [&options](const Slate& source) {
          return source.var(options);
        },
        "DataFrame::var");
  }
  const auto matrix =
      numeric_reduce_matrix_(src, "DataFrame::var");
  const auto counts =
      matrix.valid.to(matrix.values.scalar_type()).sum(normalized);
  const auto values =
      matrix.values.where(matrix.valid.logical_not(), 0.0);
  const auto means = values.sum(normalized) / counts;
  const auto centered =
      (matrix.values - means.reshape({matrix.ncases, 1}))
          .where(matrix.valid.logical_not(), 0.0);
  const auto result =
      (centered * centered).sum(normalized) /
      (counts - static_cast<double>(options.ddof));
  const auto valid = statistic_validity_(
      matrix, normalized,
      std::max(options.min_count, options.ddof + 1));
  return labelled_statistic_(
      result.clamp_min(0.0), valid, src, matrix, normalized);
}

Slate DataFrame::std(
    Axis axis,
    const StatisticOptions& options) const {
  const auto result = var(axis, options);
  return Slate(
             result.as_series().clamp_min(0.0).sqrt(),
             result.validity_mask())
      .caselbl(detail::display_labels(result));
}

Slate DataFrame::se(Axis axis,
                     ErrorTarget target,
                     const StatisticOptions& options) const {
  PLUSS_CHECK(target == ErrorTarget::Mean,
              "DataFrame::se: only ErrorTarget::Mean is currently supported");
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::se");
  PLUSS_CHECK(options.ddof >= 0, "DataFrame::se: ddof must be >= 0");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_slates_(
        src,
        [&options](const Slate& source) {
          return source.se(ErrorTarget::Mean, options);
        },
        "DataFrame::se");
  }
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::se");
  auto counts = matrix.valid.to(matrix.values.scalar_type()).sum(normalized);
  auto values = matrix.values.where(matrix.valid.logical_not(), 0.0);
  auto means = values.sum(normalized) / counts;
  auto means_2d = normalized == 0
                      ? means.reshape({1, static_cast<std::int64_t>(matrix.names.size())})
                      : means.reshape({matrix.ncases, 1});
  auto centered = (matrix.values - means_2d).where(matrix.valid.logical_not(), 0.0);
  auto variance =
      (centered * centered).sum(normalized) /
      (counts - static_cast<double>(options.ddof));
  auto error = variance.sqrt() / counts.sqrt();
  auto valid = statistic_validity_(
      matrix, normalized, std::max(options.min_count, options.ddof + 1));
  return labelled_statistic_(std::move(error), std::move(valid), src, matrix, normalized);
}

DataFrame DataFrame::scan(ScanStatistic statistic, bool skip_nulls) const {
  return scan(statistic, Axis::Case, skip_nulls);
}

DataFrame DataFrame::scan(ScanStatistic statistic, Axis axis, bool skip_nulls) const {
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::scan");
  const auto op = running_op_from_scan_(statistic, "DataFrame::scan");
  if (normalized == 0) {
    return running_statistic_(impl_ref_(), op, skip_nulls, "DataFrame::scan");
  }
  return running_statistic_matrix_(impl_ref_(), op, normalized, skip_nulls, "DataFrame::scan");
}

DataFrame DataFrame::cumsum(bool skip_nulls) const {
  return scan(ScanStatistic::Sum, skip_nulls);
}

DataFrame DataFrame::cumsum(Axis axis, bool skip_nulls) const {
  return scan(ScanStatistic::Sum, axis, skip_nulls);
}

DataFrame DataFrame::cumprod(bool skip_nulls) const {
  return scan(ScanStatistic::Product, skip_nulls);
}

DataFrame DataFrame::cumprod(Axis axis, bool skip_nulls) const {
  return scan(ScanStatistic::Product, axis, skip_nulls);
}

DataFrame DataFrame::cummin(bool skip_nulls) const {
  return scan(ScanStatistic::Minimum, skip_nulls);
}

DataFrame DataFrame::cummin(Axis axis, bool skip_nulls) const {
  return scan(ScanStatistic::Minimum, axis, skip_nulls);
}

DataFrame DataFrame::cummax(bool skip_nulls) const {
  return scan(ScanStatistic::Maximum, skip_nulls);
}

DataFrame DataFrame::cummax(Axis axis, bool skip_nulls) const {
  return scan(ScanStatistic::Maximum, axis, skip_nulls);
}

Slate DataFrame::corr_with(const DataFrame& other,
                            CorrMethod method,
                            std::int64_t min_count) const {
  PLUSS_CHECK(min_count >= 1, "DataFrame::corr_with: min_count must be >= 1");
  const auto matched = match(
      other,
      FrameMatchOptions{
          .cases = FrameMatchRule::Common,
          .features = FrameMatchRule::Common,
          .repeated_cases = RepeatedCaseRule::PairInOrder});
  std::vector<std::string> names;
  std::vector<pluss::Series> values;
  std::vector<pluss::Series> valid;
  SlateCorrMethod slate_method = SlateCorrMethod::Pearson;
  if (method == CorrMethod::Spearman) {
    slate_method = SlateCorrMethod::Spearman;
  } else if (method == CorrMethod::KendallTau) {
    slate_method = SlateCorrMethod::KendallTau;
  }
  for (const auto& name : matched.left.features()) {
    const auto left = matched.left.features(name);
    const auto right = matched.right.features(name);
    if (left.kind() != SlateKind::Series || right.kind() != SlateKind::Series ||
        !is_numeric_series_type(left.as_series()) ||
        !is_numeric_series_type(right.as_series())) {
      continue;
    }
    names.push_back(name);
    const auto correlated =
        left.corr_with(right, slate_method, min_count);
    valid.push_back(correlated.slate().notnull().as_series());
    values.push_back(correlated.as_series());
  }
  PLUSS_CHECK(!names.empty(), "DataFrame::corr_with: no common numeric features");
  auto output = pluss::series::pile(values, 0).reshape(
      {static_cast<std::int64_t>(values.size())});
  auto output_valid = pluss::series::pile(valid, 0).reshape(
      {static_cast<std::int64_t>(valid.size())});
  return Slate(output, output_valid).caselbl(make_note_slate(names));
}

DataFrame DataFrame::corr_with(pluss::msharp::RangeView::Of<DataFrame> others,
                               CorrMethod method,
                               std::int64_t min_count) const {
  PLUSS_CHECK(others.size() > 0, "DataFrame::corr_with: expected at least one comparison frame");
  std::vector<std::pair<std::string, Slate>> columns;
  columns.reserve(others.size());
  for (std::size_t i = 0; i < others.size(); ++i) {
    columns.emplace_back(fmt::format("frame_{}", i), corr_with(others[i], method, min_count));
  }
  return DataFrame(std::move(columns));
}

DataFrame DataFrame::matmul(const DataFrame& other) const {
  const auto& left = impl_ref_();
  const auto& right = other.impl_ref_();
  const auto left_matrix = numeric_reduce_matrix_(left, "DataFrame::matmul(left)");
  const auto right_matrix = numeric_reduce_matrix_(right, "DataFrame::matmul(right)");
  PLUSS_CHECK(left_matrix.values.sizes()[1] == right_matrix.values.sizes()[0],
              "DataFrame::matmul: left feature count must equal right case count");
  PLUSS_CHECK(left_matrix.valid.all().item<bool>() &&
                  right_matrix.valid.all().item<bool>(),
              "DataFrame::matmul: null values are not supported");
  auto result = left_matrix.values.matmul(right_matrix.values);
  auto valid = pluss::Series::ones(
      result.sizes(), pluss::codem::ScalarType::Bool, result.platform());
  return frame_from_matrix_(
      result, valid, right_matrix.names, left.cases_);
}
