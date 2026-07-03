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
                             const Thread& cases) {
  PLUSS_CHECK(matrix.dim() == 2, "frame_from_matrix: expected a two-dimensional value matrix");
  PLUSS_CHECK(matrix.sizes()[1] == static_cast<std::int64_t>(names.size()),
              "frame_from_matrix: feature count mismatch");
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(names.size());
  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto value_column = column_from_matrix_(matrix, static_cast<std::int64_t>(i));
    const auto valid_column =
        valid.dim() == 1 ? valid : column_from_matrix_(valid, static_cast<std::int64_t>(i));
    columns.emplace_back(names[i], Thread(value_column, valid_column));
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

Thread labelled_statistic_(pluss::Series values,
                           pluss::Series valid,
                           const detail::DataFrameImpl& src,
                           const ReduceMatrix_& matrix,
                           std::int64_t axis) {
  return with_reduce_labels_(
      Thread(std::move(values), std::move(valid)), src, matrix.names, axis);
}

template <class Operation>
DataFrame delegate_numeric_threads_(const detail::DataFrameImpl& src,
                                    Operation&& operation) {
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    columns.emplace_back(
        src.names_[i],
        source.kind() == ThreadKind::Series &&
                is_numeric_series_type(source.as_series())
            ? operation(source)
            : source);
  }
  return DataFrame(std::move(columns), src.cases_);
}

template <class Operation>
Thread reduce_numeric_threads_(const detail::DataFrameImpl& src,
                               Operation&& operation,
                               std::string_view where) {
  std::vector<std::string> names;
  std::vector<pluss::Series> values;
  std::vector<pluss::Series> validity;
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() != ThreadKind::Series ||
        !is_numeric_series_type(source.as_series())) {
      continue;
    }
    auto reduced = operation(source);
    PLUSS_CHECK(reduced.kind() == ThreadKind::Series && reduced.size() == 1,
                std::string(where) +
                    ": Thread reduction must return one numeric value");
    names.push_back(src.names_[i]);
    values.push_back(reduced.as_series());
    validity.push_back(reduced.notnull().as_series());
  }
  PLUSS_CHECK(!values.empty(), std::string(where) + ": no numeric features");
  return Thread(
             pluss::pile(values, 0).reshape(
                 {static_cast<std::int64_t>(values.size())}),
             pluss::pile(validity, 0).reshape(
                 {static_cast<std::int64_t>(validity.size())}))
      .with_display_labels(make_note_thread(names));
}

Thread pad_modes_(const Thread& source, std::int64_t size) {
  PLUSS_CHECK(source.size() <= size, "pad_modes: invalid output size");
  if (source.size() == size) {
    return source;
  }
  const auto padding = size - source.size();
  auto valid_padding = pluss::Series::zeros(
      {padding}, pluss::codem::ScalarType::Bool);
  if (source.kind() == ThreadKind::Series) {
    const auto platform = source.as_series().platform();
    auto values = pluss::Series::zeros(
        {padding}, source.as_series().scalar_type(), platform);
    if (platform != pluss::codem::Platform::CPU) {
      valid_padding = valid_padding.to(platform);
    }
    return Thread(
        source.as_series().unite({values}, 0, true),
        source.notnull().as_series().unite({valid_padding}, 0, true));
  }
  if (source.kind() == ThreadKind::Timeline ||
      source.kind() == ThreadKind::TimeIndex) {
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
    const auto platform =
        output.kind() == ThreadKind::Timeline
            ? output.as_timeline().epoch_nanos().platform()
            : output.as_timeindex().timeline().epoch_nanos().platform();
    if (output_valid.platform() != platform) {
      output_valid = output_valid.to(platform);
    }
    return output.with_validity_mask(output_valid);
  }
  std::vector<std::string> padded;
  padded.reserve(static_cast<std::size_t>(size));
  for (const auto value : source.as_note().vector()) {
    padded.emplace_back(value);
  }
  padded.resize(static_cast<std::size_t>(size));
  return Thread(
      pluss::make_note(padded),
      source.notnull().as_series().unite({valid_padding}, 0, true));
}

enum class RunningOp_ : std::uint8_t { Sum, Product, Minimum, Maximum };

