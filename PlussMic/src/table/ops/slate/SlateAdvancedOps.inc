namespace {

pluss::codem::ScalarType slate_compute_type_(const pluss::Series& values) {
  return values.platform() == pluss::codem::Platform::CPU
             ? pluss::codem::ScalarType::Float64
             : pluss::codem::ScalarType::Float32;
}

void require_numeric_slate_(const Slate& value, std::string_view where) {
  PLUSS_CHECK(
      value.kind() == SlateKind::Series &&
          is_numeric_series_type(value.as_series()),
      std::string(where) + ": expected a real numeric Series Slate");
}

pluss::Series slate_valid_(const Slate& value) {
  auto valid = value.notnull().as_series();
  if (value.kind() == SlateKind::Series &&
      valid.platform() != value.as_series().platform()) {
    valid = valid.to(value.as_series().platform());
  }
  return valid;
}

Slate scalar_slate_(pluss::Series value, bool valid) {
  auto mask = valid
                  ? pluss::Series::ones(
                        value.sizes(), pluss::codem::ScalarType::Bool,
                        value.platform())
                  : pluss::Series::zeros(
                        value.sizes(), pluss::codem::ScalarType::Bool,
                        value.platform());
  return Slate(std::move(value), std::move(mask));
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

Slate running_slate_(const Slate& source,
                       bool skip_nulls,
                       int operation,
                       std::string_view where) {
  require_numeric_slate_(source, where);
  const auto& input = source.as_series();
  const auto dtype = slate_compute_type_(input);
  auto values = input.to(dtype);
  auto valid = slate_valid_(source);
  const auto invalid = valid.logical_not();
  pluss::Series result = pluss::series::undefined();
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
  return Slate(result, out_valid);
}

std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>
fixed_bounds_(std::int64_t size,
              std::int64_t window,
              const WindowOptions& options) {
  PLUSS_CHECK(window > 0, "Slate::moving: window must be > 0");
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

Slate indexed_numeric_window_(
    const Slate& source,
    const std::vector<std::int64_t>& starts,
    const std::vector<std::int64_t>& ends,
    WindowStatistic statistic,
    const WindowOptions& options,
    std::string_view where) {
  require_numeric_slate_(source, where);
  PLUSS_CHECK(
      starts.size() == static_cast<std::size_t>(source.size()) &&
          ends.size() == static_cast<std::size_t>(source.size()),
      std::string(where) + ": bounds must match Slate size");
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
  const auto dtype = slate_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Slate(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  const auto values = input.to(dtype);
  const auto valid = slate_valid_(source);
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
  pluss::Series result = pluss::series::undefined();

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
    return Slate(cpu_result, cpu_mask);
  }
  return Slate(result, output_valid);
}

Slate moving_moment_slate_(
    const Slate& source,
    std::int64_t window,
    const WindowOptions& options,
    int operation,
    std::string_view where) {
  require_numeric_slate_(source, where);
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
  const auto dtype = slate_compute_type_(input);
  const auto platform = input.platform();
  if (n == 0) {
    return Slate(
        pluss::Series::empty({0}, dtype, platform),
        pluss::Series::zeros(
            {0}, pluss::codem::ScalarType::Bool, platform));
  }
  if (options.closure != WindowClosure::Right) {
    const auto [starts, ends] = fixed_bounds_(n, window, options);
    const auto values = input.to(dtype);
    const auto valid = slate_valid_(source);
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
    return Slate(
        result, counts.ge(static_cast<double>(minimum)));
  }
  const auto values = input.to(dtype);
  const auto valid = slate_valid_(source);
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
      auto shifted = Slate(result, output_valid).shift(-offset);
      return shifted;
    }
  }
  return Slate(result, output_valid);
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

Slate note_bool_result_(const Slate& source,
                         const std::vector<std::uint8_t>& values) {
  return Slate(bool_from_u8_(values), detail::validity_mask(source));
}

// Note predicate/comparison methods return a shape-preserving Bool Note
// (pass 4 of the Note redesign) rather than a raw std::vector<uint8_t>.
Slate note_bool_result_(const Slate& source, const pluss::Note& mask) {
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
              "Slate::split: delimiter cannot be empty");
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

SlateValue Slate::covar(
    const Slate& other,
    const StatisticOptions& options) const {
  require_numeric_slate_(*this, "Slate::covar");
  require_numeric_slate_(other, "Slate::covar");
  const auto aligned = align_slate_pair_for_cases_(*this, other, "Slate::covar");
  if (aligned.aligned) {
    return aligned.left.covar(aligned.right, options);
  }
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Slate::covar: ddof and min_count must be >= 0");
  const auto platform = as_series().platform();
  PLUSS_CHECK(other.as_series().platform() == platform,
              "Slate::covar: operands must share a platform");
  const auto dtype = slate_compute_type_(as_series());
  const auto left_valid = slate_valid_(*this);
  const auto right_valid = slate_valid_(other);
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
  return SlateValue(Slate(
      value.reshape({1}),
      count.ge(static_cast<double>(minimum)).reshape({1})));
}

Slate Slate::percentiles(
    pluss::msharp::RangeView::Double points,
    std::int64_t min_count) const {
  require_numeric_slate_(*this, "Slate::percentiles");
  PLUSS_CHECK(!points.empty(),
              "Slate::percentiles: expected at least one point");
  PLUSS_CHECK(min_count >= 0,
              "Slate::percentiles: min_count must be >= 0");
  auto clean = dropnull().as_series();
  // quantile requires a floating dtype; integer/bool slates interpolate in
  // the compute type (matching pandas, where integer percentiles are float).
  if (!clean.is_floating_point()) {
    clean = clean.to(slate_compute_type_(as_series()));
  }
  std::vector<pluss::Series> values;
  values.reserve(points.size());
  for (const auto point : points) {
    PLUSS_CHECK(point >= 0.0 && point <= 1.0,
                "Slate::percentiles: points must be in [0, 1]");
    auto value =
        clean.numel() == 0
            ? pluss::Series::zeros(
                  {1}, slate_compute_type_(as_series()),
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
  return Slate(packed, valid);
}

Slate Slate::mode(bool drop_nulls) const {
  PLUSS_CHECK(
      kind() == SlateKind::Series || kind() == SlateKind::Note ||
          kind() == SlateKind::Timeline ||
          kind() == SlateKind::Duration,
      "Slate::mode: unsupported payload");
  const auto validity = notnull().as_series().host<bool>();
  std::int64_t null_count = 0;
  std::vector<bool> output_valid;
  if (kind() == SlateKind::Series) {
    require_numeric_slate_(*this, "Slate::mode");
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
    PLUSS_CHECK(best > 0, "Slate::mode: no values");
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
    return Slate(out, mask);
  }

  if (kind() == SlateKind::Timeline ||
      kind() == SlateKind::Duration) {
    const auto ticks = kind() == SlateKind::Timeline
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
    PLUSS_CHECK(best > 0, "Slate::mode: no values");
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
  PLUSS_CHECK(best > 0, "Slate::mode: no values");
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
  return Slate(pluss::make_note(modes),
                bool_series_from_bools(output_valid));
}

Slate Slate::rank(
    RankTieRule ties,
    RankNullRule nulls,
    bool ascending,
    bool percentage) const {
  require_numeric_slate_(*this, "Slate::rank");
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
  return Slate(output, output_valid);
}

SlateValue Slate::skew(std::int64_t min_count) const {
  require_numeric_slate_(*this, "Slate::skew");
  const auto dtype = slate_compute_type_(as_series());
  const auto valid = slate_valid_(*this);
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
  return SlateValue(Slate(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max<std::int64_t>(3, min_count))).reshape({1})));
}

SlateValue Slate::kurtosis(std::int64_t min_count) const {
  require_numeric_slate_(*this, "Slate::kurtosis");
  const auto dtype = slate_compute_type_(as_series());
  const auto valid = slate_valid_(*this);
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
  return SlateValue(Slate(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max<std::int64_t>(4, min_count))).reshape({1})));
}

SlateValue Slate::var(const StatisticOptions& options) const {
  require_numeric_slate_(*this, "Slate::var");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Slate::var: ddof and min_count must be >= 0");
  const auto dtype = slate_compute_type_(as_series());
  const auto valid = slate_valid_(*this);
  const auto count = valid.to(dtype).sum();
  const auto values = as_series().to(dtype);
  const auto origin =
      values.where(valid.logical_not(), 0.0).sum() / count;
  const auto centered =
      (values - origin).where(valid.logical_not(), 0.0);
  const auto result =
      (centered * centered).sum() /
      (count - static_cast<double>(options.ddof));
  return SlateValue(Slate(
      result.clamp_min(0.0).reshape({1}),
      count.ge(static_cast<double>(
          std::max(options.min_count, options.ddof + 1))).reshape({1})));
}

SlateValue Slate::std(const StatisticOptions& options) const {
  const auto result = var(options);
  return SlateValue(Slate(
      result.as_series().clamp_min(0.0).sqrt(),
      result.slate().validity_mask()));
}

SlateValue Slate::se(ErrorTarget target, const StatisticOptions& options) const {
  PLUSS_CHECK(target == ErrorTarget::Mean,
              "Slate::se: only ErrorTarget::Mean is implemented in this pass");
  require_numeric_slate_(*this, "Slate::se");
  PLUSS_CHECK(options.ddof >= 0 && options.min_count >= 0,
              "Slate::se: ddof and min_count must be >= 0");
  const auto dtype = slate_compute_type_(as_series());
  const auto valid = slate_valid_(*this);
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
  return SlateValue(Slate(
      result.reshape({1}),
      count.ge(static_cast<double>(
          std::max(options.min_count, options.ddof + 1))).reshape({1})));
}

Slate Slate::scan(ScanStatistic statistic, bool skip_nulls) const {
  switch (statistic) {
    case ScanStatistic::Sum:
      return running_slate_(*this, skip_nulls, 0, "Slate::scan");
    case ScanStatistic::Product:
      return running_slate_(*this, skip_nulls, 1, "Slate::scan");
    case ScanStatistic::Minimum:
      return running_slate_(*this, skip_nulls, 2, "Slate::scan");
    case ScanStatistic::Maximum:
      return running_slate_(*this, skip_nulls, 3, "Slate::scan");
  }
  PLUSS_FAIL("Slate::scan: unsupported statistic");
}

Slate Slate::cumsum(bool skip_nulls) const {
  return scan(ScanStatistic::Sum, skip_nulls);
}

Slate Slate::cumprod(bool skip_nulls) const {
  return scan(ScanStatistic::Product, skip_nulls);
}

Slate Slate::cummin(bool skip_nulls) const {
  return scan(ScanStatistic::Minimum, skip_nulls);
}

Slate Slate::cummax(bool skip_nulls) const {
  return scan(ScanStatistic::Maximum, skip_nulls);
}

SlateValue Slate::corr_with(
    const Slate& other,
    SlateCorrMethod method,
    std::int64_t min_count) const {
  require_numeric_slate_(*this, "Slate::corr_with");
  require_numeric_slate_(other, "Slate::corr_with");
  const auto aligned = align_slate_pair_for_cases_(*this, other, "Slate::corr_with");
  if (aligned.aligned) {
    return aligned.left.corr_with(aligned.right, method, min_count);
  }
  PLUSS_CHECK(min_count >= 1,
              "Slate::corr_with: min_count must be >= 1");
  PLUSS_CHECK(as_series().platform() == other.as_series().platform(),
              "Slate::corr_with: operands must share a platform");
  const auto valid =
      notnull().as_series().logical_and(other.notnull().as_series());
  if (method == SlateCorrMethod::Pearson) {
    const auto dtype = slate_compute_type_(as_series());
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
    return SlateValue(Slate(
        (numerator / (left_square * right_square).sqrt())
            .clamp(-1.0, 1.0).reshape({1}),
        output_valid.reshape({1})));
  }
  const auto count =
      valid.to(pluss::codem::ScalarType::Int64).sum().item<std::int64_t>();
  auto value = pluss::Series::zeros(
      {1}, slate_compute_type_(as_series()), as_series().platform());
  bool has_value = false;
  if (count >= min_count) {
    const auto corr_value = corr(other, method);
    has_value = std::isfinite(corr_value);
    value = pluss::Series::full(
        {1}, corr_value, slate_compute_type_(as_series()),
        as_series().platform());
  }
  return SlateValue(scalar_slate_(value, has_value));
}

SlateValue Slate::matmul(const Slate& other) const {
  require_numeric_slate_(*this, "Slate::matmul");
  require_numeric_slate_(other, "Slate::matmul");
  const auto aligned = align_slate_pair_for_cases_(*this, other, "Slate::matmul");
  if (aligned.aligned) {
    return aligned.left.matmul(aligned.right);
  }
  PLUSS_CHECK(!has_validity_mask() && !other.has_validity_mask(),
              "Slate::matmul: null values are not supported");
  return SlateValue(Slate(as_series().matmul(other.as_series()).reshape({1})));
}

Slate Slate::bounded(
    const WindowBoundsFn& bounds,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  PLUSS_CHECK(static_cast<bool>(bounds),
              "Slate::bounded: bounds callback is empty");
  auto [starts, ends] = bounds(size(), options);
  return indexed_numeric_window_(
      *this, starts, ends, statistic, options,
      "Slate::bounded");
}

Slate Slate::moving(
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  if (statistic == WindowStatistic::Sem) {
    return moving_moment_slate_(*this, window, options, 0, "Slate::moving");
  }
  if (statistic == WindowStatistic::Skew) {
    return moving_moment_slate_(*this, window, options, 1, "Slate::moving");
  }
  if (statistic == WindowStatistic::Kurtosis) {
    return moving_moment_slate_(*this, window, options, 2, "Slate::moving");
  }
  auto [starts, ends] = fixed_bounds_(size(), window, options);
  return indexed_numeric_window_(
      *this, starts, ends, statistic, options, "Slate::moving");
}

Slate Slate::growing(
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

Slate exponential_mean_slate_(
    const Slate& source,
    double alpha,
    bool adjusted,
    std::int64_t min_count,
    bool ignore_nulls) {
  require_numeric_slate_(source, "Slate::decay");
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0,
              "Slate::decay: alpha must be in (0, 1]");
  PLUSS_CHECK(min_count > 0,
              "Slate::decay: min_count must be > 0");
  const auto dtype = slate_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  const auto values = source.as_series().to(dtype);
  const auto valid = slate_valid_(source);
  const auto count = valid.to(dtype).cumsum(0);
  if (!adjusted) {
    PLUSS_CHECK(
        !detail::has_validity_mask(source),
        "Slate::decay(adjusted=false): nullable input requires "
        "adjusted=true");
    auto result = Slate(pluss::subsp::stats::ts::ema(values, alpha));
    return detail::with_validity_mask(result,
        count.ge(static_cast<double>(min_count)));
  }
  if (alpha == 1.0) {
    return Slate(values, valid.logical_and(
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
  return Slate(
      numerator / denominator,
      count.ge(static_cast<double>(min_count)));
}

Slate exponential_variance_slate_(
    const Slate& source,
    double alpha,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) {
  require_numeric_slate_(source, "Slate::decay");
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0,
              "Slate::decay: alpha must be in (0, 1]");
  const auto dtype = slate_compute_type_(source.as_series());
  const auto platform = source.as_series().platform();
  const auto values = source.as_series().to(dtype);
  const auto valid = slate_valid_(source);
  const auto count = valid.to(dtype).cumsum(0);
  const auto origin =
      values.where(valid.logical_not(), 0.0).sum() /
      valid.to(dtype).sum();
  const auto centered =
      (values - origin).where(valid.logical_not(), 0.0);
  const auto centered_slate =
      Slate(centered, detail::validity_mask(source));
  const auto mean = exponential_mean_slate_(centered_slate,
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
        !detail::has_validity_mask(source),
        "Slate::decay(adjusted=false): nullable input "
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
  return Slate(result, output_valid);
}

Slate Slate::decay(
    double alpha,
    WindowStatistic statistic,
    bool adjusted,
    bool bias_corrected,
    std::int64_t min_count,
    bool ignore_nulls) const {
  switch (statistic) {
    case WindowStatistic::Mean:
      return exponential_mean_slate_(*this, alpha, adjusted, min_count, ignore_nulls);
    case WindowStatistic::Var:
      return exponential_variance_slate_(
          *this, alpha, adjusted, bias_corrected, min_count, ignore_nulls);
    case WindowStatistic::Std: {
      const auto variance = exponential_variance_slate_(
          *this, alpha, adjusted, bias_corrected, min_count, ignore_nulls);
      return Slate(variance.as_series().sqrt(), variance.validity_mask());
    }
    default:
      PLUSS_FAIL("Slate::decay: only Mean, Var, and Std are supported");
  }
}

Slate Slate::weigh(
    pluss::msharp::RangeView::Double weights,
    WindowStatistic statistic,
    std::int64_t min_count) const {
  require_numeric_slate_(*this, "Slate::weigh");
  PLUSS_CHECK(!weights.empty(),
              "Slate::weigh: weights cannot be empty");
  PLUSS_CHECK(min_count > 0 &&
                  min_count <= static_cast<std::int64_t>(weights.size()),
              "Slate::weigh: invalid min_count");
  if (statistic == WindowStatistic::Minimum ||
      statistic == WindowStatistic::Maximum) {
    return moving(
        static_cast<std::int64_t>(weights.size()), statistic,
        WindowOptions{.min_count = min_count});
  }
  const auto n = size();
  const auto window = static_cast<std::int64_t>(weights.size());
  const auto dtype = slate_compute_type_(as_series());
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
  const auto valid = slate_valid_(*this);
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
  pluss::Series result = pluss::series::undefined();
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
  return Slate(
      result, count.ge(static_cast<double>(min_count)));
}

Slate Slate::timed(
    const Slate& axis,
    std::int64_t duration_nanos,
    TimeWindowStatistic statistic,
    const StatisticOptions& options,
    WindowClosure closure) const {
  PLUSS_CHECK(axis.size() == size(),
              "Slate::timed: axis length mismatch");
  PLUSS_CHECK(duration_nanos > 0,
              "Slate::timed: duration_nanos must be > 0");
  PLUSS_CHECK(
      axis.notnull().as_series().all().item<bool>(),
      "Slate::timed: axis cannot contain null values");
  std::vector<std::int64_t> ticks;
  if (axis.kind() == SlateKind::Timeline) {
    ticks = axis.as_timeline().ticks();
  } else if (axis.kind() == SlateKind::Series &&
             axis.as_series().scalar_type() ==
                 pluss::codem::ScalarType::Int64) {
    ticks = axis.as_series().to(
        pluss::codem::Platform::CPU).host<std::int64_t>();
  } else {
    PLUSS_FAIL(
        "Slate::timed: axis must be Timeline or Int64 "
        "nanoseconds");
  }
  PLUSS_CHECK(std::is_sorted(ticks.begin(), ticks.end()),
              "Slate::timed: axis must be monotonic");
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
      "Slate::timed");
}

Slate Slate::grouped(
    const Slate& groups,
    std::int64_t window,
    WindowStatistic statistic,
    const WindowOptions& options) const {
  require_numeric_slate_(*this, "Slate::grouped");
  PLUSS_CHECK(groups.size() == size(),
              "Slate::grouped: group length mismatch");
  Slate cpu_groups = groups;
  if (groups.kind() == SlateKind::Series &&
      groups.as_series().platform() != pluss::codem::Platform::CPU) {
    cpu_groups = Slate(
        groups.as_series().to(pluss::codem::Platform::CPU),
        groups.validity_mask());
  }
  std::unordered_map<std::string, std::vector<std::int64_t>> rows_by_group;
  for (std::int64_t row = 0; row < size(); ++row) {
    const auto key =
        groups.is_valid(row)
            ? std::string("\1") + slate_scalar_string_at(cpu_groups, row)
            : std::string(1, '\0');
    rows_by_group[key].push_back(row);
  }
  const auto dtype = slate_compute_type_(as_series());
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
  return Slate(output, output_valid);
}

Slate Slate::flow(
    const std::function<Slate(const Slate&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Slate::flow: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Slate::flow: callback returned an undefined Slate");
  return result;
}

Slate Slate::fold(
    const std::function<Slate(const Slate&)>& fn,
    ResultShape shape) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Slate::fold: callback is empty");
  auto result = fn(*this);
  PLUSS_CHECK(result.defined(),
              "Slate::fold: callback returned undefined");
  if (shape == ResultShape::Reduce) {
    PLUSS_CHECK(result.size() == 1,
                "Slate::fold: Reduce requires one value");
  } else if (shape == ResultShape::Preserve) {
    PLUSS_CHECK(result.size() == size(),
                "Slate::fold: Preserve requires input length");
  }
  return result;
}

Slate Slate::transform(
    const std::function<SlateValue(const SlateValue&)>& fn) const {
  return map(fn);
}

Slate Slate::transform(
    const std::function<Slate(const Slate&)>& fn,
    ResultShape shape) const {
  return fold(fn, shape);
}

Slate Slate::map(
    const std::function<SlateValue(const SlateValue&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn),
              "Slate::map: callback is empty");
  std::vector<std::optional<Slate>> mapped_values(
      static_cast<std::size_t>(size()));
  std::optional<Slate> prototype;
  for (std::int64_t row = 0; row < size(); ++row) {
    if (!is_valid(row)) {
      continue;
    }
    const std::array<std::int64_t, 1> index{row};
    auto mapped = fn(SlateValue(take(index)));
    PLUSS_CHECK(mapped.defined() && mapped.slate().size() == 1,
                "Slate::map: callback must return one value");
    if (prototype.has_value()) {
      PLUSS_CHECK(mapped.kind() == prototype->kind(),
                  "Slate::map: output kinds must be consistent");
    }
    prototype = mapped.slate();
    mapped_values[static_cast<std::size_t>(row)] = mapped.slate();
  }
  if (!prototype.has_value()) {
    return *this;
  }
  auto null_mask = pluss::Series::zeros(
      {1}, pluss::codem::ScalarType::Bool);
  if (prototype->kind() == SlateKind::Series &&
      prototype->as_series().platform() !=
          pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(prototype->as_series().platform());
  } else if (prototype->kind() == SlateKind::Timeline &&
             prototype->as_timeline().epoch_nanos().platform() !=
                 pluss::codem::Platform::CPU) {
    null_mask = null_mask.to(
        prototype->as_timeline().epoch_nanos().platform());
  }
  std::vector<Slate> values;
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
  if (kind == SlateKind::Series) {
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
    return Slate(output, mask);
  }
  if (kind == SlateKind::Note) {
    std::vector<std::string> payloads;
    payloads.reserve(values.size());
    for (const auto& value : values) {
      payloads.emplace_back(value.as_note().at(0));
      validity.push_back(detail::valid_at(value, 0));
    }
    return Slate(pluss::make_note(payloads),
                  bool_series_from_bools(validity));
  }
  if (kind == SlateKind::Timeline) {
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
    return Slate(output, mask);
  }
  PLUSS_CHECK(kind == SlateKind::Duration,
              "Slate::map: unsupported mapped output kind");
  std::vector<std::int64_t> nanos;
  nanos.reserve(values.size());
  for (const auto& value : values) {
    nanos.push_back(value.as_duration().host().front());
    validity.push_back(detail::valid_at(value, 0));
  }
  return Slate(pluss::duration::nanos(nanos), bool_series_from_bools(validity));
}

Slate Slate::upper() const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::upper: expected Note payload");
  return Slate(as_note().upper(), validity_mask());
}

Slate Slate::casefold() const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::casefold: expected Note payload");
  return Slate(as_note().casefold(), validity_mask());
}

