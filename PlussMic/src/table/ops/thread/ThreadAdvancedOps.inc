namespace {

pluss::codem::ScalarType thread_compute_type_(const pluss::Series& values) {
  return values.platform() == pluss::codem::Platform::CPU
             ? pluss::codem::ScalarType::Float64
             : pluss::codem::ScalarType::Float32;
}

void require_numeric_thread_(const Thread& value, std::string_view where) {
  PLUSS_CHECK(
      value.kind() == ThreadKind::Series &&
          is_numeric_series_type(value.as_series()),
      std::string(where) + ": expected a real numeric Series Thread");
}

pluss::Series thread_valid_(const Thread& value) {
  auto valid = value.notnull().as_series();
  if (value.kind() == ThreadKind::Series &&
      valid.platform() != value.as_series().platform()) {
    valid = valid.to(value.as_series().platform());
  }
  return valid;
}

Thread scalar_thread_(pluss::Series value, bool valid) {
  auto mask = valid
                  ? pluss::Series::ones(
                        value.sizes(), pluss::codem::ScalarType::Bool,
                        value.platform())
                  : pluss::Series::zeros(
                        value.sizes(), pluss::codem::ScalarType::Bool,
                        value.platform());
  return Thread(std::move(value), std::move(mask));
}

pluss::Series index_series_(
    const std::vector<std::int64_t>& values,
    pluss::codem::Platform platform) {
  auto out = pluss::Series::empty(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Int64,
      pluss::codem::Platform::CPU);
  std::copy(values.begin(), values.end(),
            reinterpret_cast<std::int64_t*>(out.raw_data()));
  return platform == pluss::codem::Platform::CPU ? out : out.to(platform);
}

Thread running_thread_(const Thread& source,
                       bool skip_nulls,
                       int operation,
                       std::string_view where) {
  require_numeric_thread_(source, where);
  const auto& input = source.as_series();
  const auto dtype = thread_compute_type_(input);
  auto values = input.to(dtype);
  auto valid = thread_valid_(source);
  const auto invalid = valid.logical_not();
  pluss::Series result;
  switch (operation) {
    case 0:
      result = values.where(invalid, 0.0).cumsum(0);
      break;
    case 1:
      result = values.where(invalid, 1.0).cumprod(0);
      break;
    case 2:
      result = values.where(
          invalid, std::numeric_limits<double>::infinity()).cummin(0);
      break;
    case 3:
      result = values.where(
          invalid, -std::numeric_limits<double>::infinity()).cummax(0);
      break;
    default:
      PLUSS_FAIL(std::string(where) + ": unsupported cumulative operation");
  }
  auto out_valid =
      skip_nulls
          ? valid
          : valid.to(pluss::codem::ScalarType::Int64)
                .cummin(0)
                .to(pluss::codem::ScalarType::Bool);
  return Thread(result, out_valid);
}

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>
fixed_bounds_(std::int64_t size,
              std::int64_t window,
              const WindowOptions& options) {
  PLUSS_CHECK(window > 0, "Thread::moving: window must be > 0");
  std::vector<std::int64_t> starts(static_cast<std::size_t>(size));
  std::vector<std::int64_t> ends(static_cast<std::size_t>(size));
  const auto offset = options.centered ? (window - 1) / 2 : 0;
  for (std::int64_t row = 0; row < size; ++row) {
    auto end = row + 1 + offset;
    auto start = end - window;
    if (options.closure == WindowClosure::Left ||
        options.closure == WindowClosure::Both) {
      --start;
    }
    if (options.closure == WindowClosure::Left ||
        options.closure == WindowClosure::Neither) {
      --end;
    }
    starts[static_cast<std::size_t>(row)] =
        std::clamp<std::int64_t>(start, 0, size);
    ends[static_cast<std::size_t>(row)] =
        std::clamp<std::int64_t>(end, 0, size);
  }
  return {std::move(starts), std::move(ends)};
}

Thread indexed_numeric_window_(
    const Thread& source,
    const std::vector<std::int64_t>& starts,
    const std::vector<std::int64_t>& ends,
    WindowStatistic statistic,
    const WindowOptions& options,
    std::string_view where) {
  require_numeric_thread_(source, where);
  PLUSS_CHECK(
      starts.size() == static_cast<std::size_t>(source.size()) &&
          ends.size() == static_cast<std::size_t>(source.size()),
      std::string(where) + ": bounds must match Thread size");
  PLUSS_CHECK(options.min_count > 0,
              std::string(where) + ": min_count must be > 0");
  PLUSS_CHECK(options.ddof >= 0,
              std::string(where) + ": ddof must be >= 0");
  for (std::size_t i = 0; i < starts.size(); ++i) {
    PLUSS_CHECK(
        starts[i] >= 0 && starts[i] <= ends[i] &&
            ends[i] <= source.size(),
        std::string(where) + ": invalid window bounds");
  }

  const auto n = source.size();
  const auto& input = source.as_series();
  const auto dtype = thread_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Thread(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  const auto values = input.to(dtype);
  const auto valid = thread_valid_(source);
  const auto prepared = values.where(valid.logical_not(), 0.0);
  const auto zero = pluss::Series::zeros({1}, dtype, platform);
  const auto value_prefix =
      zero.unite({prepared.cumsum(0)}, 0, true);
  const auto count_prefix =
      zero.unite({valid.to(dtype).cumsum(0)}, 0, true);
  const auto start_index = index_series_(starts, platform);
  const auto end_index = index_series_(ends, platform);
  const auto sums =
      value_prefix.index_select(0, end_index) -
      value_prefix.index_select(0, start_index);
  const auto counts =
      count_prefix.index_select(0, end_index) -
      count_prefix.index_select(0, start_index);
  auto output_valid =
      counts.ge(static_cast<double>(options.min_count));
  pluss::Series result;

  if (statistic == WindowStatistic::Sum) {
    result = sums;
  } else if (statistic == WindowStatistic::Mean) {
    result = sums / counts;
  } else if (statistic == WindowStatistic::Var ||
             statistic == WindowStatistic::Std) {
    const auto origin = prepared.sum() / valid.to(dtype).sum();
    const auto shifted =
        (values - origin).where(valid.logical_not(), 0.0);
    const auto shifted_prefix =
        zero.unite({shifted.cumsum(0)}, 0, true);
    const auto square_prefix =
        zero.unite({(shifted * shifted).cumsum(0)}, 0, true);
    const auto shifted_sums =
        shifted_prefix.index_select(0, end_index) -
        shifted_prefix.index_select(0, start_index);
    const auto square_sums =
        square_prefix.index_select(0, end_index) -
        square_prefix.index_select(0, start_index);
    auto variance =
        (square_sums - shifted_sums * shifted_sums / counts) /
        (counts - static_cast<double>(options.ddof));
    output_valid = output_valid.logical_and(
        counts.gt(static_cast<double>(options.ddof)));
    variance = variance.clamp_min(0.0);
    result = statistic == WindowStatistic::Std
                 ? variance.sqrt()
                 : variance;
  } else {
    // Arbitrary bounds do not form a regular strided matrix. A monotone
    // deque cannot serve user-defined overlapping intervals, so extrema use
    // one inevitable bounds pass and return to the source platform.
    const auto cpu_values =
        values.to(pluss::codem::Platform::CPU)
            .to(pluss::codem::ScalarType::Float64).to_vec<double>();
    const auto cpu_valid =
        valid.to(pluss::codem::Platform::CPU).to_vec<bool>();
    std::vector<double> extrema(static_cast<std::size_t>(n), 0.0);
    std::vector<bool> extrema_valid(static_cast<std::size_t>(n), false);
    for (std::int64_t row = 0; row < n; ++row) {
      double best = statistic == WindowStatistic::Minimum
                        ? std::numeric_limits<double>::infinity()
                        : -std::numeric_limits<double>::infinity();
      std::int64_t observed = 0;
      for (auto pos = starts[static_cast<std::size_t>(row)];
           pos < ends[static_cast<std::size_t>(row)]; ++pos) {
        if (!cpu_valid[static_cast<std::size_t>(pos)]) {
          continue;
        }
        ++observed;
        best = statistic == WindowStatistic::Minimum
                   ? std::min(best, cpu_values[static_cast<std::size_t>(pos)])
                   : std::max(best, cpu_values[static_cast<std::size_t>(pos)]);
      }
      if (observed >= options.min_count) {
        extrema[static_cast<std::size_t>(row)] = best;
        extrema_valid[static_cast<std::size_t>(row)] = true;
      }
    }
    auto cpu_result = pluss::Series::empty(
        {n}, pluss::codem::ScalarType::Float64);
    std::copy(extrema.begin(), extrema.end(),
              reinterpret_cast<double*>(cpu_result.raw_data()));
    auto cpu_mask = bool_series_from_bools(extrema_valid);
    if (platform != pluss::codem::Platform::CPU) {
      cpu_result =
          cpu_result.to(pluss::codem::ScalarType::Float32).to(platform);
      cpu_mask = cpu_mask.to(platform);
    }
    return Thread(cpu_result, cpu_mask);
  }
  return Thread(result, output_valid);
}

Thread moving_moment_thread_(
    const Thread& source,
    std::int64_t window,
    const WindowOptions& options,
    int operation,
    std::string_view where) {
  require_numeric_thread_(source, where);
  const auto required =
      operation == 1 ? std::int64_t{3}
                     : operation == 2 ? std::int64_t{4}
                                      : options.ddof + 1;
  const auto minimum = std::max(options.min_count, required);
  PLUSS_CHECK(window >= minimum,
              std::string(where) +
                  ": window is smaller than required observations");

  const auto n = source.size();
  const auto& input = source.as_series();
  const auto dtype = thread_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Thread(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  if (options.closure != WindowClosure::Right) {
    const auto [starts, ends] = fixed_bounds_(n, window, options);
    const auto values = input.to(dtype);
    const auto valid = thread_valid_(source);
    const auto numeric_valid = valid.to(dtype);
    const auto count_total = numeric_valid.sum();
    const auto origin =
        values.where(valid.logical_not(), 0.0).sum() / count_total;
    const auto shifted =
        (values - origin).where(valid.logical_not(), 0.0);
    const auto zero = pluss::Series::zeros({1}, dtype, platform);
    const auto prefix = [&zero](const pluss::Series& value) {
      return zero.unite({value.cumsum(0)}, 0, true);
    };
    const auto p1 = prefix(shifted);
    const auto square = shifted * shifted;
    const auto p2 = prefix(square);
    const auto p3 = prefix(square * shifted);
    const auto p4 = prefix(square * square);
    const auto pc = prefix(numeric_valid);
    const auto begin = index_series_(starts, platform);
    const auto end = index_series_(ends, platform);
    const auto window_sum = [&](const pluss::Series& value) {
      return value.index_select(0, end) -
             value.index_select(0, begin);
    };
    const auto counts = window_sum(pc);
    const auto s1 = window_sum(p1);
    const auto s2 = window_sum(p2);
    const auto mean = s1 / counts;
    const auto m2 = s2 / counts - mean * mean;
    pluss::Series result;
    if (operation == 0) {
      const auto variance =
          (s2 - s1 * s1 / counts) /
          (counts - static_cast<double>(options.ddof));
      result = variance.clamp_min(0.0).sqrt() / counts.sqrt();
    } else if (operation == 1) {
      const auto s3 = window_sum(p3);
      const auto m3 =
          s3 / counts - 3.0 * mean * (s2 / counts) +
          2.0 * mean * mean * mean;
      result =
          (m3 / (m2 * m2.clamp_min(0.0).sqrt())) *
          ((counts * (counts - 1.0)).sqrt() / (counts - 2.0));
      result = result.where(m2.eq(0.0), 0.0);
    } else {
      const auto s3 = window_sum(p3);
      const auto s4 = window_sum(p4);
      const auto mean2 = mean * mean;
      const auto m4 =
          s4 / counts - 4.0 * mean * (s3 / counts) +
          6.0 * mean2 * (s2 / counts) - 3.0 * mean2 * mean2;
      const auto raw = m4 / (m2 * m2) - 3.0;
      result =
          ((counts - 1.0) /
           ((counts - 2.0) * (counts - 3.0))) *
          ((counts + 1.0) * raw + 6.0);
      result = result.where(m2.eq(0.0), 0.0);
    }
    return Thread(
        result, counts.ge(static_cast<double>(minimum)));
  }
  const auto values = input.to(dtype);
  const auto valid = thread_valid_(source);
  const auto value_padding =
      pluss::Series::zeros({window - 1}, dtype, platform);
  const auto valid_padding = pluss::Series::zeros(
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
  if (operation == 0) {
    result =
        (squares.sum(1) /
         (counts - static_cast<double>(options.ddof))).sqrt() /
        counts.sqrt();
  } else if (operation == 1) {
    const auto m2 = squares.sum(1) / counts;
    const auto m3 = (squares * centered).sum(1) / counts;
    result = (m3 / (m2 * m2.sqrt())) *
             ((counts * (counts - 1.0)).sqrt() / (counts - 2.0));
    result = result.where(m2.eq(0.0), 0.0);
  } else {
    const auto m2 = squares.sum(1) / counts;
    const auto m4 = (squares * squares).sum(1) / counts;
    const auto raw = m4 / (m2 * m2) - 3.0;
    result = ((counts - 1.0) /
              ((counts - 2.0) * (counts - 3.0))) *
             ((counts + 1.0) * raw + 6.0);
    result = result.where(m2.eq(0.0), 0.0);
  }
  auto output_valid = counts.ge(static_cast<double>(minimum));
  if (options.centered) {
    const auto offset = (window - 1) / 2;
    if (offset > 0) {
      auto shifted = Thread(result, output_valid).shift(-offset);
      return shifted;
    }
  }
  return Thread(result, output_valid);
}

pluss::Series bool_from_u8_(const std::vector<std::uint8_t>& values) {
  auto out = pluss::Series::zeros(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Bool);
  auto* data = reinterpret_cast<bool*>(out.raw_data());
  std::transform(values.begin(), values.end(), data,
                 [](std::uint8_t value) { return value != 0; });
  return out;
}

Thread note_bool_result_(const Thread& source,
                         const std::vector<std::uint8_t>& values) {
  return Thread(bool_from_u8_(values), source.validity_mask());
}

std::regex compile_regex_(std::string_view pattern,
                          std::string_view where) {
  try {
    return std::regex(std::string(pattern));
  } catch (const std::regex_error& error) {
    PLUSS_FAIL(
        std::string(where) + ": invalid regular expression: " +
        error.what());
  }
}

} // namespace

ThreadValue Thread::covar(
    const Thread& other,
    const StatisticOptions& options) const {
  require_numeric_thread_(*this, "Thread::covar");
  require_numeric_thread_(other, "Thread::covar");
  PLUSS_CHECK(size() == other.size(),
              "Thread::covar: operands must have equal length");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Thread::covar: ddof and min_count must be >= 0");
  const auto platform = as_series().platform();
  PLUSS_CHECK(other.as_series().platform() == platform,
              "Thread::covar: operands must share a platform");
  const auto dtype = thread_compute_type_(as_series());
  const auto left_valid = thread_valid_(*this);
  const auto right_valid = thread_valid_(other);
  const auto valid = left_valid.logical_and(right_valid);
  const auto numeric_valid = valid.to(dtype);
  const auto count = numeric_valid.sum();
  const auto left = as_series().to(dtype);
  const auto right = other.as_series().to(dtype);
  const auto left_origin =
      left.where(valid.logical_not(), 0.0).sum() / count;
  const auto right_origin =
      right.where(valid.logical_not(), 0.0).sum() / count;
  const auto centered_left =
      (left - left_origin).where(valid.logical_not(), 0.0);
  const auto centered_right =
      (right - right_origin).where(valid.logical_not(), 0.0);
  const auto value =
      (centered_left * centered_right).sum() /
      (count - static_cast<double>(options.ddof));
  const auto minimum = std::max(options.min_count, options.ddof + 1);
  return ThreadValue(Thread(
      value.reshape({1}),
      count.ge(static_cast<double>(minimum)).reshape({1})));
}

Thread Thread::percentiles(
    pluss::msharp::RangeView::Double points,
    std::int64_t min_count) const {
  require_numeric_thread_(*this, "Thread::percentiles");
  PLUSS_CHECK(!points.empty(),
              "Thread::percentiles: expected at least one point");
  PLUSS_CHECK(min_count >= 0,
              "Thread::percentiles: min_count must be >= 0");
  auto clean = dropna().as_series();
  std::vector<pluss::Series> values;
  values.reserve(points.size());
  for (const auto point : points) {
    PLUSS_CHECK(point >= 0.0 && point <= 1.0,
                "Thread::percentiles: points must be in [0, 1]");
    auto value =
        clean.numel() == 0
            ? pluss::Series::zeros(
                  {1}, thread_compute_type_(as_series()),
                  as_series().platform())
            : clean.quantile(point).reshape({1});
    if (as_series().platform() != pluss::codem::Platform::CPU &&
        value.platform() == pluss::codem::Platform::CPU) {
      value = value.to(pluss::codem::ScalarType::Float32)
                  .to(as_series().platform());
    }
    values.push_back(std::move(value));
  }
  auto packed =
      pluss::pile(values, 0).reshape(
          {static_cast<std::int64_t>(points.size())});
  const auto valid =
      clean.numel() >= min_count
          ? pluss::Series::ones(
                packed.sizes(), pluss::codem::ScalarType::Bool,
                packed.platform())
          : pluss::Series::zeros(
                packed.sizes(), pluss::codem::ScalarType::Bool,
                packed.platform());
  return Thread(packed, valid);
}

Thread Thread::mode(bool drop_nulls) const {
  PLUSS_CHECK(
      kind() == ThreadKind::Series || kind() == ThreadKind::Note ||
          kind() == ThreadKind::Timeline ||
          kind() == ThreadKind::TimeIndex,
      "Thread::mode: unsupported payload");
  const auto validity = notnull().as_series().to_vec<bool>();
  std::int64_t null_count = 0;
  std::vector<bool> output_valid;
  if (kind() == ThreadKind::Series) {
    require_numeric_thread_(*this, "Thread::mode");
    const auto values =
        as_series().to(pluss::codem::Platform::CPU)
            .to(pluss::codem::ScalarType::Float64).to_vec<double>();
    std::map<double, std::int64_t> counts;
    for (std::size_t i = 0; i < values.size(); ++i) {
      validity[i] ? ++counts[values[i]] : ++null_count;
    }
    std::int64_t best = drop_nulls ? 0 : null_count;
    for (const auto& [value, count] : counts) {
      (void)value;
      best = std::max(best, count);
    }
    PLUSS_CHECK(best > 0, "Thread::mode: no values");
    std::vector<double> modes;
    for (const auto& [value, count] : counts) {
      if (count == best) {
        modes.push_back(value);
        output_valid.push_back(true);
      }
    }
    if (!drop_nulls && null_count == best) {
      modes.push_back(0.0);
      output_valid.push_back(false);
    }
    auto out = pluss::Series::empty(
        {static_cast<std::int64_t>(modes.size())},
        pluss::codem::ScalarType::Float64);
    std::copy(modes.begin(), modes.end(),
              reinterpret_cast<double*>(out.raw_data()));
    auto mask = bool_series_from_bools(output_valid);
    if (as_series().platform() != pluss::codem::Platform::CPU) {
      out = out.to(pluss::codem::ScalarType::Float32)
                .to(as_series().platform());
      mask = mask.to(as_series().platform());
    }
    return Thread(out, mask);
  }

  if (kind() == ThreadKind::Timeline ||
      kind() == ThreadKind::TimeIndex) {
    const auto ticks =
        kind() == ThreadKind::Timeline
            ? as_timeline().to_epoch_nanos_vector()
            : as_timeindex().timeline().to_epoch_nanos_vector();
    std::map<std::int64_t, std::pair<std::int64_t, std::int64_t>>
        counts;
    std::int64_t first_null = -1;
    for (std::int64_t i = 0; i < size(); ++i) {
      if (validity[static_cast<std::size_t>(i)]) {
        auto& [count, first] =
            counts[ticks[static_cast<std::size_t>(i)]];
        if (count == 0) {
          first = i;
        }
        ++count;
      } else {
        ++null_count;
        if (first_null < 0) {
          first_null = i;
        }
      }
    }
    std::int64_t best = drop_nulls ? 0 : null_count;
    for (const auto& [tick, occurrence] : counts) {
      (void)tick;
      best = std::max(best, occurrence.first);
    }
    PLUSS_CHECK(best > 0, "Thread::mode: no values");
    std::vector<std::int64_t> positions;
    for (const auto& [tick, occurrence] : counts) {
      (void)tick;
      if (occurrence.first == best) {
        positions.push_back(occurrence.second);
      }
    }
    if (!drop_nulls && null_count == best) {
      positions.push_back(first_null);
    }
    return take(positions);
  }

  const auto values = as_note().vector();
  std::map<std::string, std::int64_t> counts;
  for (std::size_t i = 0; i < values.size(); ++i) {
    validity[i] ? ++counts[std::string(values[i])] : ++null_count;
  }
  std::int64_t best = drop_nulls ? 0 : null_count;
  for (const auto& [value, count] : counts) {
    (void)value;
    best = std::max(best, count);
  }
  PLUSS_CHECK(best > 0, "Thread::mode: no values");
  std::vector<std::string> modes;
  for (const auto& [value, count] : counts) {
    if (count == best) {
      modes.push_back(value);
      output_valid.push_back(true);
    }
  }
  if (!drop_nulls && null_count == best) {
    modes.emplace_back();
    output_valid.push_back(false);
  }
  return Thread(pluss::make_note(modes),
                bool_series_from_bools(output_valid));
}

Thread Thread::rank(
    RankTieRule ties,
    RankNullRule nulls,
    bool ascending,
    bool percentage) const {
  require_numeric_thread_(*this, "Thread::rank");
  const auto n = size();
  auto values = as_series().to(pluss::codem::Platform::CPU)
                    .to(pluss::codem::ScalarType::Float64).to_vec<double>();
  auto valid = notnull().as_series().to_vec<bool>();
  std::vector<std::int64_t> order;
  order.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    if (valid[static_cast<std::size_t>(i)] ||
        nulls != RankNullRule::Keep) {
      order.push_back(i);
    }
  }
  const auto null_fill =
      nulls == RankNullRule::Top
          ? (ascending ? -std::numeric_limits<double>::infinity()
                       : std::numeric_limits<double>::infinity())
          : (ascending ? std::numeric_limits<double>::infinity()
                       : -std::numeric_limits<double>::infinity());
  for (std::int64_t i = 0; i < n; ++i) {
    if (!valid[static_cast<std::size_t>(i)]) {
      values[static_cast<std::size_t>(i)] = null_fill;
    }
  }
  std::stable_sort(order.begin(), order.end(), [&](auto left, auto right) {
    return ascending ? values[left] < values[right]
                     : values[left] > values[right];
  });
  std::vector<double> ranked(static_cast<std::size_t>(n), 0.0);
  std::int64_t dense = 1;
  for (std::size_t begin = 0; begin < order.size();) {
    std::size_t end = begin + 1;
    while (end < order.size() &&
           values[order[begin]] == values[order[end]]) {
      ++end;
    }
    for (std::size_t pos = begin; pos < end; ++pos) {
      double rank = 0.0;
      switch (ties) {
        case RankTieRule::Average:
          rank = (static_cast<double>(begin + 1) +
                  static_cast<double>(end)) / 2.0;
          break;
        case RankTieRule::Lowest:
          rank = static_cast<double>(begin + 1);
          break;
        case RankTieRule::Highest:
          rank = static_cast<double>(end);
          break;
        case RankTieRule::First:
          rank = static_cast<double>(pos + 1);
          break;
        case RankTieRule::Dense:
          rank = static_cast<double>(dense);
          break;
      }
      ranked[static_cast<std::size_t>(order[pos])] =
          percentage && !order.empty()
              ? rank / static_cast<double>(order.size())
              : rank;
    }
    ++dense;
    begin = end;
  }
  auto output = pluss::Series::empty(
      {n}, pluss::codem::ScalarType::Float64);
  std::copy(ranked.begin(), ranked.end(),
            reinterpret_cast<double*>(output.raw_data()));
  if (as_series().platform() != pluss::codem::Platform::CPU) {
    output = output.to(pluss::codem::ScalarType::Float32)
                 .to(as_series().platform());
  }
  auto output_valid =
      nulls == RankNullRule::Keep
          ? notnull().as_series().to(output.platform())
          : pluss::Series::ones(
                {n}, pluss::codem::ScalarType::Bool,
                output.platform());
  return Thread(output, output_valid);
}

ThreadValue Thread::skew(std::int64_t min_count) const {
  require_numeric_thread_(*this, "Thread::skew");
  const auto dtype = thread_compute_type_(as_series());
  const auto valid = thread_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values =
      as_series().to(dtype).where(valid.logical_not(), 0.0);
  const auto mean = values.sum() / count;
  const auto centered =
      (as_series().to(dtype) - mean).where(valid.logical_not(), 0.0);
  const auto m2 = (centered * centered).sum() / count;
  const auto m3 = (centered * centered * centered).sum() / count;
  auto result =
      (m3 / (m2 * m2.sqrt())) *
      ((count * (count - 1.0)).sqrt() / (count - 2.0));
  result = result.where(m2.eq(0.0), 0.0);
  return ThreadValue(Thread(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max<std::int64_t>(3, min_count))).reshape({1})));
}

