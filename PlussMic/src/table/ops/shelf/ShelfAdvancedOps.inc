namespace {

pluss::codem::ScalarType shelf_compute_type_(const pluss::Series& values) {
  return values.platform() == pluss::codem::Platform::CPU
             ? pluss::codem::ScalarType::Float64
             : pluss::codem::ScalarType::Float32;
}

void require_numeric_shelf_(const Shelf& value, std::string_view where) {
  PLUSS_CHECK(
      value.kind() == ShelfKind::Series &&
          is_numeric_series_type(value.as_series()),
      std::string(where) + ": expected a real numeric Series Shelf");
}

pluss::Series shelf_valid_(const Shelf& value) {
  auto valid = value.notnull().as_series();
  if (value.kind() == ShelfKind::Series &&
      valid.platform() != value.as_series().platform()) {
    valid = valid.to(value.as_series().platform());
  }
  return valid;
}

Shelf scalar_shelf_(pluss::Series value, bool valid) {
  auto mask = valid
                  ? pluss::Series::ones(
                        value.sizes(), pluss::codem::ScalarType::Bool,
                        value.platform())
                  : pluss::Series::zeros(
                        value.sizes(), pluss::codem::ScalarType::Bool,
                        value.platform());
  return Shelf(std::move(value), std::move(mask));
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

Shelf running_shelf_(const Shelf& source,
                       bool skip_nulls,
                       int operation,
                       std::string_view where) {
  require_numeric_shelf_(source, where);
  const auto& input = source.as_series();
  const auto dtype = shelf_compute_type_(input);
  auto values = input.to(dtype);
  auto valid = shelf_valid_(source);
  const auto invalid = valid.logical_not();
  pluss::Series result = pluss::series::undefined();
  switch (operation) {
    case 0:
      result = pluss::series::where(invalid, 0.0, values).cumsum(0);
      break;
    case 1:
      result = pluss::series::where(invalid, 1.0, values).cumprod(0);
      break;
    case 2:
      result = pluss::series::where(invalid, std::numeric_limits<double>::infinity(), values).cummin(0);
      break;
    case 3:
      result = pluss::series::where(invalid, -std::numeric_limits<double>::infinity(), values).cummax(0);
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
  return Shelf(result, out_valid);
}

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>
fixed_bounds_(std::int64_t size,
              std::int64_t window,
              const WindowOptions& options) {
  PLUSS_CHECK(window > 0, "Shelf::moving: window must be > 0");
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

Shelf indexed_numeric_window_(
    const Shelf& source,
    const std::vector<std::int64_t>& starts,
    const std::vector<std::int64_t>& ends,
    WindowStatistic statistic,
    const WindowOptions& options,
    std::string_view where) {
  require_numeric_shelf_(source, where);
  PLUSS_CHECK(
      starts.size() == static_cast<std::size_t>(source.size()) &&
          ends.size() == static_cast<std::size_t>(source.size()),
      std::string(where) + ": bounds must match Shelf size");
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
  const auto dtype = shelf_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Shelf(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  const auto values = input.to(dtype);
  const auto valid = shelf_valid_(source);
  const auto prepared = pluss::series::where(valid.logical_not(), 0.0, values);
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
  pluss::Series result = pluss::series::undefined();

  if (statistic == WindowStatistic::Sum) {
    result = sums;
  } else if (statistic == WindowStatistic::Mean) {
    result = sums / counts;
  } else if (statistic == WindowStatistic::Var ||
             statistic == WindowStatistic::Std) {
    const auto origin = prepared.sum() / valid.to(dtype).sum();
    const auto shifted =
        pluss::series::where(valid.logical_not(), 0.0, (values - origin));
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
            .to(pluss::codem::ScalarType::Float64).host<double>();
    const auto cpu_valid =
        valid.to(pluss::codem::Platform::CPU).host<bool>();
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
    return Shelf(cpu_result, cpu_mask);
  }
  return Shelf(result, output_valid);
}

Shelf moving_moment_shelf_(
    const Shelf& source,
    std::int64_t window,
    const WindowOptions& options,
    int operation,
    std::string_view where) {
  require_numeric_shelf_(source, where);
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
  const auto dtype = shelf_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Shelf(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  if (options.closure != WindowClosure::Right) {
    const auto [starts, ends] = fixed_bounds_(n, window, options);
    const auto values = input.to(dtype);
    const auto valid = shelf_valid_(source);
    const auto numeric_valid = valid.to(dtype);
    const auto count_total = numeric_valid.sum();
    const auto origin =
        pluss::series::where(valid.logical_not(), 0.0, values).sum() / count_total;
    const auto shifted =
        pluss::series::where(valid.logical_not(), 0.0, (values - origin));
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
    pluss::Series result = pluss::series::undefined();
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
      result = pluss::series::where(m2.eq(0.0), 0.0, result);
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
      result = pluss::series::where(m2.eq(0.0), 0.0, result);
    }
    return Shelf(
        result, counts.ge(static_cast<double>(minimum)));
  }
  const auto values = input.to(dtype);
  const auto valid = shelf_valid_(source);
  const auto value_padding =
      pluss::Series::zeros({window - 1}, dtype, platform);
  const auto valid_padding = pluss::Series::zeros(
      {window - 1}, pluss::codem::ScalarType::Bool, platform);
  const auto window_values =
      value_padding
          .unite({pluss::series::where(valid.logical_not(), 0.0, values)}, 0, true)
          .unfold(0, window, 1);
  const auto window_valid =
      valid_padding.unite({valid}, 0, true).unfold(0, window, 1);
  const auto counts = window_valid.to(dtype).sum(1);
  const auto means = window_values.sum(1) / counts;
  const auto centered =
      pluss::series::where(window_valid.logical_not(), 0.0, (window_values - means.reshape({n, 1})));
  const auto squares = centered * centered;
  pluss::Series result = pluss::series::undefined();
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
    result = pluss::series::where(m2.eq(0.0), 0.0, result);
  } else {
    const auto m2 = squares.sum(1) / counts;
    const auto m4 = (squares * squares).sum(1) / counts;
    const auto raw = m4 / (m2 * m2) - 3.0;
    result = ((counts - 1.0) /
              ((counts - 2.0) * (counts - 3.0))) *
             ((counts + 1.0) * raw + 6.0);
    result = pluss::series::where(m2.eq(0.0), 0.0, result);
  }
  auto output_valid = counts.ge(static_cast<double>(minimum));
  if (options.centered) {
    const auto offset = (window - 1) / 2;
    if (offset > 0) {
      auto shifted = Shelf(result, output_valid).shift(-offset);
      return shifted;
    }
  }
  return Shelf(result, output_valid);
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

Shelf note_bool_result_(const Shelf& source,
                         const std::vector<std::uint8_t>& values) {
  return Shelf(bool_from_u8_(values), detail::validity_mask(source));
}

// Note predicate/comparison methods return a shape-preserving Bool Note
// (pass 4 of the Note redesign) rather than a raw std::vector<uint8_t>.
Shelf note_bool_result_(const Shelf& source, const pluss::Note& mask) {
  const auto lanes = mask.bools();
  return note_bool_result_(source, std::vector<std::uint8_t>(lanes.begin(), lanes.end()));
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

std::regex compile_regex_(std::string_view pattern,
                          const TextOptions& options,
                          std::string_view where) {
  try {
    auto flags = std::regex::ECMAScript;
    if (!options.case_sensitive) {
      flags |= std::regex::icase;
    }
    return std::regex(std::string(pattern), flags);
  } catch (const std::regex_error& error) {
    PLUSS_FAIL(
        std::string(where) + ": invalid regular expression: " +
        error.what());
  }
}

std::string ascii_fold_(std::string_view value) {
  std::string out(value);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return out;
}

bool literal_has_(std::string_view text,
                  std::string_view needle,
                  bool case_sensitive) {
  if (case_sensitive) {
    return text.find(needle) != std::string_view::npos;
  }
  const auto folded_text = ascii_fold_(text);
  const auto folded_needle = ascii_fold_(needle);
  return folded_text.find(folded_needle) != std::string::npos;
}

std::string replace_literal_(
    std::string_view text,
    std::string_view from,
    std::string_view to,
    const TextOptions& options) {
  if (from.empty()) {
    return std::string(text);
  }
  std::string out(text);
  std::string folded_out = options.case_sensitive ? std::string{} : ascii_fold_(out);
  const auto folded_from = options.case_sensitive ? std::string{} : ascii_fold_(from);
  std::size_t pos = 0;
  std::int64_t replaced = 0;
  while (options.limit < 0 || replaced < options.limit) {
    const auto found =
        options.case_sensitive
            ? out.find(from, pos)
            : folded_out.find(folded_from, pos);
    if (found == std::string::npos) {
      break;
    }
    out.replace(found, from.size(), to);
    if (!options.case_sensitive) {
      folded_out = ascii_fold_(out);
    }
    pos = found + to.size();
    ++replaced;
  }
  return out;
}

std::string replace_regex_limited_(
    std::string_view text,
    const std::regex& expression,
    std::string_view replacement,
    std::int64_t limit) {
  const std::string source(text);
  if (limit < 0) {
    return std::regex_replace(source, expression, std::string(replacement));
  }
  std::string out;
  out.reserve(source.size());
  std::sregex_iterator it(source.begin(), source.end(), expression);
  const std::sregex_iterator end;
  std::size_t last = 0;
  std::int64_t replaced = 0;
  for (; it != end && replaced < limit; ++it, ++replaced) {
    const auto& match = *it;
    out.append(source, last, static_cast<std::size_t>(match.position()) - last);
    out += match.format(std::string(replacement));
    last = static_cast<std::size_t>(match.position() + match.length());
  }
  out.append(source, last, std::string::npos);
  return out;
}

std::vector<std::string> split_literal_limited_(
    std::string_view text,
    std::string_view delimiter,
    std::int64_t limit) {
  PLUSS_CHECK(!delimiter.empty(),
              "Shelf::split: delimiter cannot be empty");
  std::vector<std::string> parts;
  std::size_t pos = 0;
  std::int64_t splits = 0;
  while (limit < 0 || splits < limit) {
    const auto found = text.find(delimiter, pos);
    if (found == std::string_view::npos) {
      break;
    }
    parts.emplace_back(text.substr(pos, found - pos));
    pos = found + delimiter.size();
    ++splits;
  }
  parts.emplace_back(text.substr(pos));
  return parts;
}

std::vector<std::string> split_regex_limited_(
    std::string_view text,
    const std::regex& expression,
    std::int64_t limit) {
  const std::string source(text);
  std::vector<std::string> parts;
  std::sregex_token_iterator it(source.begin(), source.end(), expression, -1);
  const std::sregex_token_iterator end;
  if (limit < 0) {
    for (; it != end; ++it) {
      parts.push_back(it->str());
    }
    return parts;
  }
  std::sregex_iterator match_it(source.begin(), source.end(), expression);
  const std::sregex_iterator match_end;
  std::size_t pos = 0;
  std::int64_t splits = 0;
  for (; match_it != match_end && splits < limit; ++match_it, ++splits) {
    const auto& match = *match_it;
    parts.emplace_back(source.substr(pos, static_cast<std::size_t>(match.position()) - pos));
    pos = static_cast<std::size_t>(match.position() + match.length());
  }
  parts.emplace_back(source.substr(pos));
  return parts;
}

std::int64_t count_literal_(
    std::string_view text,
    std::string_view needle,
    const TextOptions& options) {
  if (needle.empty()) {
    return 0;
  }
  const auto source = options.case_sensitive ? std::string(text) : ascii_fold_(text);
  const auto target = options.case_sensitive ? std::string(needle) : ascii_fold_(needle);
  std::int64_t count = 0;
  std::size_t pos = 0;
  while (options.limit < 0 || count < options.limit) {
    const auto found = source.find(target, pos);
    if (found == std::string::npos) {
      break;
    }
    ++count;
    pos = found + target.size();
  }
  return count;
}

std::int64_t count_regex_(
    std::string_view text,
    const std::regex& expression,
    std::int64_t limit) {
  const std::string source(text);
  std::sregex_iterator it(source.begin(), source.end(), expression);
  const std::sregex_iterator end;
  std::int64_t count = 0;
  for (; it != end && (limit < 0 || count < limit); ++it) {
    ++count;
  }
  return count;
}

std::vector<std::string> findall_literal_(
    std::string_view text,
    std::string_view needle,
    const TextOptions& options) {
  std::vector<std::string> matches;
  if (needle.empty()) {
    return matches;
  }
  const auto source = options.case_sensitive ? std::string(text) : ascii_fold_(text);
  const auto target = options.case_sensitive ? std::string(needle) : ascii_fold_(needle);
  std::size_t pos = 0;
  while (options.limit < 0 ||
         static_cast<std::int64_t>(matches.size()) < options.limit) {
    const auto found = source.find(target, pos);
    if (found == std::string::npos) {
      break;
    }
    matches.emplace_back(text.substr(found, needle.size()));
    pos = found + target.size();
  }
  return matches;
}

std::vector<std::string> findall_regex_(
    std::string_view text,
    const std::regex& expression,
    std::int64_t limit) {
  const std::string source(text);
  std::vector<std::string> matches;
  std::sregex_iterator it(source.begin(), source.end(), expression);
  const std::sregex_iterator end;
  for (; it != end &&
         (limit < 0 || static_cast<std::int64_t>(matches.size()) < limit);
       ++it) {
    matches.push_back(it->str());
  }
  return matches;
}

} // namespace

ShelfValue Shelf::covar(
    const Shelf& other,
    const StatisticOptions& options) const {
  require_numeric_shelf_(*this, "Shelf::covar");
  require_numeric_shelf_(other, "Shelf::covar");
  const auto aligned = align_shelf_pair_for_cases_(*this, other, "Shelf::covar");
  if (aligned.aligned) {
    return aligned.left.covar(aligned.right, options);
  }
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Shelf::covar: ddof and min_count must be >= 0");
  const auto platform = as_series().platform();
  PLUSS_CHECK(other.as_series().platform() == platform,
              "Shelf::covar: operands must share a platform");
  const auto dtype = shelf_compute_type_(as_series());
  const auto left_valid = shelf_valid_(*this);
  const auto right_valid = shelf_valid_(other);
  const auto valid = left_valid.logical_and(right_valid);
  const auto numeric_valid = valid.to(dtype);
  const auto count = numeric_valid.sum();
  const auto left = as_series().to(dtype);
  const auto right = other.as_series().to(dtype);
  const auto left_origin =
      pluss::series::where(valid.logical_not(), 0.0, left).sum() / count;
  const auto right_origin =
      pluss::series::where(valid.logical_not(), 0.0, right).sum() / count;
  const auto centered_left =
      pluss::series::where(valid.logical_not(), 0.0, (left - left_origin));
  const auto centered_right =
      pluss::series::where(valid.logical_not(), 0.0, (right - right_origin));
  const auto value =
      (centered_left * centered_right).sum() /
      (count - static_cast<double>(options.ddof));
  const auto minimum = std::max(options.min_count, options.ddof + 1);
  return ShelfValue(Shelf(
      value.reshape({1}),
      count.ge(static_cast<double>(minimum)).reshape({1})));
}

Shelf Shelf::percentiles(
    pluss::msharp::RangeView::Double points,
    std::int64_t min_count) const {
  require_numeric_shelf_(*this, "Shelf::percentiles");
  PLUSS_CHECK(!points.empty(),
              "Shelf::percentiles: expected at least one point");
  PLUSS_CHECK(min_count >= 0,
              "Shelf::percentiles: min_count must be >= 0");
  auto clean = dropnull().as_series();
  // quantile requires a floating dtype; integer/bool shelves interpolate in
  // the compute type (matching pandas, where integer percentiles are float).
  if (!clean.is_floating_point()) {
    clean = clean.to(shelf_compute_type_(as_series()));
  }
  std::vector<pluss::Series> values;
  values.reserve(points.size());
  for (const auto point : points) {
    PLUSS_CHECK(point >= 0.0 && point <= 1.0,
                "Shelf::percentiles: points must be in [0, 1]");
    auto value =
        clean.numel() == 0
            ? pluss::Series::zeros(
                  {1}, shelf_compute_type_(as_series()),
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
      pluss::series::pile(values, 0).reshape(
          {static_cast<std::int64_t>(points.size())});
  const auto valid =
      clean.numel() >= min_count
          ? pluss::Series::ones(
                packed.sizes(), pluss::codem::ScalarType::Bool,
                packed.platform())
          : pluss::Series::zeros(
                packed.sizes(), pluss::codem::ScalarType::Bool,
                packed.platform());
  return Shelf(packed, valid);
}

Shelf Shelf::mode(bool drop_nulls) const {
  PLUSS_CHECK(
      kind() == ShelfKind::Series || kind() == ShelfKind::Note ||
          kind() == ShelfKind::Timeline ||
          kind() == ShelfKind::Duration,
      "Shelf::mode: unsupported payload");
  const auto validity = notnull().as_series().host<bool>();
  std::int64_t null_count = 0;
  std::vector<bool> output_valid;
  if (kind() == ShelfKind::Series) {
    require_numeric_shelf_(*this, "Shelf::mode");
    const auto values =
        as_series().to(pluss::codem::Platform::CPU)
            .to(pluss::codem::ScalarType::Float64).host<double>();
    std::map<double, std::int64_t> counts;
    for (std::size_t i = 0; i < values.size(); ++i) {
      validity[i] ? ++counts[values[i]] : ++null_count;
    }
    std::int64_t best = drop_nulls ? 0 : null_count;
    for (const auto& [value, count] : counts) {
      (void)value;
      best = std::max(best, count);
    }
    PLUSS_CHECK(best > 0, "Shelf::mode: no values");
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
    return Shelf(out, mask);
  }

  if (kind() == ShelfKind::Timeline ||
      kind() == ShelfKind::Duration) {
    const auto ticks = kind() == ShelfKind::Timeline
                           ? as_timeline().ticks()
                           : as_duration().host();
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
    PLUSS_CHECK(best > 0, "Shelf::mode: no values");
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
  PLUSS_CHECK(best > 0, "Shelf::mode: no values");
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
  return Shelf(pluss::make_note(modes),
                bool_series_from_bools(output_valid));
}

Shelf Shelf::rank(
    RankTieRule ties,
    RankNullRule nulls,
    bool ascending,
    bool percentage) const {
  require_numeric_shelf_(*this, "Shelf::rank");
  const auto n = size();
  auto values = as_series().to(pluss::codem::Platform::CPU)
                    .to(pluss::codem::ScalarType::Float64).host<double>();
  auto valid = notnull().as_series().host<bool>();
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
  return Shelf(output, output_valid);
}

ShelfValue Shelf::skew(std::int64_t min_count) const {
  require_numeric_shelf_(*this, "Shelf::skew");
  const auto dtype = shelf_compute_type_(as_series());
  const auto valid = shelf_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values =
      pluss::series::where(valid.logical_not(), 0.0, as_series().to(dtype));
  const auto mean = values.sum() / count;
  const auto centered =
      pluss::series::where(valid.logical_not(), 0.0, (as_series().to(dtype) - mean));
  const auto m2 = (centered * centered).sum() / count;
  const auto m3 = (centered * centered * centered).sum() / count;
  auto result =
      (m3 / (m2 * m2.sqrt())) *
      ((count * (count - 1.0)).sqrt() / (count - 2.0));
  result = pluss::series::where(m2.eq(0.0), 0.0, result);
  return ShelfValue(Shelf(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max<std::int64_t>(3, min_count))).reshape({1})));
}

ShelfValue Shelf::kurtosis(std::int64_t min_count) const {
  require_numeric_shelf_(*this, "Shelf::kurtosis");
  const auto dtype = shelf_compute_type_(as_series());
  const auto valid = shelf_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values =
      pluss::series::where(valid.logical_not(), 0.0, as_series().to(dtype));
  const auto mean = values.sum() / count;
  const auto centered =
      pluss::series::where(valid.logical_not(), 0.0, (as_series().to(dtype) - mean));
  const auto square = centered * centered;
  const auto m2 = square.sum() / count;
  const auto m4 = (square * square).sum() / count;
  const auto raw = m4 / (m2 * m2) - 3.0;
  auto result =
      ((count - 1.0) / ((count - 2.0) * (count - 3.0))) *
      ((count + 1.0) * raw + 6.0);
  result = pluss::series::where(m2.eq(0.0), 0.0, result);
  return ShelfValue(Shelf(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max<std::int64_t>(4, min_count))).reshape({1})));
}

ShelfValue Shelf::var(const StatisticOptions& options) const {
  require_numeric_shelf_(*this, "Shelf::var");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Shelf::var: ddof and min_count must be >= 0");
  const auto dtype = shelf_compute_type_(as_series());
  const auto valid = shelf_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values = as_series().to(dtype);
  const auto origin =
      pluss::series::where(valid.logical_not(), 0.0, values).sum() / count;
  const auto centered =
      pluss::series::where(valid.logical_not(), 0.0, (values - origin));
  const auto result =
      (centered * centered).sum() /
      (count - static_cast<double>(options.ddof));
  return ShelfValue(Shelf(
      result.clamp_min(0.0).reshape({1}),
      count.ge(static_cast<double>(
          std::max(options.min_count, options.ddof + 1))).reshape({1})));
}

ShelfValue Shelf::std(const StatisticOptions& options) const {
  const auto result = var(options);
  return ShelfValue(Shelf(
      result.as_series().clamp_min(0.0).sqrt(),
      result.shelf().validity_mask()));
}

ShelfValue Shelf::se(ErrorTarget target, const StatisticOptions& options) const {
  PLUSS_CHECK(target == ErrorTarget::Mean,
              "Shelf::se: only ErrorTarget::Mean is implemented in this pass");
  require_numeric_shelf_(*this, "Shelf::se");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Shelf::se: ddof and min_count must be >= 0");
  const auto dtype = shelf_compute_type_(as_series());
  const auto valid = shelf_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values =
      pluss::series::where(valid.logical_not(), 0.0, as_series().to(dtype));
  const auto mean = values.sum() / count;
  const auto centered =
      pluss::series::where(valid.logical_not(), 0.0, (as_series().to(dtype) - mean));
  const auto variance =
      (centered * centered).sum() /
      (count - static_cast<double>(options.ddof));
  const auto result = variance.sqrt() / count.sqrt();
  return ShelfValue(Shelf(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max(options.min_count, options.ddof + 1))).reshape({1})));
}

Shelf Shelf::scan(ScanStatistic statistic, bool skip_nulls) const {
  switch (statistic) {
    case ScanStatistic::Sum:
      return running_shelf_(*this, skip_nulls, 0, "Shelf::scan");
    case ScanStatistic::Product:
      return running_shelf_(*this, skip_nulls, 1, "Shelf::scan");
    case ScanStatistic::Minimum:
      return running_shelf_(*this, skip_nulls, 2, "Shelf::scan");
    case ScanStatistic::Maximum:
      return running_shelf_(*this, skip_nulls, 3, "Shelf::scan");
  }
  PLUSS_FAIL("Shelf::scan: unsupported statistic");
}

Shelf Shelf::cumsum(bool skip_nulls) const {
  return scan(ScanStatistic::Sum, skip_nulls);
}

Shelf Shelf::cumprod(bool skip_nulls) const {
  return scan(ScanStatistic::Product, skip_nulls);
}

Shelf Shelf::cummin(bool skip_nulls) const {
  return scan(ScanStatistic::Minimum, skip_nulls);
}

Shelf Shelf::cummax(bool skip_nulls) const {
  return scan(ScanStatistic::Maximum, skip_nulls);
}

ShelfValue Shelf::corr_with(
    const Shelf& other,
    ShelfCorrMethod method,
    std::int64_t min_count) const {
  require_numeric_shelf_(*this, "Shelf::corr_with");
  require_numeric_shelf_(other, "Shelf::corr_with");
  const auto aligned = align_shelf_pair_for_cases_(*this, other, "Shelf::corr_with");
  if (aligned.aligned) {
    return aligned.left.corr_with(aligned.right, method, min_count);
  }
  PLUSS_CHECK(min_count >= 1,
              "Shelf::corr_with: min_count must be >= 1");
  PLUSS_CHECK(as_series().platform() == other.as_series().platform(),
              "Shelf::corr_with: operands must share a platform");
  const auto valid =
      notnull().as_series().logical_and(other.notnull().as_series());
  if (method == ShelfCorrMethod::Pearson) {
    const auto dtype = shelf_compute_type_(as_series());
    const auto count = valid.to(dtype).sum();
    const auto left = as_series().to(dtype);
    const auto right = other.as_series().to(dtype);
    const auto left_origin =
        pluss::series::where(valid.logical_not(), 0.0, left).sum() / count;
    const auto right_origin =
        pluss::series::where(valid.logical_not(), 0.0, right).sum() / count;
    const auto centered_left =
        pluss::series::where(valid.logical_not(), 0.0, (left - left_origin));
    const auto centered_right =
        pluss::series::where(valid.logical_not(), 0.0, (right - right_origin));
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
    return ShelfValue(Shelf(
        (numerator / (left_square * right_square).sqrt())
            .clamp(-1.0, 1.0).reshape({1}),
        output_valid.reshape({1})));
  }
  const auto count =
      valid.to(pluss::codem::ScalarType::Int64).sum().item<std::int64_t>();
  auto value = pluss::Series::zeros(
      {1}, shelf_compute_type_(as_series()), as_series().platform());
  bool has_value = false;
  if (count >= min_count) {
    const auto corr_value = corr(other, method);
    has_value = std::isfinite(corr_value);
    value = pluss::Series::full(
        {1}, corr_value, shelf_compute_type_(as_series()),
        as_series().platform());
  }
  return ShelfValue(scalar_shelf_(value, has_value));
}

ShelfValue Shelf::matmul(const Shelf& other) const {
  require_numeric_shelf_(*this, "Shelf::matmul");
  require_numeric_shelf_(other, "Shelf::matmul");
  const auto aligned = align_shelf_pair_for_cases_(*this, other, "Shelf::matmul");
  if (aligned.aligned) {
    return aligned.left.matmul(aligned.right);
  }
  PLUSS_CHECK(!has_mask() && !other.has_mask(),
              "Shelf::matmul: null values are not supported");
  return ShelfValue(Shelf(as_series().matmul(other.as_series()).reshape({1})));
}

Shelf Shelf::bounded(
    const WindowBoundsFn& bounds,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  PLUSS_CHECK(static_cast<bool>(bounds),
              "Shelf::bounded: bounds callback is empty");
  auto [starts, ends] = bounds(size(), options);
  return indexed_numeric_window_(
      *this, starts, ends, statistic, options,
      "Shelf::bounded");
}

Shelf Shelf::moving(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  if (statistic == WindowStatistic::Sem) {
    return moving_moment_shelf_(*this, window, options, 0, "Shelf::moving");
  }
  if (statistic == WindowStatistic::Skew) {
    return moving_moment_shelf_(*this, window, options, 1, "Shelf::moving");
  }
  if (statistic == WindowStatistic::Kurtosis) {
    return moving_moment_shelf_(*this, window, options, 2, "Shelf::moving");
  }
  auto [starts, ends] = fixed_bounds_(size(), window, options);
  return indexed_numeric_window_(
      *this, starts, ends, statistic, options, "Shelf::moving");
}

Shelf Shelf::growing(
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

Shelf exponential_mean_shelf_(
    const Shelf& source,
    double alpha,
    bool adjusted,
    std::int64_t min_count,
    bool ignore_nulls) {
  require_numeric_shelf_(source, "Shelf::decay");
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0,
              "Shelf::decay: alpha must be in (0, 1]");
  PLUSS_CHECK(min_count > 0,
              "Shelf::decay: min_count must be > 0");
  const auto dtype = shelf_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  const auto values = source.as_series().to(dtype);
  const auto valid = shelf_valid_(source);
  const auto count = valid.to(dtype).cumsum(0);
  if (!adjusted) {
    PLUSS_CHECK(
        !detail::has_mask(source),
        "Shelf::decay(adjusted=false): nullable input requires "
        "adjusted=true");
    auto result = ts_result_shelf_(pluss::subsp::stats::ts::ema(values, alpha));
    return detail::with_mask(result,
        count.ge(static_cast<double>(min_count)));
  }
  if (alpha == 1.0) {
    return Shelf(values, valid.logical_and(
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
      (pluss::series::where(valid.logical_not(), 0.0, values) * inverse_weight).cumsum(0);
  const auto denominator =
      (valid.to(dtype) * inverse_weight).cumsum(0);
  return Shelf(
      numerator / denominator,
      count.ge(static_cast<double>(min_count)));
}

Shelf exponential_variance_shelf_(
    const Shelf& source,
    double alpha,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) {
  require_numeric_shelf_(source, "Shelf::decay");
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0,
              "Shelf::decay: alpha must be in (0, 1]");
  const auto dtype = shelf_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  const auto values = source.as_series().to(dtype);
  const auto valid = shelf_valid_(source);
  const auto count = valid.to(dtype).cumsum(0);
  const auto origin =
      pluss::series::where(valid.logical_not(), 0.0, values).sum() /
      valid.to(dtype).sum();
  const auto centered =
      pluss::series::where(valid.logical_not(), 0.0, (values - origin));
  const auto centered_shelf =
      Shelf(centered, detail::validity_mask(source));
  const auto mean = exponential_mean_shelf_(centered_shelf,
      alpha, adjusted, 1, ignore_nulls).as_series();
  pluss::Series second = pluss::series::undefined();
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
        !detail::has_mask(source),
        "Shelf::decay(adjusted=false): nullable input "
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
  return Shelf(result, output_valid);
}

Shelf Shelf::decay(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  switch (statistic) {
    case WindowStatistic::Mean:
      return exponential_mean_shelf_(*this, alpha, adjusted, min_count, ignore_nulls);
    case WindowStatistic::Var:
      return exponential_variance_shelf_(
          *this, alpha, adjusted, bias_corrected, min_count, ignore_nulls);
    case WindowStatistic::Std: {
      const auto variance = exponential_variance_shelf_(
          *this, alpha, adjusted, bias_corrected, min_count, ignore_nulls);
      return Shelf(variance.as_series().sqrt(), variance.validity_mask());
    }
    default:
      PLUSS_FAIL("Shelf::decay: only Mean, Var, and Std are supported");
  }
}

Shelf Shelf::weigh(
    pluss::msharp::RangeView::Double weights,
    WindowStatistic statistic,
    std::int64_t min_count) const {
  require_numeric_shelf_(*this, "Shelf::weigh");
  PLUSS_CHECK(!weights.empty(),
              "Shelf::weigh: weights cannot be empty");
  PLUSS_CHECK(min_count > 0 &&
                  min_count <= static_cast<std::int64_t>(weights.size()),
              "Shelf::weigh: invalid min_count");
  if (statistic == WindowStatistic::Minimum ||
      statistic == WindowStatistic::Maximum) {
    return moving(
        static_cast<std::int64_t>(weights.size()), statistic,
        WindowOptions{.min_count = min_count});
  }
  const auto n = size();
  const auto window = static_cast<std::int64_t>(weights.size());
  const auto dtype = shelf_compute_type_(as_series());
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
  const auto valid = shelf_valid_(*this);
  const auto padded_values =
      pluss::Series::zeros({window - 1}, dtype, platform)
          .unite({pluss::series::where(valid.logical_not(), 0.0, values)}, 0, true)
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
  pluss::Series result = pluss::series::undefined();
  if (statistic == WindowStatistic::Sum) {
    result = weighted_sum;
  } else if (statistic == WindowStatistic::Mean) {
    result = weighted_sum / weight_sum;
  } else {
    const auto mean = weighted_sum / weight_sum;
    const auto centered =
        pluss::series::where(padded_valid.logical_not(), 0.0, (padded_values - mean.reshape({n, 1})));
    auto variance =
        (centered * centered * active_weights).sum(1) / weight_sum;
    variance = variance.clamp_min(0.0);
    result = statistic == WindowStatistic::Std
                 ? variance.sqrt()
                 : variance;
  }
  return Shelf(
      result, count.ge(static_cast<double>(min_count)));
}

Shelf Shelf::timed(
    const Shelf& axis,
    std::int64_t duration_nanos,
    TimeWindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  PLUSS_CHECK(axis.size() == size(),
              "Shelf::timed: axis length mismatch");
  PLUSS_CHECK(duration_nanos > 0,
              "Shelf::timed: duration_nanos must be > 0");
  PLUSS_CHECK(
      axis.notnull().as_series().all().item<bool>(),
      "Shelf::timed: axis cannot contain null values");
  std::vector<std::int64_t> ticks;
  if (axis.kind() == ShelfKind::Timeline) {
    ticks = axis.as_timeline().ticks();
  } else if (axis.kind() == ShelfKind::Series &&
             axis.as_series().scalar_type() ==
                 pluss::codem::ScalarType::Int64) {
    ticks = axis.as_series().to(
        pluss::codem::Platform::CPU).host<std::int64_t>();
  } else {
    PLUSS_FAIL(
        "Shelf::timed: axis must be Timeline or Int64 "
        "nanoseconds");
  }
  PLUSS_CHECK(std::is_sorted(ticks.begin(), ticks.end()),
              "Shelf::timed: axis must be monotonic");
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
      "Shelf::timed");
}

Shelf Shelf::grouped(
    const Shelf& groups,
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  require_numeric_shelf_(*this, "Shelf::grouped");
  PLUSS_CHECK(groups.size() == size(),
              "Shelf::grouped: group length mismatch");
  Shelf cpu_groups = groups;
  if (groups.kind() == ShelfKind::Series &&
      groups.as_series().platform() != pluss::codem::Platform::CPU) {
    cpu_groups = Shelf(
        groups.as_series().to(pluss::codem::Platform::CPU),
        groups.validity_mask());
  }
  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_group;
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto key =
        groups.is_valid(row)
            ? std::string("\1") + shelf_scalar_string_at(cpu_groups, row)
            : std::string(1, '\0');
    rows_by_group[key].push_back(row);
  }
  const auto dtype = shelf_compute_type_(as_series());
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
  return Shelf(output, output_valid);
}

Shelf Shelf::flow(
    const std::function<Shelf(const Shelf&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Shelf::flow: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Shelf::flow: callback returned an undefined Shelf");
  return result;
}

Shelf Shelf::fold(
    const std::function<Shelf(const Shelf&)>& fn,
    ResultShape shape) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Shelf::fold: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Shelf::fold: callback returned undefined");
  if (shape == ResultShape::Reduce) {
    PLUSS_CHECK(result.size() == 1,
                "Shelf::fold: Reduce requires one value");
  } else if (shape == ResultShape::Preserve) {
    PLUSS_CHECK(result.size() == size(),
                "Shelf::fold: Preserve requires input length");
  }
  return result;
}

Shelf Shelf::transform(
    const std::function<ShelfValue(const ShelfValue&)>& fn) const {
  return map(fn);
}

Shelf Shelf::transform(
    const std::function<Shelf(const Shelf&)>& fn,
    ResultShape shape) const {
  return fold(fn, shape);
}

Shelf Shelf::map(
    const std::function<ShelfValue(const ShelfValue&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Shelf::map: callback is empty");
  std::vector<std::optional<Shelf>> mapped_values(
      static_cast<std::size_t>(size()));
  std::optional<Shelf> prototype;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    const std::array<std::int64_t, 1> index{row};
    auto mapped = fn(ShelfValue(take(index)));
    PLUSS_CHECK(mapped.defined() && mapped.shelf().size() == 1,
                "Shelf::map: callback must return one value");
    if (prototype.has_value()) {
      PLUSS_CHECK(mapped.kind() == prototype->kind(),
                  "Shelf::map: output kinds must be consistent");
    }
    prototype = mapped.shelf();
    mapped_values[static_cast<std::size_t>(row)] = mapped.shelf();
  }
  if (!prototype.has_value()) {
    return *this;
  }
  auto null_mask = pluss::Series::zeros(
      {1}, pluss::codem::ScalarType::Bool);
  if (prototype->kind() == ShelfKind::Series &&
      prototype->as_series().platform() !=
          pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(prototype->as_series().platform());
  } else if (prototype->kind() == ShelfKind::Timeline &&
             prototype->as_timeline().epoch_nanos().platform() !=
                 pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(
        prototype->as_timeline().epoch_nanos().platform());
  }
  std::vector<Shelf> values;
  values.reserve(mapped_values.size());
  for (auto& value : mapped_values) {
    values.push_back(
        value.has_value()
            ? std::move(*value)
            : prototype->with_mask(null_mask));
  }
  const auto kind = values.front().kind();
  std::vector<bool> validity;
  validity.reserve(values.size());
  if (kind == ShelfKind::Series) {
    std::vector<pluss::Series> payloads;
    payloads.reserve(values.size());
    for (const auto& value : values) {
      payloads.push_back(value.as_series());
      validity.push_back(detail::valid_at(value, 0));
    }
    auto output = pluss::series::unite(payloads, 0, true);
    auto mask = bool_series_from_bools(validity);
    if (output.platform() != pluss::codem::Platform::CPU) {
      mask = mask.to(output.platform());
    }
    return Shelf(output, mask);
  }
  if (kind == ShelfKind::Note) {
    std::vector<std::string> payloads;
    payloads.reserve(values.size());
    for (const auto& value : values) {
      payloads.emplace_back(value.as_note().at(0));
      validity.push_back(detail::valid_at(value, 0));
    }
    return Shelf(pluss::make_note(payloads),
                  bool_series_from_bools(validity));
  }
  if (kind == ShelfKind::Timeline) {
    auto output = values.front().as_timeline();
    for (std::size_t i = 1; i < values.size(); ++i) {
      output = output.concat(values[i].as_timeline());
    }
    for (const auto& value : values) {
      validity.push_back(detail::valid_at(value, 0));
    }
    auto mask = bool_series_from_bools(validity);
    if (output.epoch_nanos().platform() !=
        pluss::codem::Platform::CPU) {
      mask = mask.to(output.epoch_nanos().platform());
    }
    return Shelf(output, mask);
  }
  PLUSS_CHECK(kind == ShelfKind::Duration,
              "Shelf::map: unsupported mapped output kind");
  std::vector<std::int64_t> nanos;
  nanos.reserve(values.size());
  for (const auto& value : values) {
    nanos.push_back(value.as_duration().host().front());
    validity.push_back(detail::valid_at(value, 0));
  }
  return Shelf(pluss::duration::nanos(nanos), bool_series_from_bools(validity));
}

Shelf Shelf::upper() const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::upper: expected Note payload");
  return Shelf(as_note().upper(), validity_mask());
}

Shelf Shelf::casefold() const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::casefold: expected Note payload");
  return Shelf(as_note().casefold(), validity_mask());
}