Slate Slate::titlecase() const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::titlecase: expected Note payload");
  return Slate(as_note().title(), validity_mask());
}

Slate Slate::length() const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::length: expected Note payload");
  const auto measured = as_note().lengths_chars();
  const auto lengths = measured.integers();
  auto output = pluss::Series::empty(
      {static_cast<std::int64_t>(lengths.size())},
      pluss::codem::ScalarType::Int64);
  std::copy(lengths.begin(), lengths.end(),
            reinterpret_cast<std::int64_t*>(output.raw_data()));
  return Slate(output, validity_mask());
}

Slate Slate::starts(std::string_view prefix) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::starts: expected Note payload");
  return note_bool_result_(*this, as_note().starts_with(prefix));
}

Slate Slate::ends(std::string_view suffix) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::ends: expected Note payload");
  return note_bool_result_(*this, as_note().ends_with(suffix));
}

Slate note_has_(const Slate& source,
                 std::string_view needle,
                 const TextOptions& options) {
  PLUSS_CHECK(source.kind() == SlateKind::Note,
              "Slate::nt().has: expected Note payload");
  if (options.regex) {
    if (options.case_sensitive) {
      (void)compile_regex_(needle, "Slate::nt().has");
      return note_bool_result_(source, source.as_note().regex().search(needle));
    }
    const auto expression = compile_regex_(needle, options, "Slate::nt().has");
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

Slate note_count_(const Slate& source,
                   std::string_view pattern,
                   const TextOptions& options) {
  PLUSS_CHECK(source.kind() == SlateKind::Note,
              "Slate::nt().count: expected Note payload");
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(pattern, options, "Slate::nt().count");
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
  return Slate(output, detail::validity_mask(source));
}

DataFrame note_findall_(const Slate& source,
                        std::string_view pattern,
                        const TextOptions& options) {
  PLUSS_CHECK(source.kind() == SlateKind::Note,
              "Slate::nt().findall: expected Note payload");
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(pattern, options, "Slate::nt().findall");
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
  std::vector<std::pair<std::string, Slate>> output;
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
        Slate(pluss::make_note(values), bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

Slate Slate::matches(std::string_view pattern) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::matches: expected Note payload");
  (void)compile_regex_(pattern, "Slate::matches");
  return note_bool_result_(*this, as_note().regex().search(pattern));
}

Slate Slate::fullmatch(std::string_view pattern) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::fullmatch: expected Note payload");
  (void)compile_regex_(pattern, "Slate::fullmatch");
  return note_bool_result_(*this, as_note().regex().match(pattern));
}

DataFrame Slate::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::extract: expected Note payload");
  auto expression =
      compile_regex_(pattern, "Slate::extract");
  const auto group_count = expression.mark_count();
  PLUSS_CHECK(group_count > 0,
              "Slate::extract: pattern needs a capture group");
  PLUSS_CHECK(names.empty() || names.size() == group_count,
              "Slate::extract: names must match capture groups");
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
  std::vector<std::pair<std::string, Slate>> output;
  output.reserve(group_count);
  for (std::size_t group = 0; group < group_count; ++group) {
    const auto name =
        names.empty() ? "group_" + std::to_string(group)
                      : std::string(names[group]);
    output.emplace_back(
        name,
        Slate(pluss::make_note(columns[group]),
               bool_series_from_bools(valid[group])));
  }
  return DataFrame(std::move(output));
}

DataFrame Slate::framerize(std::string_view name) const {
  std::string column = name.empty()
                           ? (named() ? this->name() : std::string("x"))
                           : std::string(name);

  const bool indexed = detail::has_display_labels(*this);
  std::optional<Slate> cases =
      indexed ? std::optional<Slate>(detail::display_labels(*this)) : std::nullopt;

  // The column carries values and validity only; its own name and index are
  // dropped -- the name is replaced by `column`, the index becomes the frame's
  // cases.
  Slate body = indexed ? detail::clear_display_labels(*this) : *this;
  if (body.named()) {
    body = body.unname();
  }

  std::vector<std::pair<std::string, Slate>> output;
  output.emplace_back(std::move(column), std::move(body));
  return DataFrame(std::move(output), std::move(cases));
}

DataFrame Slate::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::split: expected Note payload");
  const auto batch =
      limit < 0 ? as_note().split(delimiter)
                : as_note().split(delimiter, limit);
  const auto offsets = batch.offsets().integers();
  const auto tokens = batch.tokens().vector();
  std::int64_t width = 0;
  for (std::size_t row = 0; row + 1 < offsets.size(); ++row) {
    width = std::max(width, offsets[row + 1] - offsets[row]);
  }
  std::vector<std::pair<std::string, Slate>> output;
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
        Slate(pluss::make_note(values),
               bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

DataFrame Slate::split(
    std::string_view pattern,
    const TextOptions& options) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::split: expected Note payload");
  std::vector<std::vector<std::string>> rows;
  rows.reserve(static_cast<std::size_t>(size()));
  std::int64_t width = 0;
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(pattern, options, "Slate::split");
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
  std::vector<std::pair<std::string, Slate>> output;
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
        Slate(pluss::make_note(values),
               bool_series_from_bools(valid)));
  }
  return DataFrame(std::move(output));
}