ThreadValue Thread::kurtosis(std::int64_t min_count) const {
  require_numeric_thread_(*this, "Thread::kurtosis");
  const auto dtype = thread_compute_type_(as_series());
  const auto valid = thread_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values =
      as_series().to(dtype).where(valid.logical_not(), 0.0);
  const auto mean = values.sum() / count;
  const auto centered =
      (as_series().to(dtype) - mean).where(valid.logical_not(), 0.0);
  const auto square = centered * centered;
  const auto m2 = square.sum() / count;
  const auto m4 = (square * square).sum() / count;
  const auto raw = m4 / (m2 * m2) - 3.0;
  auto result =
      ((count - 1.0) / ((count - 2.0) * (count - 3.0))) *
      ((count + 1.0) * raw + 6.0);
  result = result.where(m2.eq(0.0), 0.0);
  return ThreadValue(Thread(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max<std::int64_t>(4, min_count))).reshape({1})));
}

ThreadValue Thread::var(const StatisticOptions& options) const {
  require_numeric_thread_(*this, "Thread::var");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Thread::var: ddof and min_count must be >= 0");
  const auto dtype = thread_compute_type_(as_series());
  const auto valid = thread_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values = as_series().to(dtype);
  const auto origin =
      values.where(valid.logical_not(), 0.0).sum() / count;
  const auto centered =
      (values - origin).where(valid.logical_not(), 0.0);
  const auto result =
      (centered * centered).sum() /
      (count - static_cast<double>(options.ddof));
  return ThreadValue(Thread(
      result.clamp_min(0.0).reshape({1}),
      count.ge(static_cast<double>(
          std::max(options.min_count, options.ddof + 1))).reshape({1})));
}