DataFrame running_statistic_(const detail::DataFrameImpl& src,
                             RunningOp_ op,
                             bool skip_nulls,
                             std::string_view where) {
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() != ThreadKind::Series ||
        !is_numeric_series_type(source.as_series())) {
      columns.emplace_back(src.names_[i], source);
      continue;
    }
    auto values = source.as_series().to(statistics_compute_type_(source.as_series()));
    auto valid = source.notnull().as_series().to(values.platform());
    const auto invalid = valid.logical_not();
    pluss::Series result;
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
    columns.emplace_back(src.names_[i], Thread(result, out_valid));
  }
  PLUSS_CHECK(!columns.empty(), std::string(where) + ": no features");
  return DataFrame(std::move(columns), src.cases_);
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

Thread moving_statistic_(const Thread& source,
                         std::int64_t window,
                         const WindowOptions& options,
                         MovingOp_ op,
                         std::string_view where) {
  PLUSS_CHECK(source.kind() == ThreadKind::Series,
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
    return Thread(
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
  pluss::Series result;
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
      result = Thread(result, out_valid).shift(-offset).as_series();
      out_valid = Thread(out_valid).shift(-offset).fillna(0.0).as_series();
    }
  }
  return Thread(result, out_valid);
}

DataFrame moving_frame_(const detail::DataFrameImpl& src,
                        std::int64_t window,
                        const WindowOptions& options,
                        MovingOp_ op,
                        std::string_view where) {
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() == ThreadKind::Series &&
        is_numeric_series_type(source.as_series())) {
      columns.emplace_back(
          src.names_[i], moving_statistic_(source, window, options, op, where));
    } else {
      columns.emplace_back(src.names_[i], source);
    }
  }
  return DataFrame(std::move(columns), src.cases_);
}

Thread moving_moment_(const Thread& source,
                      std::int64_t window,
                      const WindowOptions& options,
                      MovingMomentOp_ op,
                      std::string_view where) {
  PLUSS_CHECK(source.kind() == ThreadKind::Series &&
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
    return Thread(
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
  pluss::Series result;
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
      result = Thread(result, output_valid).shift(-offset).as_series();
      output_valid =
          Thread(output_valid).shift(-offset).fillna(0.0).as_series();
    }
  }
  return Thread(result, output_valid);
}

DataFrame moving_moment_frame_(const detail::DataFrameImpl& src,
                               std::int64_t window,
                               const WindowOptions& options,
                               MovingMomentOp_ op,
                               std::string_view where) {
  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() == ThreadKind::Series &&
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

DataFrameView DataFrame::covariance(const StatisticOptions& options, bool render) const {
  PLUSS_CHECK(options.ddof >= 0, "DataFrame::covariance: ddof must be >= 0");
  PLUSS_CHECK(options.min_count >= 0, "DataFrame::covariance: min_count must be >= 0");
  const auto& src = impl_ref_();
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::covariance");
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

  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(matrix.names.size());
  for (std::size_t i = 0; i < matrix.names.size(); ++i) {
    columns.emplace_back(
        matrix.names[i],
        Thread(column_from_matrix_(covariance, static_cast<std::int64_t>(i)),
               column_from_matrix_(valid, static_cast<std::int64_t>(i))));
  }
  auto result = DataFrame(std::move(columns), make_note_thread(matrix.names));
  return DataFrameView(std::move(result), render, "covariance", title());
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
  std::vector<std::pair<std::string, Thread>> columns;
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if (source.kind() != ThreadKind::Series ||
        !is_numeric_series_type(source.as_series())) {
      continue;
    }
    columns.emplace_back(
        src.names_[i], source.percentiles(points, min_count));
  }
  PLUSS_CHECK(!columns.empty(), "DataFrame::percentiles: no numeric features");
  return DataFrame(std::move(columns), make_note_thread(labels));
}

DataFrame DataFrame::most_frequent(bool drop_nulls) const {
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, Thread>> modes;
  std::int64_t output_size = 0;
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    if ((source.kind() == ThreadKind::Series &&
         is_numeric_series_type(source.as_series())) ||
        source.kind() == ThreadKind::Note ||
        source.kind() == ThreadKind::Timeline ||
        source.kind() == ThreadKind::TimeIndex) {
      auto values = source.most_frequent(drop_nulls);
      output_size = std::max(output_size, values.size());
      modes.emplace_back(src.names_[i], std::move(values));
    }
  }
  PLUSS_CHECK(!modes.empty(), "DataFrame::most_frequent: no supported features");

  std::vector<std::pair<std::string, Thread>> columns;
  columns.reserve(modes.size());
  for (auto& [name, values] : modes) {
    columns.emplace_back(name, pad_modes_(values, output_size));
  }
  std::vector<std::string> case_labels;
  case_labels.reserve(output_size);
  for (std::int64_t i = 0; i < output_size; ++i) {
    case_labels.push_back(fmt::format("mode_{}", i));
  }
  return DataFrame(std::move(columns), make_note_thread(case_labels));
}