DataFrame Slate::partition(
    std::string_view separator,
    std::string prefix) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::partition: expected Note payload");
  PLUSS_CHECK(!separator.empty(),
              "Slate::partition: separator cannot be empty");
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
  std::vector<std::pair<std::string, Slate>> output;
  for (std::size_t part = 0; part < values.size(); ++part) {
    output.emplace_back(
        prefix + "_" + std::to_string(part),
        Slate(pluss::make_note(values[part]), validity_mask()));
  }
  return DataFrame(std::move(output));
}

Slate Slate::pad(
    std::int64_t width,
    TextAlign align,
    char fill) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::pad: expected Note payload");
  PLUSS_CHECK(width >= 0, "Slate::pad: width must be >= 0");
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
  return Slate(rebuild_note_like(as_note(), values), validity_mask());
}

Slate Slate::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::chars: expected Note payload");
  PLUSS_CHECK(step != 0, "Slate::chars: step cannot be zero");
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
  return Slate(rebuild_note_like(as_note(), output), validity_mask());
}

Slate Slate::normalize(UnicodeForm form) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::normalize: expected Note payload");
  const auto& note = as_note();
  switch (form) {
    case UnicodeForm::NFC:
      return Slate(note.to_nfc(), validity_mask());
    case UnicodeForm::NFD:
      return Slate(note.to_nfd(), validity_mask());
    case UnicodeForm::NFKC:
      return Slate(note.to_nfkc(), validity_mask());
    case UnicodeForm::NFKD:
      return Slate(note.to_nfkd(), validity_mask());
  }
  PLUSS_FAIL("Slate::normalize: unsupported Unicode form");
}