ThreadValue Thread::std(const StatisticOptions& options) const {
  const auto result = var(options);
  return ThreadValue(Thread(
      result.as_series().clamp_min(0.0).sqrt(),
      result.thread().validity_mask()));
}

ThreadValue Thread::se(ErrorTarget target, const StatisticOptions& options) const {
  PLUSS_CHECK(target == ErrorTarget::Mean,
              "Thread::se: only ErrorTarget::Mean is implemented in this pass");
  require_numeric_thread_(*this, "Thread::se");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Thread::se: ddof and min_count must be >= 0");
  const auto dtype = thread_compute_type_(as_series());
  const auto valid = thread_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values =
      as_series().to(dtype).where(valid.logical_not(), 0.0);
  const auto mean = values.sum() / count;
  const auto centered =
      (as_series().to(dtype) - mean).where(valid.logical_not(), 0.0);
  const auto variance =
      (centered * centered).sum() /
      (count - static_cast<double>(options.ddof));
  const auto result = variance.sqrt() / count.sqrt();
  return ThreadValue(Thread(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max(options.min_count, options.ddof + 1))).reshape({1})));
}

Thread Thread::scan(ScanStatistic statistic, bool skip_nulls) const {
  switch (statistic) {
    case ScanStatistic::Sum:
      return running_thread_(*this, skip_nulls, 0, "Thread::scan");
    case ScanStatistic::Product:
      return running_thread_(*this, skip_nulls, 1, "Thread::scan");
    case ScanStatistic::Minimum:
      return running_thread_(*this, skip_nulls, 2, "Thread::scan");
    case ScanStatistic::Maximum:
      return running_thread_(*this, skip_nulls, 3, "Thread::scan");
  }
  PLUSS_FAIL("Thread::scan: unsupported statistic");
}

