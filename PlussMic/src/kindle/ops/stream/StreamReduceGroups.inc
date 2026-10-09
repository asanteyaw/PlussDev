Stream Stream::reduce_groups(const std::vector<std::int64_t>& source, std::int64_t shift,
                             const std::vector<std::int64_t>& offsets, GroupReduce op, bool unbiased) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  PLUSS_CHECK(!offsets.empty() && offsets.front() == 0 &&
                  offsets.back() == static_cast<std::int64_t>(source.size()),
              "Stream::reduce_groups: offsets must start at 0 and end at source.size()");
  for (std::size_t g = 1; g < offsets.size(); ++g) {
    PLUSS_CHECK(offsets[g] >= offsets[g - 1], "Stream::reduce_groups: offsets must be non-decreasing");
  }
  const auto k = kind();
  const bool counting = op == GroupReduce::Count || op == GroupReduce::Ndistinct;
  PLUSS_CHECK(counting || k == StreamKind::Series,
              "Stream::reduce_groups: this reduction needs a numeric Series stream");
  PLUSS_CHECK(k == StreamKind::Series || k == StreamKind::Note || k == StreamKind::Timeline,
              "Stream::reduce_groups: unsupported stream kind");

  const std::int64_t n = size();
  const auto groups = static_cast<std::int64_t>(offsets.size()) - 1;
  const std::vector<bool> validity =
      detail::has_validity_mask(*this) ? detail::validity_mask(*this)->to_vec<bool>() : std::vector<bool>{};

  std::vector<double> numbers;
  std::vector<std::int64_t> times;
  std::vector<std::string_view> words;
  if (k == StreamKind::Series) {
    numbers = as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  } else if (k == StreamKind::Timeline) {
    times = as_timeline().epoch_nanos().to_vec<std::int64_t>();
  } else {
    words = as_note().vector();
  }
  const auto usable = [&](std::int64_t p) {
    if (p < 0 || p >= n) return false;
    if (!validity.empty() && !validity[static_cast<std::size_t>(p)]) return false;
    return k != StreamKind::Series || !std::isnan(numbers[static_cast<std::size_t>(p)]);
  };

  std::vector<double> value(static_cast<std::size_t>(groups), 0.0);
  std::vector<std::int64_t> count(static_cast<std::size_t>(groups), 0);
  std::vector<std::uint8_t> ok(static_cast<std::size_t>(groups), 1);
  const double ddof = unbiased ? 1.0 : 0.0;

  pluss::msharp::parallel_for(0, groups, 256, [&](std::int64_t g0, std::int64_t g1) {
    std::vector<double> buf;
    std::vector<std::int64_t> ibuf;
    std::unordered_set<std::string_view> seen;
    for (std::int64_t g = g0; g < g1; ++g) {
      const auto lo = offsets[static_cast<std::size_t>(g)], hi = offsets[static_cast<std::size_t>(g) + 1];
      buf.clear();
      ibuf.clear();
      seen.clear();
      std::int64_t c = 0;
      for (auto e = lo; e < hi; ++e) {
        const auto p = source[static_cast<std::size_t>(e)] + shift;
        if (!usable(p)) continue;
        ++c;
        if (k == StreamKind::Series) buf.push_back(numbers[static_cast<std::size_t>(p)]);
        else if (k == StreamKind::Timeline) ibuf.push_back(times[static_cast<std::size_t>(p)]);
        else if (op == GroupReduce::Ndistinct) seen.insert(words[static_cast<std::size_t>(p)]);
      }
      count[static_cast<std::size_t>(g)] = c;
      const auto u = static_cast<std::size_t>(g);
      if (op == GroupReduce::Count) {
        continue;
      }
      if (op == GroupReduce::Ndistinct) {
        if (k == StreamKind::Series) {
          std::sort(buf.begin(), buf.end());
          count[u] = static_cast<std::int64_t>(std::unique(buf.begin(), buf.end()) - buf.begin());
        } else if (k == StreamKind::Timeline) {
          std::sort(ibuf.begin(), ibuf.end());
          count[u] = static_cast<std::int64_t>(std::unique(ibuf.begin(), ibuf.end()) - ibuf.begin());
        } else {
          count[u] = static_cast<std::int64_t>(seen.size());
        }
        continue;
      }
      if (c == 0) {
        ok[u] = 0;
        continue;
      }
      const double cnt = static_cast<double>(c);
      switch (op) {
        case GroupReduce::Sum: {
          double s = 0.0;
          for (const double x : buf) s += x;
          value[u] = s;
          break;
        }
        case GroupReduce::Mean: {
          double s = 0.0;
          for (const double x : buf) s += x;
          value[u] = s / cnt;
          break;
        }
        case GroupReduce::Min:
          value[u] = *std::min_element(buf.begin(), buf.end());
          break;
        case GroupReduce::Max:
          value[u] = *std::max_element(buf.begin(), buf.end());
          break;
        case GroupReduce::Median: {
          const auto mid = buf.begin() + static_cast<std::ptrdiff_t>(c / 2);
          std::nth_element(buf.begin(), mid, buf.end());
          if (c % 2 == 1) {
            value[u] = *mid;
          } else {
            const double upper = *mid;
            const double lower = *std::max_element(buf.begin(), mid);
            value[u] = 0.5 * (lower + upper);
          }
          break;
        }
        case GroupReduce::Var:
        case GroupReduce::Std: {
          if (cnt - ddof <= 0.0) {
            ok[u] = 0;
            break;
          }
          double s = 0.0;
          for (const double x : buf) s += x;
          const double mean = s / cnt;
          double ss = 0.0;
          for (const double x : buf) ss += (x - mean) * (x - mean);
          const double var = ss / (cnt - ddof);
          value[u] = op == GroupReduce::Std ? std::sqrt(var) : var;
          break;
        }
        default:
          break;
      }
    }
  });

  if (counting) {
    return Stream(pluss::make_series(count, {groups}));
  }
  Stream out(pluss::make_series(value, {groups}));
  if (std::find(ok.begin(), ok.end(), std::uint8_t{0}) != ok.end()) {
    std::vector<double> mask(ok.begin(), ok.end());
    out = detail::with_validity_mask(out, pluss::make_series(mask, {groups}).to(pluss::codem::ScalarType::Bool));
  }
  return out;
}