Slate Slate::encode(std::string_view encoding) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::encode: expected Note payload");
  if (encoding == "base64") {
    return Slate(pluss::coden::ops_internal::base64_encode(as_note()), validity_mask());
  }
  if (encoding == "hex") {
    return Slate(pluss::coden::ops_internal::hex_encode(as_note()), validity_mask());
  }
  if (encoding == "url") {
    return Slate(pluss::coden::ops_internal::url_encode(as_note()), validity_mask());
  }
  PLUSS_FAIL(
      "Slate::encode: encoding must be base64, hex, or url");
}

Slate Slate::replace(std::string_view from,
                       std::string_view to,
                       const TextOptions& options) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::replace: expected Note payload");
  if (!options.regex && options.case_sensitive && options.limit < 0) {
    return Slate(as_note().replace(from, to), validity_mask());
  }
  if (options.regex && options.case_sensitive && options.limit < 0) {
    (void)compile_regex_(from, "Slate::replace");
    return Slate(as_note().regex().replace(from, to), validity_mask());
  }
  std::optional<std::regex> expression;
  if (options.regex) {
    expression = compile_regex_(from, options, "Slate::replace");
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
  return Slate(rebuild_note_like(as_note(), values), validity_mask());
}

DataFrame Slate::indicate(std::string_view delimiter,
                                  std::string prefix) const {
  PLUSS_CHECK(kind() == SlateKind::Note,
              "Slate::indicate: expected Note payload");
  PLUSS_CHECK(!delimiter.empty(),
              "Slate::indicate: delimiter cannot be empty");
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
  std::vector<std::pair<std::string, Slate>> output;
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
        Slate(bool_series_from_bools(indicators)));
  }
  return DataFrame(std::move(output));
}