Shelf Shelf::titlecase() const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::titlecase: expected Note payload");
  return Shelf(as_note().title(), validity_mask());
}

Shelf Shelf::length() const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::length: expected Note payload");
  const auto measured = as_note().lengths_chars();
  const auto lengths = measured.integers();
  auto output = pluss::Series::empty(
      {static_cast<std::int64_t>(lengths.size())},
      pluss::codem::ScalarType::Int64);
  std::copy(lengths.begin(), lengths.end(),
            reinterpret_cast<std::int64_t*>(output.raw_data()));
  return Shelf(output, validity_mask());
}

Shelf Shelf::starts(std::string_view prefix) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::starts: expected Note payload");
  return note_bool_result_(*this, as_note().starts_with(prefix));
}

Shelf Shelf::ends(std::string_view suffix) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::ends: expected Note payload");
  return note_bool_result_(*this, as_note().ends_with(suffix));
}

Shelf note_has_(const Shelf& source,
                 std::string_view needle,
                 const TextOptions& options) {
  PLUSS_CHECK(source.kind() == ShelfKind::Note,
              "Shelf::nt().has: expected Note payload");
  if (options.regex) {
    if (options.case_sensitive) {
      (void)compile_regex_(needle, "Shelf::nt().has");
      return note_bool_result_(source, source.as_note().regex().search(needle));
    }
    const auto expression = compile_regex_(needle, options, "Shelf::nt().has");
    std::vector<std::uint8_t> values;
    values.reserve(static_cast<std::size_t>(source.size()));
    for (std::int64_t row = 0; row < source.size(); ++row) {
      const auto text = std::string(source.as_note().at(row));
      values.push_back(
          detail::valid_at(source, row) && std::regex_search(text, expression) ? 1U : 0U);
    }
    return note_bool_result_(source, values);
  }
  if (options.case_sensitive) {
    return note_bool_result_(source, source.as_note().contains(needle));
  }
  std::vector<std::uint8_t> values;
  values.reserve(static_cast<std::size_t>(source.size()));
  for (std::int64_t row = 0; row < source.size(); ++row) {
    values.push_back(
        detail::valid_at(source, row) && literal_has_(source.as_note().at(row), needle, false)
            ? 1U
            : 0U);
  }
  return note_bool_result_(source, values);
}