ThreadValue Thread::corr_with(
    const Thread& other,
    ThreadCorrMethod method,
    std::int64_t min_count) const {
  require_numeric_thread_(*this, "Thread::corr_with");
  require_numeric_thread_(other, "Thread::corr_with");
  PLUSS_CHECK(size() == other.size(),
              "Thread::corr_with: operands must have equal length");
  PLUSS_CHECK(min_count >= 1,
              "Thread::corr_with: min_count must be >= 1");
  PLUSS_CHECK(as_series().platform() == other.as_series().platform(),
              "Thread::corr_with: operands must share a platform");
  const auto valid =
      notnull().as_series().logical_and(other.notnull().as_series());
  if (method == ThreadCorrMethod::Pearson) {
    const auto dtype = thread_compute_type_(as_series());
    const auto count = valid.to(dtype).sum();
    const auto left = as_series().to(dtype);
    const auto right = other.as_series().to(dtype);
    const auto left_origin =
        left.where(valid.logical_not(), 0.0).sum() / count;
    const auto right_origin =
        right.where(valid.logical_not(), 0.0).sum() / count;
    const auto centered_left =
        (left - left_origin).where(valid.logical_not(), 0.0);
    const auto centered_right =
        (right - right_origin).where(valid.logical_not(), 0.0);
    const auto numerator =
        (centered_left * centered_right).sum();
    const auto left_square =
        (centered_left * centered_left).sum();
    const auto right_square =
        (centered_right * centered_right).sum();
    const auto output_valid =
        count.ge(static_cast<double>(min_count))
            .logical_and(left_square.gt(0.0))
            .logical_and(right_square.gt(0.0));
    return ThreadValue(Thread(
        (numerator / (left_square * right_square).sqrt())
            .clamp(-1.0, 1.0).reshape({1}),
        output_valid.reshape({1})));
  }
  const auto count =
      valid.to(pluss::codem::ScalarType::Int64).sum().item<std::int64_t>();
  auto value = pluss::Series::zeros(
      {1}, thread_compute_type_(as_series()), as_series().platform());
  if (count >= min_count) {
    value = pluss::Series::full(
        {1}, corr(other, method), thread_compute_type_(as_series()),
        as_series().platform());
  }
  return ThreadValue(scalar_thread_(value, count >= min_count));
}

ThreadValue Thread::matmul(const Thread& other) const {
  require_numeric_thread_(*this, "Thread::matmul");
  require_numeric_thread_(other, "Thread::matmul");
  PLUSS_CHECK(!has_validity_mask() && !other.has_validity_mask(),
              "Thread::matmul: null values are not supported");
  return ThreadValue(Thread(as_series().matmul(other.as_series()).reshape({1})));
}

Thread Thread::bounded(
    const WindowBoundsFn& bounds,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  PLUSS_CHECK(static_cast<bool>(bounds),
              "Thread::bounded: bounds callback is empty");
  auto [starts, ends] = bounds(size(), options);
  return indexed_numeric_window_(
      *this, starts, ends, statistic, options,
      "Thread::bounded");
}

Thread Thread::moving(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  if (statistic == WindowStatistic::Sem) {
    return moving_moment_thread_(*this, window, options, 0, "Thread::moving");
  }
  if (statistic == WindowStatistic::Skew) {
    return moving_moment_thread_(*this, window, options, 1, "Thread::moving");
  }
  if (statistic == WindowStatistic::Kurtosis) {
    return moving_moment_thread_(*this, window, options, 2, "Thread::moving");
  }
  auto [starts, ends] = fixed_bounds_(size(), window, options);
  return indexed_numeric_window_(
      *this, starts, ends, statistic, options, "Thread::moving");
}

Thread Thread::growing(
    WindowStatistic statistic,
    const StatisticOptions& options) const {
  WindowOptions window_options{
      .min_count = options.min_count,
      .ddof = options.ddof,
      .centered = false,
      .closure = WindowClosure::Right};
  return moving(
      std::max<std::int64_t>(1, size()), statistic, window_options);
}

Thread exponential_mean_thread_(
    const Thread& source,
    double alpha,
    bool adjusted,
    std::int64_t min_count,
    bool ignore_nulls) {
  require_numeric_thread_(source, "Thread::decay");
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0,
              "Thread::decay: alpha must be in (0, 1]");
  PLUSS_CHECK(min_count > 0,
              "Thread::decay: min_count must be > 0");
  const auto dtype = thread_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  const auto values = source.as_series().to(dtype);
  const auto valid = thread_valid_(source);
  const auto count = valid.to(dtype).cumsum(0);
  if (!adjusted) {
    PLUSS_CHECK(
        !source.has_validity_mask(),
        "Thread::decay(adjusted=false): nullable input requires "
        "adjusted=true");
    auto result = Thread(pluss::subsp::stats::ts::ema(values, alpha));
    return result.with_validity_mask(
        count.ge(static_cast<double>(min_count)));
  }
  if (alpha == 1.0) {
    return Thread(values, valid.logical_and(
        count.ge(static_cast<double>(min_count))));
  }
  const auto beta = 1.0 - alpha;
  auto positions = pluss::Series::arange(
      0, source.size(), 1, pluss::codem::ScalarType::Int64, platform).to(dtype);
  if (ignore_nulls) {
    positions = count - 1.0;
  }
  const auto inverse_weight =
      (positions * -std::log(beta)).exp();
  const auto numerator =
      (values.where(valid.logical_not(), 0.0) * inverse_weight).cumsum(0);
  const auto denominator =
      (valid.to(dtype) * inverse_weight).cumsum(0);
  return Thread(
      numerator / denominator,
      count.ge(static_cast<double>(min_count)));
}