SlateNoteAccessor Slate::nt() const {
  return SlateNoteAccessor(*this);
}

SlateNoteAccessor::SlateNoteAccessor(Slate base)
    : base_(std::move(base)) {}

Slate SlateNoteAccessor::strip() const { return base_.strip(); }
Slate SlateNoteAccessor::strip(std::string_view chars) const { return base_.strip(chars); }
Slate SlateNoteAccessor::lstrip() const { return base_.lstrip(); }
Slate SlateNoteAccessor::lstrip(std::string_view chars) const { return base_.lstrip(chars); }
Slate SlateNoteAccessor::rstrip() const { return base_.rstrip(); }
Slate SlateNoteAccessor::rstrip(std::string_view chars) const { return base_.rstrip(chars); }
Slate SlateNoteAccessor::lower() const { return base_.lower(); }
Slate SlateNoteAccessor::upper() const { return base_.upper(); }
Slate SlateNoteAccessor::casefold() const { return base_.casefold(); }
Slate SlateNoteAccessor::titlecase() const { return base_.titlecase(); }
Slate SlateNoteAccessor::length() const { return base_.length(); }
Slate SlateNoteAccessor::starts(std::string_view prefix) const { return base_.starts(prefix); }
Slate SlateNoteAccessor::ends(std::string_view suffix) const { return base_.ends(suffix); }
Slate SlateNoteAccessor::has(std::string_view needle, const TextOptions& options) const {
  return note_has_(base_, needle, options);
}
Slate SlateNoteAccessor::count(std::string_view pattern) const {
  return note_count_(base_, pattern, {.regex = true});
}
Slate SlateNoteAccessor::count(std::string_view pattern, const TextOptions& options) const {
  return note_count_(base_, pattern, options);
}
Slate SlateNoteAccessor::matches(std::string_view pattern, bool full) const {
  return full ? base_.fullmatch(pattern) : base_.matches(pattern);
}
Slate SlateNoteAccessor::fullmatch(std::string_view pattern) const { return base_.fullmatch(pattern); }
DataFrame SlateNoteAccessor::findall(std::string_view pattern) const {
  return note_findall_(base_, pattern, {.regex = true});
}
DataFrame SlateNoteAccessor::findall(
    std::string_view pattern,
    const TextOptions& options) const {
  return note_findall_(base_, pattern, options);
}
DataFrame SlateNoteAccessor::extract(
    std::string_view pattern,
    pluss::msharp::RangeView::Of<std::string_view> names) const {
  return base_.extract(pattern, names);
}
DataFrame SlateNoteAccessor::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  return base_.split(delimiter, limit, std::move(prefix));
}
DataFrame SlateNoteAccessor::split(
    std::string_view pattern,
    const TextOptions& options) const {
  return base_.split(pattern, options);
}
DataFrame SlateNoteAccessor::partition(
    std::string_view separator,
    std::string prefix) const {
  return base_.partition(separator, std::move(prefix));
}
Slate SlateNoteAccessor::pad(std::int64_t width, TextAlign align, char fill) const {
  return base_.pad(width, align, fill);
}
Slate SlateNoteAccessor::chars(
    std::int64_t start,
    std::optional<std::int64_t> stop,
    std::int64_t step) const {
  return base_.chars(start, stop, step);
}
Slate SlateNoteAccessor::normalize(UnicodeForm form) const {
  return base_.normalize(form);
}
Slate SlateNoteAccessor::encode(std::string_view encoding) const {
  return base_.encode(encoding);
}
DataFrame SlateNoteAccessor::indicate(std::string_view delimiter, std::string prefix) const {
  return base_.indicate(delimiter, std::move(prefix));
}
Slate SlateNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return base_.replace(from, to);
}
Slate SlateNoteAccessor::replace(
    std::string_view from,
    std::string_view to,
    const TextOptions& options) const {
  return base_.replace(from, to, options);
}