Shelf note_count_(const Shelf& source,
                   std::string_view pattern,
                   const TextOptions& options) {
  PLUSS_CHECK(source.kind() == ShelfKind::Note,
              "Shelf::nt().count: expected Note payload");
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(pattern, options, "Shelf::nt().count");
  }
  auto output = pluss::Series::empty(
      {source.size()}, pluss::codem::ScalarType::Int64);
  auto* data = reinterpret_cast<std::int64_t*>(output.raw_data());
  for (std::int64_t row = 0; row < source.size(); ++row) {
    data[row] =
        detail::valid_at(source, row)
            ? (options.regex
                   ? count_regex_(source.as_note().at(row), *expression, options.limit)
                   : count_literal_(source.as_note().at(row), pattern, options))
            : 0;
  }
  return Shelf(output, detail::validity_mask(source));
}

DataFrame note_findall_(const Shelf& source,
                        std::string_view pattern,
                        const TextOptions& options) {
  PLUSS_CHECK(source.kind() == ShelfKind::Note,
              "Shelf::nt().findall: expected Note payload");
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(pattern, options, "Shelf::nt().findall");
  }
  std::vector<std::vector<std::string>> rows;
  rows.reserve(static_cast<std::size_t>(source.size()));
  std::int64_t width = 0;
  for (std::int64_t row = 0; row < source.size(); ++row) {
    std::vector<std::string> matches;
    if (detail::valid_at(source, row)) {
      matches =
          options.regex
              ? findall_regex_(source.as_note().at(row), *expression, options.limit)
              : findall_literal_(source.as_note().at(row), pattern, options);
    }
    width = std::max<std::int64_t>(width, static_cast<std::int64_t>(matches.size()));
    rows.push_back(std::move(matches));
  }
  std::vector<std::pair<std::string, Shelf>> output;
  output.reserve(static_cast<std::size_t>(width));
  for (std::int64_t match = 0; match < width; ++match) {
    std::vector<std::string> values(static_cast<std::size_t>(source.size()));
    std::vector<bool> valid(static_cast<std::size_t>(source.size()), false);
    for (std::int64_t row = 0; row < source.size(); ++row) {
      const auto& row_matches = rows[static_cast<std::size_t>(row)];
      if (match < static_cast<std::int64_t>(row_matches.size())) {
        values[static_cast<std::size_t>(row)] =
            row_matches[static_cast<std::size_t>(match)];
        valid[static_cast<std::size_t>(row)] = true;
      }
    }
    output.emplace_back(
        "match_" + std::to_string(match),
        Shelf(pluss::make_note(values), bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

Shelf Shelf::matches(std::string_view pattern) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::matches: expected Note payload");
  (void)compile_regex_(pattern, "Shelf::matches");
  return note_bool_result_(*this, as_note().regex().search(pattern));
}

Shelf Shelf::fullmatch(std::string_view pattern) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::fullmatch: expected Note payload");
  (void)compile_regex_(pattern, "Shelf::fullmatch");
  return note_bool_result_(*this, as_note().regex().match(pattern));
}

DataFrame Shelf::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::extract: expected Note payload");
  auto expression =
      compile_regex_(pattern, "Shelf::extract");
  const auto group_count = expression.mark_count();
  PLUSS_CHECK(group_count > 0,
              "Shelf::extract: pattern needs a capture group");
  PLUSS_CHECK(names.empty() || names.size() == group_count,
              "Shelf::extract: names must match capture groups");
  std::vector<std::vector<std::string>> columns(group_count);
  std::vector<std::vector<bool>> valid(group_count);
  for (auto& column : columns) {
    column.reserve(static_cast<std::size_t>(size()));
  }
  for (auto& mask : valid) {
    mask.reserve(static_cast<std::size_t>(size()));
  }
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto text = as_note().at(row);
    std::match_results<decltype(text.begin())> match;
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
  std::vector<std::pair<std::string, Shelf>> output;
  output.reserve(group_count);
  for (std::size_t group = 0; group < group_count; ++group) {
    const auto name =
        names.empty() ? "group_" + std::to_string(group)
                      : std::string(names[group]);
    output.emplace_back(
        name,
        Shelf(pluss::make_note(columns[group]),
               bool_series_from_bools(valid[group])));
  }
  return DataFrame(std::move(output));
}