DataFrame DataFrame::ranks(RankTieRule ties,
                           RankNullRule nulls,
                           bool ascending,
                           bool percentage) const {
  return delegate_numeric_threads_(
      impl_ref_(), [&](const Thread& source) {
        return source.ranks(ties, nulls, ascending, percentage);
      });
}

Thread DataFrame::asymmetry(std::int64_t axis, std::int64_t min_count) const {
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::asymmetry");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_threads_(
        src,
        [min_count](const Thread& source) {
          return source.asymmetry(min_count);
        },
        "DataFrame::asymmetry");
  }
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::asymmetry");
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

Thread DataFrame::tail_weight(std::int64_t axis, std::int64_t min_count) const {
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::tail_weight");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_threads_(
        src,
        [min_count](const Thread& source) {
          return source.tail_weight(min_count);
        },
        "DataFrame::tail_weight");
  }
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::tail_weight");
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

Thread DataFrame::variance(
    std::int64_t axis,
    const StatisticOptions& options) const {
  const auto normalized =
      normalize_reduce_axis_(axis, "DataFrame::variance");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "DataFrame::variance: ddof and min_count must be >= 0");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_threads_(
        src,
        [&options](const Thread& source) {
          return source.variance(options);
        },
        "DataFrame::variance");
  }
  const auto matrix =
      numeric_reduce_matrix_(src, "DataFrame::variance");
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

Thread DataFrame::deviation(
    std::int64_t axis,
    const StatisticOptions& options) const {
  const auto result = variance(axis, options);
  return Thread(
             result.as_series().clamp_min(0.0).sqrt(),
             result.validity_mask())
      .with_display_labels(result.display_labels());
}

Thread DataFrame::mean_error(std::int64_t axis,
                             const StatisticOptions& options) const {
  const auto normalized = normalize_reduce_axis_(axis, "DataFrame::mean_error");
  PLUSS_CHECK(options.ddof >= 0, "DataFrame::mean_error: ddof must be >= 0");
  const auto& src = impl_ref_();
  if (normalized == 0) {
    return reduce_numeric_threads_(
        src,
        [&options](const Thread& source) {
          return source.mean_error(options);
        },
        "DataFrame::mean_error");
  }
  const auto matrix = numeric_reduce_matrix_(src, "DataFrame::mean_error");
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

DataFrame DataFrame::running_sum(bool skip_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [skip_nulls](const Thread& source) {
        return source.running_sum(skip_nulls);
      });
}

DataFrame DataFrame::running_product(bool skip_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [skip_nulls](const Thread& source) {
        return source.running_product(skip_nulls);
      });
}

DataFrame DataFrame::running_minimum(bool skip_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [skip_nulls](const Thread& source) {
        return source.running_minimum(skip_nulls);
      });
}

DataFrame DataFrame::running_maximum(bool skip_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [skip_nulls](const Thread& source) {
        return source.running_maximum(skip_nulls);
      });
}

Thread DataFrame::correlate_with(const DataFrame& other,
                                 CorrMethod method,
                                 std::int64_t min_count) const {
  PLUSS_CHECK(min_count >= 1, "DataFrame::correlate_with: min_count must be >= 1");
  const auto matched = matched_with(
      other,
      FrameMatchOptions{
          .cases = FrameMatchRule::Common,
          .features = FrameMatchRule::Common,
          .repeated_cases = RepeatedCaseRule::PairInOrder});
  std::vector<std::string> names;
  std::vector<pluss::Series> values;
  std::vector<pluss::Series> valid;
  ThreadCorrMethod thread_method = ThreadCorrMethod::Pearson;
  if (method == CorrMethod::Spearman) {
    thread_method = ThreadCorrMethod::Spearman;
  } else if (method == CorrMethod::KendallTau) {
    thread_method = ThreadCorrMethod::KendallTau;
  }
  for (const auto& name : matched.left.feature_names()) {
    const auto left = matched.left.features(name);
    const auto right = matched.right.features(name);
    if (left.kind() != ThreadKind::Series || right.kind() != ThreadKind::Series ||
        !is_numeric_series_type(left.as_series()) ||
        !is_numeric_series_type(right.as_series())) {
      continue;
    }
    names.push_back(name);
    const auto correlated =
        left.correlate_with(right, thread_method, min_count);
    valid.push_back(correlated.notnull().as_series());
    values.push_back(correlated.as_series());
  }
  PLUSS_CHECK(!names.empty(), "DataFrame::correlate_with: no common numeric features");
  auto output = pluss::pile(values, 0).reshape(
      {static_cast<std::int64_t>(values.size())});
  auto output_valid = pluss::pile(valid, 0).reshape(
      {static_cast<std::int64_t>(valid.size())});
  return Thread(output, output_valid).with_display_labels(make_note_thread(names));
}