namespace {

const pluss::Timeline& tl_payload_(const Slate& base, std::string_view where) {
  PLUSS_CHECK(base.kind() == SlateKind::Timeline,
              std::string(where) + ": expected Timeline payload");
  return base.as_timeline();
}

pluss::Series tl_validity_(const Slate& base, std::string_view where) {
  auto valid = tl_payload_(base, where).isvalid();
  if (detail::has_validity_mask(base)) {
    valid = valid.logical_and(*detail::validity_mask(base));
  }
  return valid;
}

Slate tl_series_(const Slate& base, const pluss::Series& values, std::string_view where) {
  auto out = Slate(values.reshape({base.size()}), tl_validity_(base, where));
  if (detail::has_display_labels(base)) {
    out = detail::with_display_labels(out, detail::display_labels(base));
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

Slate tl_span_series_(
    const Slate& base,
    const Slate& other,
    const pluss::Series& values,
    std::string_view where) {
  auto valid = tl_validity_(base, where).logical_and(tl_validity_(other, where));
  auto out = Slate(values.reshape({values.numel()}), valid.reshape({values.numel()}));
  if (values.numel() == base.size() && detail::has_display_labels(base)) {
    out = detail::with_display_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

Slate tl_note_(const Slate& base, const pluss::Note& values, std::string_view where) {
  std::vector<std::string> owned;
  owned.reserve(static_cast<std::size_t>(values.numel()));
  for (std::int64_t i = 0; i < values.numel(); ++i) {
    owned.push_back(std::string(values.at(i)));
  }
  auto out = Slate(make_utf8_note(owned), tl_validity_(base, where));
  if (detail::has_display_labels(base)) {
    out = detail::with_display_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

Slate tl_temporal_(const Slate& base, const pluss::Timeline& values) {
  auto out = Slate(values, detail::validity_mask(base));
  if (detail::has_display_labels(base)) {
    out = detail::with_display_labels(out, detail::display_labels(base));
  }
  if (base.named()) {
    out = out.name(base.name());
  }
  return out;
}

Slate tl_with_meta_(const Slate& base, Slate out) {
  if (detail::has_display_labels(base)) {
    out = detail::with_display_labels(out, detail::display_labels(base));
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

SlateTimelineAccessor Slate::tl() const {
  return SlateTimelineAccessor(*this);
}

SlateTimelineAccessor::SlateTimelineAccessor(Slate base)
    : base_(std::move(base)) {
  PLUSS_CHECK(base_.defined(), "Slate::tl: Slate is undefined");
  PLUSS_CHECK(base_.kind() == SlateKind::Timeline,
              "Slate::tl: expected Timeline payload");
}

Slate SlateTimelineAccessor::isnat() const {
  return tl_with_meta_(
      base_, Slate(tl_payload_(base_, "Slate::tl().isnat").isnat(), base_.validity_mask()));
}

Slate SlateTimelineAccessor::valid() const {
  return tl_with_meta_(base_, Slate(tl_validity_(base_, "Slate::tl().valid")));
}

Slate SlateTimelineAccessor::year() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().year").year(), "Slate::tl().year");
}

Slate SlateTimelineAccessor::month() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().month").month(), "Slate::tl().month");
}

Slate SlateTimelineAccessor::day() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().day").day(), "Slate::tl().day");
}

Slate SlateTimelineAccessor::hour() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().hour").hour(), "Slate::tl().hour");
}

Slate SlateTimelineAccessor::minute() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().minute").minute(), "Slate::tl().minute");
}

Slate SlateTimelineAccessor::second() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().second").second(), "Slate::tl().second");
}

Slate SlateTimelineAccessor::weekday() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().weekday").weekday(), "Slate::tl().weekday");
}

Slate SlateTimelineAccessor::yearday() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().yearday").dayofyear(), "Slate::tl().yearday");
}

Slate SlateTimelineAccessor::quarter() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().quarter").quarter(), "Slate::tl().quarter");
}