DataFrame Shelf::framerize(std::string_view name) const {
  std::string column = name.empty()
                           ? (named() ? this->name() : std::string("x"))
                           : std::string(name);

  const bool indexed = detail::has_labels(*this);
  std::optional<Shelf> cases =
      indexed ? std::optional<Shelf>(detail::display_labels(*this)) : std::nullopt;

  // The column carries values and validity only; its own name and index are
  // dropped -- the name is replaced by `column`, the index becomes the frame's
  // cases.
  Shelf body = indexed ? detail::clear_labels(*this) : *this;
  if (body.named()) {
    body = body.unname();
  }

  std::vector<std::pair<std::string, Shelf>> output;
  output.emplace_back(std::move(column), std::move(body));
  return DataFrame(std::move(output), std::move(cases));
}

DataFrame Shelf::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::split: expected Note payload");
  const auto batch =
      limit < 0 ? as_note().split(delimiter)
                : as_note().split(delimiter, limit);
  const auto offsets = batch.offsets().integers();
  const auto tokens = batch.tokens().vector();
  std::int64_t width = 0;
  for (std::size_t row = 0; row + 1 < offsets.size(); ++row) {
    width = std::max(width, offsets[row + 1] - offsets[row]);
  }
  std::vector<std::pair<std::string, Shelf>> output;
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
        Shelf(pluss::make_note(values),
               bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

DataFrame Shelf::split(
    std::string_view pattern,
    const TextOptions& options) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::split: expected Note payload");
  std::vector<std::vector<std::string>> rows;
  rows.reserve(static_cast<std::size_t>(size()));
  std::int64_t width = 0;
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(pattern, options, "Shelf::split");
  }
  for (std::int64_t row = 0; row < size(); ++row) {
    std::vector<std::string> parts;
    if (is_valid(row)) {
      parts =
          options.regex
              ? split_regex_limited_(as_note().at(row), *expression, options.limit)
              : split_literal_limited_(as_note().at(row), pattern, options.limit);
    }
    width = std::max<std::int64_t>(width, static_cast<std::int64_t>(parts.size()));
    rows.push_back(std::move(parts));
  }
  std::vector<std::pair<std::string, Shelf>> output;
  output.reserve(static_cast<std::size_t>(width));
  for (std::int64_t part = 0; part < width; ++part) {
    std::vector<std::string> values(static_cast<std::size_t>(size()));
    std::vector<bool> valid(static_cast<std::size_t>(size()), false);
    for (std::int64_t row = 0; row < size(); ++row) {
      const auto& row_parts = rows[static_cast<std::size_t>(row)];
      if (part < static_cast<std::int64_t>(row_parts.size())) {
        values[static_cast<std::size_t>(row)] =
            row_parts[static_cast<std::size_t>(part)];
        valid[static_cast<std::size_t>(row)] = true;
      }
    }
    output.emplace_back(
        options.prefix + "_" + std::to_string(part),
        Shelf(pluss::make_note(values),
               bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

DataFrame Shelf::partition(
    std::string_view separator,
    std::string prefix) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::partition: expected Note payload");
  PLUSS_CHECK(!separator.empty(),
              "Shelf::partition: separator cannot be empty");
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
  std::vector<std::pair<std::string, Shelf>> output;
  for (std::size_t part = 0; part < values.size(); ++part) {
    output.emplace_back(
        prefix + "_" + std::to_string(part),
        Shelf(pluss::make_note(values[part]), validity_mask()));
  }
  return DataFrame(std::move(output));
}

Shelf Shelf::pad(
    std::int64_t width,
    TextAlign align,
    char fill) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::pad: expected Note payload");
  PLUSS_CHECK(width >= 0, "Shelf::pad: width must be >= 0");
  const auto measured = as_note().lengths_chars();
  const auto lengths = measured.integers();
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
  return Shelf(rebuild_note_like(as_note(), values), validity_mask());
}

Shelf Shelf::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::chars: expected Note payload");
  PLUSS_CHECK(step != 0, "Shelf::chars: step cannot be zero");
  const auto chars = as_note().split_chars();
  const auto offsets = chars.offsets().integers();
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
  return Shelf(rebuild_note_like(as_note(), output), validity_mask());
}