Thread exponential_variance_thread_(
    const Thread& source,
    double alpha,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) {
  require_numeric_thread_(source, "Thread::decay");
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0,
              "Thread::decay: alpha must be in (0, 1]");
  const auto dtype = thread_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  const auto values = source.as_series().to(dtype);
  const auto valid = thread_valid_(source);
  const auto count = valid.to(dtype).cumsum(0);
  const auto origin =
      values.where(valid.logical_not(), 0.0).sum() /
      valid.to(dtype).sum();
  const auto centered =
      (values - origin).where(valid.logical_not(), 0.0);
  const auto centered_thread =
      Thread(centered, source.validity_mask());
  const auto mean = exponential_mean_thread_(centered_thread,
      alpha, adjusted, 1, ignore_nulls).as_series();
  pluss::Series second;
  auto correction =
      pluss::Series::ones({source.size()}, dtype, platform);
  if (adjusted && alpha < 1.0) {
    auto positions = pluss::Series::arange(
        0, source.size(), 1, pluss::codem::ScalarType::Int64, platform).to(dtype);
    if (ignore_nulls) {
      positions = count - 1.0;
    }
    const auto inverse_weight =
        (positions * -std::log(1.0 - alpha)).exp();
    const auto denominator =
        (valid.to(dtype) * inverse_weight).cumsum(0);
    second =
        (centered * centered * inverse_weight).cumsum(0) /
        denominator;
    if (bias_corrected) {
      const auto denominator2 =
          (valid.to(dtype) * inverse_weight * inverse_weight).cumsum(0);
      correction =
          (denominator * denominator) /
          (denominator * denominator - denominator2);
    }
  } else {
    PLUSS_CHECK(
        !source.has_validity_mask(),
        "Thread::decay(adjusted=false): nullable input "
        "requires adjusted=true");
    second = pluss::subsp::stats::ts::ema(
        centered * centered, alpha);
    if (bias_corrected) {
      if (alpha == 1.0) {
        correction = pluss::Series::full(
            {source.size()}, std::numeric_limits<double>::infinity(),
            dtype, platform);
      } else {
        const auto positions = pluss::Series::arange(
            0, source.size(), 1, pluss::codem::ScalarType::Int64,
            platform).to(dtype);
        const auto beta2 =
            (positions * (2.0 * std::log(1.0 - alpha))).exp();
        const auto weight_square_sum =
            beta2 + alpha * alpha * (1.0 - beta2) /
                        (1.0 - (1.0 - alpha) * (1.0 - alpha));
        correction = 1.0 / (1.0 - weight_square_sum);
      }
    }
  }
  auto result = (second - mean * mean).clamp_min(0.0);
  if (bias_corrected) {
    result = result * correction;
  }
  const auto required =
      std::max<std::int64_t>(min_count, bias_corrected ? 2 : 1);
  auto output_valid = count.ge(static_cast<double>(required));
  if (bias_corrected && alpha == 1.0) {
    output_valid = pluss::Series::zeros(
        {source.size()}, pluss::codem::ScalarType::Bool, platform);
  }
  return Thread(result, output_valid);
}

Thread Thread::decay(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  switch (statistic) {
    case WindowStatistic::Mean:
      return exponential_mean_thread_(*this, alpha, adjusted, min_count, ignore_nulls);
    case WindowStatistic::Var:
      return exponential_variance_thread_(
          *this, alpha, adjusted, bias_corrected, min_count, ignore_nulls);
    case WindowStatistic::Std: {
      const auto variance = exponential_variance_thread_(
          *this, alpha, adjusted, bias_corrected, min_count, ignore_nulls);
      return Thread(variance.as_series().sqrt(), variance.validity_mask());
    }
    default:
      PLUSS_FAIL("Thread::decay: only Mean, Var, and Std are supported");
  }
}

Thread Thread::weigh(
    pluss::msharp::RangeView::Double weights,
    WindowStatistic statistic,
    std::int64_t min_count) const {
  require_numeric_thread_(*this, "Thread::weigh");
  PLUSS_CHECK(!weights.empty(),
              "Thread::weigh: weights cannot be empty");
  PLUSS_CHECK(min_count > 0 &&
                  min_count <= static_cast<std::int64_t>(weights.size()),
              "Thread::weigh: invalid min_count");
  if (statistic == WindowStatistic::Minimum ||
      statistic == WindowStatistic::Maximum) {
    return moving(
        static_cast<std::int64_t>(weights.size()), statistic,
        WindowOptions{.min_count = min_count});
  }
  const auto n = size();
  const auto window = static_cast<std::int64_t>(weights.size());
  const auto dtype = thread_compute_type_(as_series());
  const auto platform = as_series().platform();
  auto weight_series = pluss::Series::empty(
      {window}, pluss::codem::ScalarType::Float64);
  std::copy(weights.begin(), weights.end(),
            reinterpret_cast<double*>(weight_series.raw_data()));
  if (platform != pluss::codem::Platform::CPU) {
    weight_series =
        weight_series.to(pluss::codem::ScalarType::Float32).to(platform);
  }
  const auto values = as_series().to(dtype);
  const auto valid = thread_valid_(*this);
  const auto padded_values =
      pluss::Series::zeros({window - 1}, dtype, platform)
          .unite({values.where(valid.logical_not(), 0.0)}, 0, true)
          .unfold(0, window, 1);
  const auto padded_valid =
      pluss::Series::zeros(
          {window - 1}, pluss::codem::ScalarType::Bool, platform)
          .unite({valid}, 0, true)
          .unfold(0, window, 1);
  const auto active_weights =
      padded_valid.to(dtype) * weight_series.reshape({1, window});
  const auto weight_sum = active_weights.sum(1);
  const auto weighted_sum =
      (padded_values * active_weights).sum(1);
  const auto count = padded_valid.to(dtype).sum(1);
  pluss::Series result;
  if (statistic == WindowStatistic::Sum) {
    result = weighted_sum;
  } else if (statistic == WindowStatistic::Mean) {
    result = weighted_sum / weight_sum;
  } else {
    const auto mean = weighted_sum / weight_sum;
    const auto centered =
        (padded_values - mean.reshape({n, 1}))
            .where(padded_valid.logical_not(), 0.0);
    auto variance =
        (centered * centered * active_weights).sum(1) / weight_sum;
    variance = variance.clamp_min(0.0);
    result = statistic == WindowStatistic::Std
                 ? variance.sqrt()
                 : variance;
  }
  return Thread(
      result, count.ge(static_cast<double>(min_count)));
}

Thread Thread::timed(
    const Thread& axis,
    std::int64_t duration_nanos,
    TimeWindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  PLUSS_CHECK(axis.size() == size(),
              "Thread::timed: axis length mismatch");
  PLUSS_CHECK(duration_nanos > 0,
              "Thread::timed: duration_nanos must be > 0");
  PLUSS_CHECK(
      axis.notnull().as_series().all().item<bool>(),
      "Thread::timed: axis cannot contain null values");
  std::vector<std::int64_t> ticks;
  if (axis.kind() == ThreadKind::Timeline) {
    ticks = axis.as_timeline().to_epoch_nanos_vector();
  } else if (axis.kind() == ThreadKind::TimeIndex) {
    ticks = axis.as_timeindex().timeline().to_epoch_nanos_vector();
  } else if (axis.kind() == ThreadKind::Series &&
             axis.as_series().scalar_type() ==
                 pluss::codem::ScalarType::Int64) {
    ticks = axis.as_series().to(
        pluss::codem::Platform::CPU).to_vec<std::int64_t>();
  } else {
    PLUSS_FAIL(
        "Thread::timed: axis must be Timeline, TimeIndex, or Int64 "
        "nanoseconds");
  }
  PLUSS_CHECK(std::is_sorted(ticks.begin(), ticks.end()),
              "Thread::timed: axis must be monotonic");
  std::vector<std::int64_t> starts(ticks.size());
  std::vector<std::int64_t> ends(ticks.size());
  std::int64_t start = 0;
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto boundary =
        ticks[static_cast<std::size_t>(row)] - duration_nanos;
    const bool include_left =
        closure == WindowClosure::Left ||
        closure == WindowClosure::Both;
    while (start < row &&
           (include_left
                ? ticks[static_cast<std::size_t>(start)] < boundary
                : ticks[static_cast<std::size_t>(start)] <= boundary)) {
      ++start;
    }
    starts[static_cast<std::size_t>(row)] = start;
    const bool include_right =
        closure == WindowClosure::Right ||
        closure == WindowClosure::Both;
    ends[static_cast<std::size_t>(row)] =
        row + (include_right ? 1 : 0);
  }
  WindowStatistic mapped = WindowStatistic::Mean;
  switch (statistic) {
    case TimeWindowStatistic::Sum:
      mapped = WindowStatistic::Sum;
      break;
    case TimeWindowStatistic::Mean:
      mapped = WindowStatistic::Mean;
      break;
    case TimeWindowStatistic::Var:
      mapped = WindowStatistic::Var;
      break;
    case TimeWindowStatistic::Std:
      mapped = WindowStatistic::Std;
      break;
  }
  return indexed_numeric_window_(
      *this, starts, ends, mapped,
      WindowOptions{
          .min_count = options.min_count,
          .ddof = options.ddof,
          .centered = false,
          .closure = closure},
      "Thread::timed");
}