Slate SlateTimelineAccessor::week() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().week").week(), "Slate::tl().week");
}

Slate SlateTimelineAccessor::weekend() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().weekend").is_weekend(), "Slate::tl().weekend");
}

Slate SlateTimelineAccessor::month_start() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().month_start").is_month_start(), "Slate::tl().month_start");
}

Slate SlateTimelineAccessor::month_end() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().month_end").is_month_end(), "Slate::tl().month_end");
}

Slate SlateTimelineAccessor::leap() const {
  return tl_series_(base_, tl_payload_(base_, "Slate::tl().leap").is_leap_year(), "Slate::tl().leap");
}

Slate SlateTimelineAccessor::weekday_name() const {
  return tl_note_(base_, tl_payload_(base_, "Slate::tl().weekday_name").strftime("%A"), "Slate::tl().weekday_name");
}

Slate SlateTimelineAccessor::month_name() const {
  return tl_note_(base_, tl_payload_(base_, "Slate::tl().month_name").strftime("%B"), "Slate::tl().month_name");
}

Slate SlateTimelineAccessor::fmt(std::string_view pattern) const {
  PLUSS_CHECK(!pattern.empty(), "Slate::tl().fmt: pattern cannot be empty");
  return tl_note_(base_, tl_payload_(base_, "Slate::tl().fmt").strftime(pattern), "Slate::tl().fmt");
}