Shelf Shelf::normalize(UnicodeForm form) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::normalize: expected Note payload");
  const auto& note = as_note();
  switch (form) {
    case UnicodeForm::NFC:
      return Shelf(note.to_nfc(), validity_mask());
    case UnicodeForm::NFD:
      return Shelf(note.to_nfd(), validity_mask());
    case UnicodeForm::NFKC:
      return Shelf(note.to_nfkc(), validity_mask());
    case UnicodeForm::NFKD:
      return Shelf(note.to_nfkd(), validity_mask());
  }
  PLUSS_FAIL("Shelf::normalize: unsupported Unicode form");
}

Shelf Shelf::encode(std::string_view encoding) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::encode: expected Note payload");
  if (encoding == "base64") {
    return Shelf(pluss::coden::ops_internal::base64_encode(as_note()), validity_mask());
  }
  if (encoding == "hex") {
    return Shelf(pluss::coden::ops_internal::hex_encode(as_note()), validity_mask());
  }
  if (encoding == "url") {
    return Shelf(pluss::coden::ops_internal::url_encode(as_note()), validity_mask());
  }
  PLUSS_FAIL(
      "Shelf::encode: encoding must be base64, hex, or url");
}

Shelf Shelf::replace(std::string_view from,
                       std::string_view to,
                       const TextOptions& options) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::replace: expected Note payload");
  if (!options.regex && options.case_sensitive && options.limit < 0) {
    return Shelf(as_note().replace(from, to), validity_mask());
  }
  if (options.regex && options.case_sensitive && options.limit < 0) {
    (void)compile_regex_(from, "Shelf::replace");
    return Shelf(as_note().regex().replace(from, to), validity_mask());
  }
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(from, options, "Shelf::replace");
  }
  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(size()));
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto text = as_note().at(row);
    values.push_back(
        options.regex
            ? replace_regex_limited_(text, *expression, to, options.limit)
            : replace_literal_(text, from, to, options));
  }
  return Shelf(rebuild_note_like(as_note(), values), validity_mask());
}