DataFrame DataFrame::matrix_product(const DataFrame& other) const {
  const auto& left = impl_ref_();
  const auto& right = other.impl_ref_();
  const auto left_matrix = numeric_reduce_matrix_(left, "DataFrame::matrix_product(left)");
  const auto right_matrix = numeric_reduce_matrix_(right, "DataFrame::matrix_product(right)");
  PLUSS_CHECK(left_matrix.values.sizes()[1] == right_matrix.values.sizes()[0],
              "DataFrame::matrix_product: left feature count must equal right case count");
  PLUSS_CHECK(left_matrix.valid.all().item<bool>() &&
                  right_matrix.valid.all().item<bool>(),
              "DataFrame::matrix_product: null values are not supported");
  auto result = left_matrix.values.matmul(right_matrix.values);
  auto valid = pluss::Series::ones(
      result.sizes(), pluss::codem::ScalarType::Bool, result.platform());
  return frame_from_matrix_(
      result, valid, right_matrix.names, left.cases_);
}

DataFrame DataFrame::moving_sum(std::int64_t window,
                                const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving(window, WindowStatistic::Sum, options);
      });
}

DataFrame DataFrame::moving_mean(std::int64_t window,
                                 const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving(window, WindowStatistic::Mean, options);
      });
}

DataFrame DataFrame::moving_minimum(std::int64_t window,
                                    const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving(window, WindowStatistic::Minimum, options);
      });
}

DataFrame DataFrame::moving_maximum(std::int64_t window,
                                    const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving(window, WindowStatistic::Maximum, options);
      });
}

DataFrame DataFrame::moving_variance(std::int64_t window,
                                     const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving(window, WindowStatistic::Variance, options);
      });
}

DataFrame DataFrame::moving_deviation(std::int64_t window,
                                      const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving(window, WindowStatistic::Deviation, options);
      });
}

DataFrame DataFrame::moving_mean_error(std::int64_t window,
                                       const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving_mean_error(window, options);
      });
}

DataFrame DataFrame::moving_asymmetry(std::int64_t window,
                                      const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving_asymmetry(window, options);
      });
}

DataFrame DataFrame::moving_tail_weight(std::int64_t window,
                                        const WindowOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [window, &options](const Thread& source) {
        return source.moving_tail_weight(window, options);
      });
}

DataFrame DataFrame::weighted_moving(
    pluss::msharp::RangeView::Double weights,
    WindowStatistic statistic,
    std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [=](const Thread& source) {
        return source.weighted_moving(
            weights, statistic, min_count);
      });
}

DataFrame DataFrame::indexed_moving(
    const WindowBoundsFn& bounds,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  PLUSS_CHECK(static_cast<bool>(bounds),
              "DataFrame::indexed_moving: bounds callback is empty");
  return delegate_numeric_threads_(
      impl_ref_(), [&](const Thread& source) {
        return source.indexed_moving(bounds, statistic, options);
      });
}

DataFrame DataFrame::growing_sum(std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [min_count](const Thread& source) {
        return source.growing(
            WindowStatistic::Sum,
            StatisticOptions{.ddof = 1, .min_count = min_count});
      });
}

DataFrame DataFrame::growing_mean(std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [min_count](const Thread& source) {
        return source.growing(
            WindowStatistic::Mean,
            StatisticOptions{.ddof = 1, .min_count = min_count});
      });
}

DataFrame DataFrame::growing_minimum(std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [min_count](const Thread& source) {
        return source.growing(
            WindowStatistic::Minimum,
            StatisticOptions{.ddof = 1, .min_count = min_count});
      });
}

DataFrame DataFrame::growing_maximum(std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [min_count](const Thread& source) {
        return source.growing(
            WindowStatistic::Maximum,
            StatisticOptions{.ddof = 1, .min_count = min_count});
      });
}

DataFrame DataFrame::growing_variance(const StatisticOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [&options](const Thread& source) {
        return source.growing(WindowStatistic::Variance, options);
      });
}

DataFrame DataFrame::growing_deviation(const StatisticOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [&options](const Thread& source) {
        return source.growing(WindowStatistic::Deviation, options);
      });
}