Slate SlateTimelineAccessor::iso() const {
  return tl_note_(base_, tl_payload_(base_, "Slate::tl().iso").to_note_iso(), "Slate::tl().iso");
}

Slate SlateTimelineAccessor::epoch(TimeUnit unit) const {
  const auto t = tl_payload_(base_, "Slate::tl().epoch");
  switch (unit) {
    case TimeUnit::Nanos:
      return tl_series_(base_, t.epoch_nanos(), "Slate::tl().epoch");
    case TimeUnit::Micros:
      return tl_series_(base_, t.epoch_micros(), "Slate::tl().epoch");
    case TimeUnit::Millis:
      return tl_series_(base_, t.epoch_millis(), "Slate::tl().epoch");
    case TimeUnit::Seconds:
      return tl_series_(base_, t.epoch_seconds(), "Slate::tl().epoch");
    case TimeUnit::Days:
      return tl_series_(base_, t.epoch_days(), "Slate::tl().epoch");
    case TimeUnit::Weeks:
    case TimeUnit::Months:
    case TimeUnit::Years:
    case TimeUnit::BusinessDays:
      PLUSS_FAIL("Slate::tl().epoch: unit is span-only; use Slate::tl().span");
  }
  PLUSS_FAIL("Slate::tl().epoch: unknown TimeUnit");
}

Slate SlateTimelineAccessor::span(
    const Slate& other,
    const TimelineSpanOptions& options) const {
  const auto left = tl_payload_(base_, "Slate::tl().span");
  const auto right = tl_payload_(other, "Slate::tl().span");
  return tl_span_series_(
      base_,
      other,
      left.span(right, to_codeh_span_options_(options, "Slate::tl().span")),
      "Slate::tl().span");
}

Slate SlateTimelineAccessor::span(const Slate& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

Slate SlateTimelineAccessor::span(
    const pluss::Timeline& other,
    const TimelineSpanOptions& options) const {
  return span(Slate(other), options);
}

Slate SlateTimelineAccessor::span(const pluss::Timeline& other, TimeUnit unit) const {
  TimelineSpanOptions options;
  options.unit = unit;
  return span(other, options);
}

Slate SlateTimelineAccessor::floor(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Slate::tl().floor: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Slate::tl().floor").floor(unit));
}

Slate SlateTimelineAccessor::ceil(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Slate::tl().ceil: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Slate::tl().ceil").ceil(unit));
}

Slate SlateTimelineAccessor::round(std::string_view unit) const {
  PLUSS_CHECK(!unit.empty(), "Slate::tl().round: unit cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Slate::tl().round").round(unit));
}

Slate SlateTimelineAccessor::date() const {
  return tl_temporal_(base_, tl_payload_(base_, "Slate::tl().date").date());
}

Slate SlateTimelineAccessor::time() const {
  return tl_temporal_(base_, tl_payload_(base_, "Slate::tl().time").time());
}

Slate SlateTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  PLUSS_CHECK(!freq.empty(), "Slate::tl().shift: freq cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Slate::tl().shift")
          .shift(static_cast<int>(periods), tl_period_unit_(freq)));
}

Slate SlateTimelineAccessor::period(std::string_view freq) const {
  PLUSS_CHECK(!freq.empty(), "Slate::tl().period: freq cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Slate::tl().period").period_start(tl_period_unit_(freq)));
}

Slate SlateTimelineAccessor::tz(std::string_view name) const {
  PLUSS_CHECK(!name.empty(), "Slate::tl().tz: timezone cannot be empty");
  return tl_temporal_(base_, tl_payload_(base_, "Slate::tl().tz").tz_convert(name));
}

Slate SlateTimelineAccessor::localize(const TimelineLocalizeOptions& options) const {
  PLUSS_CHECK(!options.tz.empty(), "Slate::tl().localize: timezone cannot be empty");
  return tl_temporal_(
      base_,
      tl_payload_(base_, "Slate::tl().localize")
          .tz_localize(options.tz, options.fold, options.nonexistent));
}

Slate SlateTimelineAccessor::utc() const {
  return tl_temporal_(base_,
                      tl_payload_(base_, "Slate::tl().utc").tz_convert("UTC"));
}