DataFrame Shelf::indicate(std::string_view delimiter,
                                  std::string prefix) const {
  PLUSS_CHECK(kind() == ShelfKind::Note,
              "Shelf::indicate: expected Note payload");
  PLUSS_CHECK(!delimiter.empty(),
              "Shelf::indicate: delimiter cannot be empty");
  const auto batch = as_note().split(delimiter);
  const auto offsets = batch.offsets().integers();
  const auto tokens = batch.tokens().vector();
  std::set<std::string> levels;
  for (std::int64_t token = 0;
       token < static_cast<std::int64_t>(tokens.size()); ++token) {
    if (!tokens[static_cast<std::size_t>(token)].empty()) {
      levels.insert(
          std::string(tokens[static_cast<std::size_t>(token)]));
    }
  }
  std::vector<std::pair<std::string, Shelf>> output;
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
        Shelf(bool_series_from_bools(indicators)));
  }
  return DataFrame(std::move(output));
}

ShelfNoteAccessor Shelf::nt() const {
  return ShelfNoteAccessor(*this);
}

ShelfNoteAccessor::ShelfNoteAccessor(Shelf base)
    : base_(std::move(base)) {}

Shelf ShelfNoteAccessor::strip() const { return base_.strip(); }
Shelf ShelfNoteAccessor::strip(std::string_view chars) const { return base_.strip(chars); }
Shelf ShelfNoteAccessor::lstrip() const { return base_.lstrip(); }
Shelf ShelfNoteAccessor::lstrip(std::string_view chars) const { return base_.lstrip(chars); }
Shelf ShelfNoteAccessor::rstrip() const { return base_.rstrip(); }
Shelf ShelfNoteAccessor::rstrip(std::string_view chars) const { return base_.rstrip(chars); }
Shelf ShelfNoteAccessor::lower() const { return base_.lower(); }
Shelf ShelfNoteAccessor::upper() const { return base_.upper(); }
Shelf ShelfNoteAccessor::casefold() const { return base_.casefold(); }
Shelf ShelfNoteAccessor::titlecase() const { return base_.titlecase(); }
Shelf ShelfNoteAccessor::length() const { return base_.length(); }
Shelf ShelfNoteAccessor::starts(std::string_view prefix) const { return base_.starts(prefix); }
Shelf ShelfNoteAccessor::ends(std::string_view suffix) const { return base_.ends(suffix); }
Shelf ShelfNoteAccessor::has(std::string_view needle, const TextOptions& options) const {
  return note_has_(base_, needle, options);
}
Shelf ShelfNoteAccessor::count(std::string_view pattern) const {
  return note_count_(base_, pattern, {.regex = true});
}
Shelf ShelfNoteAccessor::count(std::string_view pattern, const TextOptions& options) const {
  return note_count_(base_, pattern, options);
}
Shelf ShelfNoteAccessor::matches(std::string_view pattern, bool full) const {
  return full ? base_.fullmatch(pattern) : base_.matches(pattern);
}
Shelf ShelfNoteAccessor::fullmatch(std::string_view pattern) const { return base_.fullmatch(pattern); }
DataFrame ShelfNoteAccessor::findall(std::string_view pattern) const {
  return note_findall_(base_, pattern, {.regex = true});
}
DataFrame ShelfNoteAccessor::findall(
    std::string_view pattern,
    const TextOptions& options) const {
  return note_findall_(base_, pattern, options);
}
DataFrame ShelfNoteAccessor::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  return base_.extract(pattern, names);
}
DataFrame ShelfNoteAccessor::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  return base_.split(delimiter, limit, std::move(prefix));
}
DataFrame ShelfNoteAccessor::split(
    std::string_view pattern,
    const TextOptions& options) const {
  return base_.split(pattern, options);
}
DataFrame ShelfNoteAccessor::partition(
    std::string_view separator,
    std::string prefix) const {
  return base_.partition(separator, std::move(prefix));
}
Shelf ShelfNoteAccessor::pad(std::int64_t width, TextAlign align, char fill) const {
  return base_.pad(width, align, fill);
}
Shelf ShelfNoteAccessor::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  return base_.chars(start, stop, step);
}
Shelf ShelfNoteAccessor::normalize(UnicodeForm form) const {
  return base_.normalize(form);
}
Shelf ShelfNoteAccessor::encode(std::string_view encoding) const {
  return base_.encode(encoding);
}
DataFrame ShelfNoteAccessor::indicate(std::string_view delimiter, std::string prefix) const {
  return base_.indicate(delimiter, std::move(prefix));
}
Shelf ShelfNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return base_.replace(from, to);
}
Shelf ShelfNoteAccessor::replace(
    std::string_view from,
    std::string_view to,
    const TextOptions& options) const {
  return base_.replace(from, to, options);
}