Thread Thread::grouped(
    const Thread& groups,
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  require_numeric_thread_(*this, "Thread::grouped");
  PLUSS_CHECK(groups.size() == size(),
              "Thread::grouped: group length mismatch");
  Thread cpu_groups = groups;
  if (groups.kind() == ThreadKind::Series &&
      groups.as_series().platform() != pluss::codem::Platform::CPU) {
    cpu_groups = Thread(
        groups.as_series().to(pluss::codem::Platform::CPU),
        groups.validity_mask());
  }
  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_group;
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto key =
        groups.is_valid(row)
            ? std::string("\1") + thread_scalar_string_at(cpu_groups, row)
            : std::string(1, '\0');
    rows_by_group[key].push_back(row);
  }
  const auto dtype = thread_compute_type_(as_series());
  const auto platform = as_series().platform();
  auto output = pluss::Series::zeros({size()}, dtype, platform);
  auto output_valid = pluss::Series::zeros(
      {size()}, pluss::codem::ScalarType::Bool, platform);
  for (const auto& [key, rows] : rows_by_group) {
    (void)key;
    const auto index = index_series_(rows, platform);
    const auto result =
        take(rows).moving(window, statistic, options);
    output =
        output.index_copy(0, index, result.as_series().to(dtype));
    output_valid = output_valid.index_copy(
        0, index, result.notnull().as_series().to(platform));
  }
  return Thread(output, output_valid);
}

Thread Thread::flow(
    const std::function<Thread(const Thread&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Thread::flow: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Thread::flow: callback returned an undefined Thread");
  return result;
}

Thread Thread::fold(
    const std::function<Thread(const Thread&)>& fn,
    ResultShape shape) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Thread::fold: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Thread::fold: callback returned undefined");
  if (shape == ResultShape::Reduce) {
    PLUSS_CHECK(result.size() == 1,
                "Thread::fold: Reduce requires one value");
  } else if (shape == ResultShape::Preserve) {
    PLUSS_CHECK(result.size() == size(),
                "Thread::fold: Preserve requires input length");
  }
  return result;
}

Thread Thread::transform(
    const std::function<ThreadValue(const ThreadValue&)>& fn) const {
  return map(fn);
}

Thread Thread::transform(
    const std::function<Thread(const Thread&)>& fn,
    ResultShape shape) const {
  return fold(fn, shape);
}

Thread Thread::map(
    const std::function<ThreadValue(const ThreadValue&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Thread::map: callback is empty");
  std::vector<std::optional<Thread>> mapped_values(
      static_cast<std::size_t>(size()));
  std::optional<Thread> prototype;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    const std::array<std::int64_t, 1> index{row};
    auto mapped = fn(ThreadValue(take(index)));
    PLUSS_CHECK(mapped.defined() && mapped.thread().size() == 1,
                "Thread::map: callback must return one value");
    if (prototype.has_value()) {
      PLUSS_CHECK(mapped.kind() == prototype->kind(),
                  "Thread::map: output kinds must be consistent");
    }
    prototype = mapped.thread();
    mapped_values[static_cast<std::size_t>(row)] = mapped.thread();
  }
  if (!prototype.has_value()) {
    return *this;
  }
  auto null_mask = pluss::Series::zeros(
      {1}, pluss::codem::ScalarType::Bool);
  if (prototype->kind() == ThreadKind::Series &&
      prototype->as_series().platform() !=
          pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(prototype->as_series().platform());
  } else if (prototype->kind() == ThreadKind::Timeline &&
             prototype->as_timeline().epoch_nanos().platform() !=
                 pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(
        prototype->as_timeline().epoch_nanos().platform());
  } else if (
      prototype->kind() == ThreadKind::TimeIndex &&
      prototype->as_timeindex().timeline().epoch_nanos().platform() !=
          pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(
        prototype->as_timeindex().timeline().epoch_nanos().platform());
  }
  std::vector<Thread> values;
  values.reserve(mapped_values.size());
  for (auto& value : mapped_values) {
    values.push_back(
        value.has_value()
            ? std::move(*value)
            : prototype->with_validity_mask(null_mask));
  }
  const auto kind = values.front().kind();
  std::vector<bool> validity;
  validity.reserve(values.size());
  if (kind == ThreadKind::Series) {
    std::vector<pluss::Series> payloads;
    payloads.reserve(values.size());
    for (const auto& value : values) {
      payloads.push_back(value.as_series());
      validity.push_back(value.is_valid(0));
    }
    auto output = pluss::unite(payloads, 0, true);
    auto mask = bool_series_from_bools(validity);
    if (output.platform() != pluss::codem::Platform::CPU) {
      mask = mask.to(output.platform());
    }
    return Thread(output, mask);
  }
  if (kind == ThreadKind::Note) {
    std::vector<std::string> payloads;
    payloads.reserve(values.size());
    for (const auto& value : values) {
      payloads.emplace_back(value.as_note().at(0));
      validity.push_back(value.is_valid(0));
    }
    return Thread(pluss::make_note(payloads),
                  bool_series_from_bools(validity));
  }
  if (kind == ThreadKind::Timeline) {
    auto output = values.front().as_timeline();
    for (std::size_t i = 1; i < values.size(); ++i) {
      output = output.concat(values[i].as_timeline());
    }
    for (const auto& value : values) {
      validity.push_back(value.is_valid(0));
    }
    auto mask = bool_series_from_bools(validity);
    if (output.epoch_nanos().platform() !=
        pluss::codem::Platform::CPU) {
      mask = mask.to(output.epoch_nanos().platform());
    }
    return Thread(output, mask);
  }
  auto timeline = values.front().as_timeindex().timeline();
  for (std::size_t i = 1; i < values.size(); ++i) {
    timeline = timeline.concat(values[i].as_timeindex().timeline());
  }
  for (const auto& value : values) {
    validity.push_back(value.is_valid(0));
  }
  auto mask = bool_series_from_bools(validity);
  if (timeline.epoch_nanos().platform() !=
      pluss::codem::Platform::CPU) {
    mask = mask.to(timeline.epoch_nanos().platform());
  }
  return Thread(timeline.to_timeindex(false), mask);
}

Thread Thread::upper() const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::upper: expected Note payload");
  return Thread(as_note().upper(), validity_mask());
}

Thread Thread::casefold() const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::casefold: expected Note payload");
  return Thread(as_note().casefold(), validity_mask());
}

Thread Thread::titlecase() const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::titlecase: expected Note payload");
  return Thread(as_note().title(), validity_mask());
}

Thread Thread::length() const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::length: expected Note payload");
  const auto lengths = as_note().lengths_chars();
  auto output = pluss::Series::empty(
      {static_cast<std::int64_t>(lengths.size())},
      pluss::codem::ScalarType::Int64);
  std::copy(lengths.begin(), lengths.end(),
            reinterpret_cast<std::int64_t*>(output.raw_data()));
  return Thread(output, validity_mask());
}

Thread Thread::starts(std::string_view prefix) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::starts: expected Note payload");
  return note_bool_result_(*this, as_note().starts_with(prefix));
}

Thread Thread::ends(std::string_view suffix) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::ends: expected Note payload");
  return note_bool_result_(*this, as_note().ends_with(suffix));
}

Thread Thread::matches(std::string_view pattern) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::matches: expected Note payload");
  (void)compile_regex_(pattern, "Thread::matches");
  const auto anchored = "^(?:" + std::string(pattern) + ")";
  return note_bool_result_(*this, as_note().regex_search(anchored));
}

Thread Thread::fullmatch(std::string_view pattern) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::fullmatch: expected Note payload");
  (void)compile_regex_(pattern, "Thread::fullmatch");
  return note_bool_result_(*this, as_note().regex_match(pattern));
}

DataFrame Thread::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::extract: expected Note payload");
  auto expression =
      compile_regex_(pattern, "Thread::extract");
  const auto group_count = expression.mark_count();
  PLUSS_CHECK(group_count > 0,
              "Thread::extract: pattern needs a capture group");
  PLUSS_CHECK(names.empty() || names.size() == group_count,
              "Thread::extract: names must match capture groups");
  std::vector<std::vector<std::string>> columns(group_count);
  std::vector<std::vector<bool>> valid(group_count);
  for (auto& column : columns) {
    column.reserve(static_cast<std::size_t>(size()));
  }
  for (auto& mask : valid) {
    mask.reserve(static_cast<std::size_t>(size()));
  }
  for (std::int64_t row = 0; row < size(); ++row) {
    std::cmatch match;
    const auto text = as_note().at(row);
    const bool matched =
        is_valid(row) &&
        std::regex_search(text.begin(), text.end(), match, expression);
    for (std::size_t group = 0; group < group_count; ++group) {
      const bool observed =
          matched && match[group + 1].matched;
      columns[group].push_back(
          observed ? match[group + 1].str() : std::string{});
      valid[group].push_back(observed);
    }
  }
  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(group_count);
  for (std::size_t group = 0; group < group_count; ++group) {
    const auto name =
        names.empty() ? "group_" + std::to_string(group)
                      : std::string(names[group]);
    output.emplace_back(
        name,
        Thread(pluss::make_note(columns[group]),
               bool_series_from_bools(valid[group])));
  }
  return DataFrame(std::move(output));
}