DataFrame DataFrame::growing_mean_error(const StatisticOptions& options) const {
  return delegate_numeric_threads_(
      impl_ref_(), [&options](const Thread& source) {
        return source.growing_mean_error(options);
      });
}

DataFrame DataFrame::growing_asymmetry(std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [min_count](const Thread& source) {
        return source.growing_asymmetry(min_count);
      });
}

DataFrame DataFrame::growing_tail_weight(std::int64_t min_count) const {
  return delegate_numeric_threads_(
      impl_ref_(), [min_count](const Thread& source) {
        return source.growing_tail_weight(min_count);
      });
}

DataFrame DataFrame::exponential_mean(double alpha,
                                      bool adjusted,
                                      std::int64_t min_count,
                                      bool ignore_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [=](const Thread& source) {
        return source.exponential_mean(
            alpha, adjusted, min_count, ignore_nulls);
      });
}

DataFrame DataFrame::exponential_variance(double alpha,
                                          bool adjusted,
                                          bool bias_corrected,
                                          std::int64_t min_count,
                                          bool ignore_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [=](const Thread& source) {
        return source.exponential_variance(
            alpha, adjusted, bias_corrected, min_count, ignore_nulls);
      });
}

DataFrame DataFrame::exponential_deviation(double alpha,
                                           bool adjusted,
                                           bool bias_corrected,
                                           std::int64_t min_count,
                                           bool ignore_nulls) const {
  return delegate_numeric_threads_(
      impl_ref_(), [=](const Thread& source) {
        return source.exponential_deviation(
            alpha, adjusted, bias_corrected, min_count, ignore_nulls);
  });
}

DataFrame DataFrame::grouped_moving(std::string_view group_feature,
                                    std::int64_t window,
                                    WindowStatistic statistic,
                                    const WindowOptions& options) const {
  const auto& src = impl_ref_();
  const auto group_name = owned_name(group_feature);
  const auto group_it = src.name_to_idx_.find(group_name);
  PLUSS_CHECK(group_it != src.name_to_idx_.end(),
              "DataFrame::grouped_moving: unknown group feature '" +
                  group_name + "'");
  const auto& groups = src.columns_[group_it->second];
  std::vector<std::pair<std::string, Thread>> delegated;
  delegated.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    delegated.emplace_back(
        src.names_[i],
        i != group_it->second &&
                source.kind() == ThreadKind::Series &&
                is_numeric_series_type(source.as_series())
            ? source.grouped_moving(
                  groups, window, statistic, options)
            : source);
  }
  return DataFrame(std::move(delegated), src.cases_);
}

DataFrame DataFrame::grouped_moving_mean(std::string_view group_feature,
                                         std::int64_t window,
                                         const WindowOptions& options) const {
  return grouped_moving(
      group_feature, window, WindowStatistic::Mean, options);
}

DataFrame DataFrame::time_moving(std::string_view time_feature,
                                 std::int64_t duration_nanos,
                                 TimeWindowStatistic statistic,
                                 const StatisticOptions& options,
                                 WindowClosure closure) const {
  PLUSS_CHECK(duration_nanos > 0,
              "DataFrame::time_moving: duration_nanos must be > 0");
  PLUSS_CHECK(options.min_count > 0,
              "DataFrame::time_moving: min_count must be > 0");
  PLUSS_CHECK(options.ddof >= 0,
              "DataFrame::time_moving: ddof must be >= 0");
  const auto& src = impl_ref_();
  const auto time_name = owned_name(time_feature);
  const auto time_it = src.name_to_idx_.find(time_name);
  PLUSS_CHECK(time_it != src.name_to_idx_.end(),
              "DataFrame::time_moving: unknown time feature '" +
                  time_name + "'");
  const auto& axis = src.columns_[time_it->second];
  std::vector<std::pair<std::string, Thread>> delegated;
  delegated.reserve(src.names_.size());
  for (std::size_t i = 0; i < src.names_.size(); ++i) {
    const auto& source = src.columns_[i];
    delegated.emplace_back(
        src.names_[i],
        i != time_it->second &&
                source.kind() == ThreadKind::Series &&
                is_numeric_series_type(source.as_series())
            ? source.time_moving(
                  axis, duration_nanos, statistic, options, closure)
            : source);
  }
  return DataFrame(std::move(delegated), src.cases_);
}

DataFrame DataFrame::time_moving_mean(std::string_view time_feature,
                                      std::int64_t duration_nanos,
                                      std::int64_t min_count) const {
  return time_moving(
      time_feature,
      duration_nanos,
      TimeWindowStatistic::Mean,
      StatisticOptions{.ddof = 1, .min_count = min_count});
}