namespace {

const pluss::Timeline& tl_payload_(const Shelf& base, std::string_view where) {
  PLUSS_CHECK(base.kind() == ShelfKind::Timeline,
              std::string(where) + ": expected Timeline payload");
  return base.as_timeline();
}

pluss::Series tl_validity_(const Shelf& base, std::string_view where) {
  auto valid = tl_payload_(base, where).isvalid();
  if (detail::has_mask(base)) {
    valid = valid.logical_and(*detail::validity_mask(base));
  }
  return valid;
}

Shelf tl_series_(const Shelf& base, const pluss::Series& values, std::string_view where) {
  auto out = Shelf(values.reshape({base.size()}), tl_validity_(base, where));
  if (detail::has_labels(base)) {
    out = detail::with_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

pluss::codeh::TimelineSpanUnit to_codeh_span_unit_(TimeUnit unit, std::string_view where) {
  switch (unit) {
    case TimeUnit::Nanos:
      return pluss::codeh::TimelineSpanUnit::Nanos;
    case TimeUnit::Micros:
      return pluss::codeh::TimelineSpanUnit::Micros;
    case TimeUnit::Millis:
      return pluss::codeh::TimelineSpanUnit::Millis;
    case TimeUnit::Seconds:
      return pluss::codeh::TimelineSpanUnit::Seconds;
    case TimeUnit::Days:
      return pluss::codeh::TimelineSpanUnit::Days;
    case TimeUnit::Weeks:
      return pluss::codeh::TimelineSpanUnit::Weeks;
    case TimeUnit::Months:
      return pluss::codeh::TimelineSpanUnit::Months;
    case TimeUnit::Years:
      return pluss::codeh::TimelineSpanUnit::Years;
    case TimeUnit::BusinessDays:
      return pluss::codeh::TimelineSpanUnit::BusinessDays;
  }
  PLUSS_FAIL(std::string(where) + ": unknown TimeUnit");
}

pluss::codeh::TimelineSpanOptions to_codeh_span_options_(
    const TimelineSpanOptions& options,
    std::string_view where) {
  pluss::codeh::TimelineSpanOptions out;
  out.unit = to_codeh_span_unit_(options.unit, where);
  out.mode = options.mode;
  out.anchor = options.anchor;
  out.signed_result = options.signed_result;
  out.calendar = options.calendar;
  out.closed = options.closed;
  return out;
}

Shelf tl_span_series_(
    const Shelf& base,
    const Shelf& other,
    const pluss::Series& values,
    std::string_view where) {
  auto valid = tl_validity_(base, where).logical_and(tl_validity_(other, where));
  auto out = Shelf(values.reshape({values.numel()}), valid.reshape({values.numel()}));
  if (values.numel() == base.size() && detail::has_labels(base)) {
    out = detail::with_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

Shelf tl_note_(const Shelf& base, const pluss::Note& values, std::string_view where) {
  std::vector<std::string> owned;
  owned.reserve(static_cast<std::size_t>(values.numel()));
  for (std::int64_t i = 0; i < values.numel(); ++i) {
    owned.push_back(std::string(values.at(i)));
  }
  auto out = Shelf(make_utf8_note(owned), tl_validity_(base, where));
  if (detail::has_labels(base)) {
    out = detail::with_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

Shelf tl_temporal_(const Shelf& base, const pluss::Timeline& values) {
  auto out = Shelf(values, detail::validity_mask(base));
  if (detail::has_labels(base)) {
    out = detail::with_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

Shelf tl_with_meta_(const Shelf& base, Shelf out) {
  if (detail::has_labels(base)) {
    out = detail::with_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

std::string_view tl_period_unit_(std::string_view freq) {
  if (freq == "Y" || freq == "YS" || freq == "y" || freq == "year" || freq == "years") {
    return "year";
  }
  if (freq == "M" || freq == "MS" || freq == "month" || freq == "months") {
    return "month";
  }
  if (freq == "D" || freq == "d" || freq == "day" || freq == "days") {
    return "day";
  }
  if (freq == "H" || freq == "h" || freq == "hour" || freq == "hours") {
    return "hour";
  }
  if (freq == "min" || freq == "T" || freq == "m" || freq == "minute" || freq == "minutes") {
    return "minute";
  }
  if (freq == "S" || freq == "s" || freq == "second" || freq == "seconds") {
    return "second";
  }
  return freq;
}

} // namespace

ShelfTimelineAccessor Shelf::tl() const {
  return ShelfTimelineAccessor(*this);
}

ShelfTimelineAccessor::ShelfTimelineAccessor(Shelf base)
    : base_(std::move(base)) {
  PLUSS_CHECK(base_.defined(), "Shelf::tl: Shelf is undefined");
  PLUSS_CHECK(base_.kind() == ShelfKind::Timeline,
              "Shelf::tl: expected Timeline payload");
}

Shelf ShelfTimelineAccessor::isnat() const {
  return tl_with_meta_(
      base_, Shelf(tl_payload_(base_, "Shelf::tl().isnat").isnat(), base_.validity_mask()));
}

Shelf ShelfTimelineAccessor::valid() const {
  return tl_with_meta_(base_, Shelf(tl_validity_(base_, "Shelf::tl().valid")));
}

Shelf ShelfTimelineAccessor::year() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().year").year(), "Shelf::tl().year");
}

Shelf ShelfTimelineAccessor::month() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().month").month(), "Shelf::tl().month");
}

Shelf ShelfTimelineAccessor::day() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().day").day(), "Shelf::tl().day");
}

Shelf ShelfTimelineAccessor::hour() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().hour").hour(), "Shelf::tl().hour");
}

Shelf ShelfTimelineAccessor::minute() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().minute").minute(), "Shelf::tl().minute");
}

Shelf ShelfTimelineAccessor::second() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().second").second(), "Shelf::tl().second");
}

Shelf ShelfTimelineAccessor::weekday() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().weekday").weekday(), "Shelf::tl().weekday");
}

Shelf ShelfTimelineAccessor::yearday() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().yearday").dayofyear(), "Shelf::tl().yearday");
}

Shelf ShelfTimelineAccessor::quarter() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().quarter").quarter(), "Shelf::tl().quarter");
}

Shelf ShelfTimelineAccessor::week() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().week").week(), "Shelf::tl().week");
}