DataFrame Thread::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::split: expected Note payload");
  const auto batch =
      limit < 0 ? as_note().split(delimiter)
                : as_note().split(delimiter, limit);
  const auto& offsets = batch.offsets();
  const auto tokens = batch.tokens().vector();
  std::int64_t width = 0;
  for (std::size_t row = 0; row + 1 < offsets.size(); ++row) {
    width = std::max(width, offsets[row + 1] - offsets[row]);
  }
  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(static_cast<std::size_t>(width));
  for (std::int64_t part = 0; part < width; ++part) {
    std::vector<std::string> values(static_cast<std::size_t>(size()));
    std::vector<bool> valid(static_cast<std::size_t>(size()), false);
    for (std::int64_t row = 0; row < size(); ++row) {
      const auto begin = offsets[static_cast<std::size_t>(row)];
      const auto stop = offsets[static_cast<std::size_t>(row + 1)];
      if (is_valid(row) && begin + part < stop) {
        values[static_cast<std::size_t>(row)] =
            std::string(tokens[static_cast<std::size_t>(begin + part)]);
        valid[static_cast<std::size_t>(row)] = true;
      }
    }
    output.emplace_back(
        prefix + "_" + std::to_string(part),
        Thread(pluss::make_note(values),
               bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

DataFrame Thread::partition(
    std::string_view separator,
    std::string prefix) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::partition: expected Note payload");
  PLUSS_CHECK(!separator.empty(),
              "Thread::partition: separator cannot be empty");
  std::array<std::vector<std::string>, 3> values;
  for (auto& column : values) {
    column.reserve(static_cast<std::size_t>(size()));
  }
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto text = std::string(as_note().at(row));
    const auto pos = text.find(separator);
    if (pos == std::string::npos) {
      values[0].push_back(text);
      values[1].emplace_back();
      values[2].emplace_back();
    } else {
      values[0].push_back(text.substr(0, pos));
      values[1].push_back(std::string(separator));
      values[2].push_back(text.substr(pos + separator.size()));
    }
  }
  std::vector<std::pair<std::string, Thread>> output;
  for (std::size_t part = 0; part < values.size(); ++part) {
    output.emplace_back(
        prefix + "_" + std::to_string(part),
        Thread(pluss::make_note(values[part]), validity_mask()));
  }
  return DataFrame(std::move(output));
}

Thread Thread::pad(
    std::int64_t width,
    TextAlign align,
    char fill) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::pad: expected Note payload");
  PLUSS_CHECK(width >= 0, "Thread::pad: width must be >= 0");
  const auto lengths = as_note().lengths_chars();
  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t row = 0; row < size(); ++row) {
    auto value = std::string(as_note().at(row));
    const auto needed =
        std::max<std::int64_t>(
            0, width - lengths[static_cast<std::size_t>(row)]);
    const auto left =
        align == TextAlign::Right
            ? needed
            : align == TextAlign::Center ? needed / 2 : 0;
    const auto right = needed - left;
    values.push_back(
        std::string(static_cast<std::size_t>(left), fill) + value +
        std::string(static_cast<std::size_t>(right), fill));
  }
  return Thread(rebuild_note_like(as_note(), values), validity_mask());
}

Thread Thread::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::chars: expected Note payload");
  PLUSS_CHECK(step != 0, "Thread::chars: step cannot be zero");
  const auto chars = as_note().split_chars();
  const auto& offsets = chars.offsets();
  const auto tokens = chars.tokens().vector();
  std::vector<std::string> output;
  output.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto begin = offsets[static_cast<std::size_t>(row)];
    const auto end = offsets[static_cast<std::size_t>(row + 1)];
    const auto length = end - begin;
    auto first = start < 0 ? length + start : start;
    auto last = stop.value_or(step > 0 ? length : -1);
    if (last < 0 && stop.has_value()) {
      last += length;
    }
    std::string value;
    if (step > 0) {
      first = std::clamp<std::int64_t>(first, 0, length);
      last = std::clamp<std::int64_t>(last, 0, length);
      for (auto pos = first; pos < last; pos += step) {
        value += tokens[static_cast<std::size_t>(begin + pos)];
      }
    } else {
      first = std::clamp<std::int64_t>(first, -1, length - 1);
      last = std::clamp<std::int64_t>(last, -1, length - 1);
      for (auto pos = first; pos > last; pos += step) {
        value += tokens[static_cast<std::size_t>(begin + pos)];
      }
    }
    output.push_back(std::move(value));
  }
  return Thread(rebuild_note_like(as_note(), output), validity_mask());
}

Thread Thread::normalize(UnicodeForm form) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::normalize: expected Note payload");
  return Thread(
      as_note().normalize(static_cast<std::int64_t>(form)),
      validity_mask());
}

Thread Thread::encode(std::string_view encoding) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::encode: expected Note payload");
  if (encoding == "base64") {
    return Thread(as_note().base64_encode(), validity_mask());
  }
  if (encoding == "hex") {
    return Thread(as_note().hex_encode(), validity_mask());
  }
  if (encoding == "url") {
    return Thread(as_note().url_encode(), validity_mask());
  }
  PLUSS_FAIL(
      "Thread::encode: encoding must be base64, hex, or url");
}

DataFrame Thread::indicate(std::string_view delimiter,
                                  std::string prefix) const {
  PLUSS_CHECK(kind() == ThreadKind::Note,
              "Thread::indicate: expected Note payload");
  PLUSS_CHECK(!delimiter.empty(),
              "Thread::indicate: delimiter cannot be empty");
  const auto batch = as_note().split(delimiter);
  const auto& offsets = batch.offsets();
  const auto tokens = batch.tokens().vector();
  std::set<std::string> levels;
  for (std::int64_t token = 0;
       token < static_cast<std::int64_t>(tokens.size()); ++token) {
    if (!tokens[static_cast<std::size_t>(token)].empty()) {
      levels.insert(
          std::string(tokens[static_cast<std::size_t>(token)]));
    }
  }
  std::vector<std::pair<std::string, Thread>> output;
  output.reserve(levels.size());
  for (const auto& level : levels) {
    std::vector<bool> indicators(static_cast<std::size_t>(size()), false);
    for (std::int64_t row = 0; row < size(); ++row) {
      if (!is_valid(row)) {
        continue;
      }
      for (auto token = offsets[static_cast<std::size_t>(row)];
           token < offsets[static_cast<std::size_t>(row + 1)];
           ++token) {
        if (tokens[static_cast<std::size_t>(token)] == level) {
          indicators[static_cast<std::size_t>(row)] = true;
          break;
        }
      }
    }
    output.emplace_back(
        prefix + level,
        Thread(bool_series_from_bools(indicators)));
  }
  return DataFrame(std::move(output));
}

ThreadNoteAccessor Thread::note() const {
  return ThreadNoteAccessor(*this);
}

ThreadNoteAccessor::ThreadNoteAccessor(Thread base)
    : base_(std::move(base)) {}

Thread ThreadNoteAccessor::strip() const { return base_.strip(); }
Thread ThreadNoteAccessor::strip(std::string_view chars) const { return base_.strip(chars); }
Thread ThreadNoteAccessor::lstrip() const { return base_.lstrip(); }
Thread ThreadNoteAccessor::lstrip(std::string_view chars) const { return base_.lstrip(chars); }
Thread ThreadNoteAccessor::rstrip() const { return base_.rstrip(); }
Thread ThreadNoteAccessor::rstrip(std::string_view chars) const { return base_.rstrip(chars); }
Thread ThreadNoteAccessor::lower() const { return base_.lower(); }
Thread ThreadNoteAccessor::upper() const { return base_.upper(); }
Thread ThreadNoteAccessor::casefold() const { return base_.casefold(); }
Thread ThreadNoteAccessor::titlecase() const { return base_.titlecase(); }
Thread ThreadNoteAccessor::length() const { return base_.length(); }
Thread ThreadNoteAccessor::starts(std::string_view prefix) const { return base_.starts(prefix); }
Thread ThreadNoteAccessor::ends(std::string_view suffix) const { return base_.ends(suffix); }
Thread ThreadNoteAccessor::matches(std::string_view pattern, bool full) const {
  return full ? base_.fullmatch(pattern) : base_.matches(pattern);
}
Thread ThreadNoteAccessor::fullmatch(std::string_view pattern) const { return base_.fullmatch(pattern); }
DataFrame ThreadNoteAccessor::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  return base_.extract(pattern, names);
}
DataFrame ThreadNoteAccessor::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  return base_.split(delimiter, limit, std::move(prefix));
}
DataFrame ThreadNoteAccessor::partition(
    std::string_view separator,
    std::string prefix) const {
  return base_.partition(separator, std::move(prefix));
}
Thread ThreadNoteAccessor::pad(std::int64_t width, TextAlign align, char fill) const {
  return base_.pad(width, align, fill);
}
Thread ThreadNoteAccessor::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  return base_.chars(start, stop, step);
}
Thread ThreadNoteAccessor::normalize(UnicodeForm form) const {
  return base_.normalize(form);
}
Thread ThreadNoteAccessor::encode(std::string_view encoding) const {
  return base_.encode(encoding);
}
DataFrame ThreadNoteAccessor::indicate(std::string_view delimiter, std::string prefix) const {
  return base_.indicate(delimiter, std::move(prefix));
}