Shelf ShelfTimelineAccessor::weekend() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().weekend").is_weekend(), "Shelf::tl().weekend");
}

Shelf ShelfTimelineAccessor::month_start() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().month_start").is_month_start(), "Shelf::tl().month_start");
}

Shelf ShelfTimelineAccessor::month_end() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().month_end").is_month_end(), "Shelf::tl().month_end");
}

Shelf ShelfTimelineAccessor::leap() const {
  return tl_series_(base_, tl_payload_(base_, "Shelf::tl().leap").is_leap_year(), "Shelf::tl().leap");
}

Shelf ShelfTimelineAccessor::weekday_name() const {
  return tl_note_(base_, tl_payload_(base_, "Shelf::tl().weekday_name").strftime("%A"), "Shelf::tl().weekday_name");
}

Shelf ShelfTimelineAccessor::month_name() const {
  return tl_note_(base_, tl_payload_(base_, "Shelf::tl().month_name").strftime("%B"), "Shelf::tl().month_name");
}

Shelf ShelfTimelineAccessor::fmt(std::string_view pattern) const {
  PLUSS_CHECK(!pattern.empty(), "Shelf::tl().fmt: pattern cannot be empty");
  return tl_note_(base_, tl_payload_(base_, "Shelf::tl().fmt").strftime(pattern), "Shelf::tl().fmt");
}

Shelf ShelfTimelineAccessor::iso() const {
  return tl_note_(base_, tl_payload_(base_, "Shelf::tl().iso").to_note_iso(), "Shelf::tl().iso");
}

Shelf ShelfTimelineAccessor::epoch(TimeUnit unit) const {
  const auto t = tl_payload_(base_, "Shelf::tl().epoch");
  switch (unit) {
    case TimeUnit::Nanos:
      return tl_series_(base_, t.epoch_nanos(), "Shelf::tl().epoch");
    case TimeUnit::Micros:
      return tl_series_(base_, t.epoch_micros(), "Shelf::tl().epoch");
    case TimeUnit::Millis:
      return tl_series_(base_, t.epoch_millis(), "Shelf::tl().epoch");
    case TimeUnit::Seconds:
      return tl_series_(base_, t.epoch_seconds(), "Shelf::tl().epoch");
    case TimeUnit::Days:
      return tl_series_(base_, t.epoch_days(), "Shelf::tl().epoch");
    case TimeUnit::Weeks:
    case TimeUnit::Months:
    case TimeUnit::Years:
    case TimeUnit::BusinessDays:
      PLUSS_FAIL("Shelf::tl().epoch: unit is span-only; use Shelf::tl().span");
  }
  PLUSS_FAIL("Shelf::tl().epoch: unknown TimeUnit");
}

Shelf ShelfTimelineAccessor::span(
    const Shelf& other,
    const TimelineSpanOptions& options) const {
  const auto left = tl_payload_(base_, "Shelf::tl().span");
  const auto right = tl_payload_(other, "Shelf::tl().span");
  return tl_span_series_(
      base_,
      other,
      left.span(right, to_codeh_span_options_(options, "Shelf::tl().span")),
      "Shelf::tl().span");
}

Shelf ShelfTimelineAccessor::span(const Shelf& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

Shelf ShelfTimelineAccessor::span(
    const pluss::Timeline& other,
    const TimelineSpanOptions& options) const {
  return span(Shelf(other), options);
}

Shelf ShelfTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

Shelf ShelfTimelineAccessor::floor(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Shelf::tl().floor: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Shelf::tl().floor").floor(unit));
}

Shelf ShelfTimelineAccessor::ceil(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Shelf::tl().ceil: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Shelf::tl().ceil").ceil(unit));
}

Shelf ShelfTimelineAccessor::round(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Shelf::tl().round: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Shelf::tl().round").round(unit));
}

Shelf ShelfTimelineAccessor::date() const {
  return tl_temporal_(base_, tl_payload_(base_, "Shelf::tl().date").date());
}

Shelf ShelfTimelineAccessor::time() const {
  return tl_temporal_(base_, tl_payload_(base_, "Shelf::tl().time").time());
}

Shelf ShelfTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  PLUSS_CHECK(!freq.empty(), "Shelf::tl().shift: freq cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Shelf::tl().shift")
          .shift(static_cast<int>(periods), tl_period_unit_(freq)));
}

Shelf ShelfTimelineAccessor::period(std::string_view freq) const {
  PLUSS_CHECK(!freq.empty(), "Shelf::tl().period: freq cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Shelf::tl().period").period_start(tl_period_unit_(freq)));
}

Shelf ShelfTimelineAccessor::tz(std::string_view name) const {
  PLUSS_CHECK(!name.empty(), "Shelf::tl().tz: timezone cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Shelf::tl().tz").tz_convert(name));
}

Shelf ShelfTimelineAccessor::localize(const TimelineLocalizeOptions& options) const {
  PLUSS_CHECK(!options.tz.empty(), "Shelf::tl().localize: timezone cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Shelf::tl().localize")
          .tz_localize(options.tz, options.fold, options.nonexistent));
}

Shelf ShelfTimelineAccessor::utc() const {
  return tl_temporal_(base_,
                      tl_payload_(base_, "Shelf::tl().utc").tz_convert("UTC"));
}