namespace {

const pluss::Timeline& tl_payload_(const Thread& base, std::string_view where) {
  if (base.kind() == ThreadKind::Timeline) {
    return base.as_timeline();
  }
  if (base.kind() == ThreadKind::TimeIndex) {
    return base.as_timeindex().timeline();
  }
  PLUSS_FAIL(std::string(where) + ": expected Timeline or TimeIndex payload");
}

pluss::Series tl_validity_(const Thread& base, std::string_view where) {
  auto valid = tl_payload_(base, where).isvalid();
  if (base.has_validity_mask()) {
    valid = valid.logical_and(*base.validity_mask());
  }
  return valid;
}

Thread tl_series_(const Thread& base, const pluss::Series& values, std::string_view where) {
  auto out = Thread(values.reshape({base.size()}), tl_validity_(base, where));
  if (base.has_display_labels()) {
    out = out.with_display_labels(base.display_labels());
  }
  if (base.has_display_name()) {
    out = out.with_display_name(base.display_name());
  }
  return out;
}

Thread tl_note_(const Thread& base, const pluss::Note& values, std::string_view where) {
  auto out = Thread(values, tl_validity_(base, where));
  if (base.has_display_labels()) {
    out = out.with_display_labels(base.display_labels());
  }
  if (base.has_display_name()) {
    out = out.with_display_name(base.display_name());
  }
  return out;
}

Thread tl_temporal_(const Thread& base, const pluss::Timeline& values) {
  auto out = base.kind() == ThreadKind::TimeIndex
                 ? Thread(values.to_timeindex(false), base.validity_mask())
                 : Thread(values, base.validity_mask());
  if (base.has_display_labels()) {
    out = out.with_display_labels(base.display_labels());
  }
  if (base.has_display_name()) {
    out = out.with_display_name(base.display_name());
  }
  return out;
}

} // namespace

ThreadTimelineAccessor Thread::tl() const {
  return ThreadTimelineAccessor(*this);
}

ThreadTimelineAccessor::ThreadTimelineAccessor(Thread base)
    : base_(std::move(base)) {
  PLUSS_CHECK(base_.defined(), "Thread::tl: Thread is undefined");
  PLUSS_CHECK(base_.kind() == ThreadKind::Timeline || base_.kind() == ThreadKind::TimeIndex,
              "Thread::tl: expected Timeline or TimeIndex payload");
}

Thread ThreadTimelineAccessor::isnat() const {
  return Thread(tl_payload_(base_, "Thread::tl().isnat").isnat(), base_.validity_mask());
}

Thread ThreadTimelineAccessor::valid() const {
  return Thread(tl_validity_(base_, "Thread::tl().valid"));
}

Thread ThreadTimelineAccessor::year() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().year").year(), "Thread::tl().year");
}

Thread ThreadTimelineAccessor::month() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().month").month(), "Thread::tl().month");
}

Thread ThreadTimelineAccessor::day() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().day").day(), "Thread::tl().day");
}

Thread ThreadTimelineAccessor::hour() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().hour").hour(), "Thread::tl().hour");
}

Thread ThreadTimelineAccessor::minute() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().minute").minute(), "Thread::tl().minute");
}

Thread ThreadTimelineAccessor::second() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().second").second(), "Thread::tl().second");
}

Thread ThreadTimelineAccessor::weekday() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().weekday").weekday(), "Thread::tl().weekday");
}

Thread ThreadTimelineAccessor::yearday() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().yearday").dayofyear(), "Thread::tl().yearday");
}

Thread ThreadTimelineAccessor::quarter() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().quarter").quarter(), "Thread::tl().quarter");
}

Thread ThreadTimelineAccessor::week() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().week").week(), "Thread::tl().week");
}

Thread ThreadTimelineAccessor::weekend() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().weekend").is_weekend(), "Thread::tl().weekend");
}

Thread ThreadTimelineAccessor::month_start() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().month_start").is_month_start(), "Thread::tl().month_start");
}

Thread ThreadTimelineAccessor::month_end() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().month_end").is_month_end(), "Thread::tl().month_end");
}

Thread ThreadTimelineAccessor::leap() const {
  return tl_series_(base_, tl_payload_(base_, "Thread::tl().leap").is_leap_year(), "Thread::tl().leap");
}

Thread ThreadTimelineAccessor::weekday_name() const {
  return tl_note_(base_, tl_payload_(base_, "Thread::tl().weekday_name").strftime("%A"), "Thread::tl().weekday_name");
}

Thread ThreadTimelineAccessor::month_name() const {
  return tl_note_(base_, tl_payload_(base_, "Thread::tl().month_name").strftime("%B"), "Thread::tl().month_name");
}

Thread ThreadTimelineAccessor::fmt(std::string_view pattern) const {
  PLUSS_CHECK(!pattern.empty(), "Thread::tl().fmt: pattern cannot be empty");
  return tl_note_(base_, tl_payload_(base_, "Thread::tl().fmt").strftime(pattern), "Thread::tl().fmt");
}

Thread ThreadTimelineAccessor::iso() const {
  return tl_note_(base_, tl_payload_(base_, "Thread::tl().iso").to_note_iso(), "Thread::tl().iso");
}

Thread ThreadTimelineAccessor::epoch(TimeUnit unit) const {
  const auto& t = tl_payload_(base_, "Thread::tl().epoch");
  switch (unit) {
    case TimeUnit::Nanos:
      return tl_series_(base_, t.epoch_nanos(), "Thread::tl().epoch");
    case TimeUnit::Micros:
      return tl_series_(base_, t.epoch_micros(), "Thread::tl().epoch");
    case TimeUnit::Millis:
      return tl_series_(base_, t.epoch_millis(), "Thread::tl().epoch");
    case TimeUnit::Seconds:
      return tl_series_(base_, t.epoch_seconds(), "Thread::tl().epoch");
  }
  PLUSS_FAIL("Thread::tl().epoch: unknown TimeUnit");
}

Thread ThreadTimelineAccessor::floor(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Thread::tl().floor: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().floor").floor(unit));
}

Thread ThreadTimelineAccessor::ceil(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Thread::tl().ceil: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().ceil").ceil(unit));
}

Thread ThreadTimelineAccessor::round(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Thread::tl().round: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().round").round(unit));
}

Thread ThreadTimelineAccessor::date() const {
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().date").date());
}

Thread ThreadTimelineAccessor::time() const {
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().time").time());
}

Thread ThreadTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  PLUSS_CHECK(!freq.empty(), "Thread::tl().shift: freq cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().shift").shift(static_cast<int>(periods), freq));
}

Thread ThreadTimelineAccessor::tz(std::string_view name) const {
  PLUSS_CHECK(!name.empty(), "Thread::tl().tz: timezone cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().tz").tz_convert(name));
}

Thread ThreadTimelineAccessor::localize(const TimelineLocalizeOptions& options) const {
  PLUSS_CHECK(!options.tz.empty(), "Thread::tl().localize: timezone cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Thread::tl().localize")
          .tz_localize(options.tz, options.fold, options.nonexistent));
}

Thread ThreadTimelineAccessor::utc() const {
  return tl_temporal_(base_, tl_payload_(base_, "Thread::tl().utc").to_utc());
}

Thread ThreadTimelineAccessor::index(bool require_sorted) const {
  if (base_.kind() == ThreadKind::TimeIndex) {
    return base_;
  }
  auto out = Thread(base_.as_timeline().to_timeindex(require_sorted), base_.validity_mask());
  if (base_.has_display_labels()) {
    out = out.with_display_labels(base_.display_labels());
  }
  if (base_.has_display_name()) {
    out = out.with_display_name(base_.display_name());
  }
  return out;
}
