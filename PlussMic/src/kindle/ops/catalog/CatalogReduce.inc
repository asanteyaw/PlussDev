namespace {

CatalogReduceOptions with_reduce_axis(CatalogReduceOptions options, ReduceAxis axis) {
  options.axis = axis;
  return options;
}

Catalog reduce_recordwise(const Catalog& catalog,
                          const std::function<Stream(const Stream&)>& reducer) {
  PLUSS_CHECK(catalog.defined(), "Catalog: undefined handle");
  PLUSS_CHECK(static_cast<bool>(reducer), "Catalog reduction: reducer is empty");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(catalog.nkeys()));
  for (const auto& key : catalog.keys()) {
    out.emplace_back(key, reducer(catalog.keys(key)));
  }
  return Catalog(std::move(out));
}

TabelizePolicy reduce_tabelize_policy(const CatalogReduceOptions& options) {
  TabelizePolicy policy;
  policy.align = options.align;
  policy.align_by = options.align_by;
  policy.fill = options.fill_scalar.has_value() ? FillPolicy::Scalar : FillPolicy::Null;
  policy.fill_scalar = options.fill_scalar;
  return policy;
}

void require_series_streams(const Catalog& catalog, std::string_view where) {
  for (const auto& key : catalog.keys()) {
    PLUSS_CHECK(catalog.keys(key).kind() == StreamKind::Series,
                std::string(where) + ": key '" + key + "' must be a Series stream for cross-key reduction");
  }
}


// ---------------------------------------------------------------------------
// Jagged cross-key reduction: reduce position i over the streams that have a
// value there. One pass over each stream, no padded copy; positions are
// processed in cache-sized blocks (the accumulators for a block stay hot while
// every stream is swept) and blocks run in parallel for large inputs.
// ---------------------------------------------------------------------------

enum class JaggedOp { Sum, Mean, Min, Max, Var, Std, Count };

struct JaggedColumn {
  ColumnView<double> values;  // zero-copy when the stream already is contiguous Float64 on the CPU
  std::vector<std::uint8_t> valid;  // 1 = usable (in range, non-null, non-NaN)
  std::int64_t size{0};
};

JaggedColumn load_jagged_column(const Stream& stream) {
  JaggedColumn col;
  col.values = ColumnView<double>(stream.as_series(), pluss::codem::ScalarType::Float64);
  col.size = static_cast<std::int64_t>(col.values.size());
  col.valid.assign(col.values.size(), 1);
  if (pluss::kindle::detail::has_validity_mask(stream)) {
    const auto mask = pluss::kindle::detail::validity_mask(stream)->host<bool>();
    for (std::size_t j = 0; j < mask.size() && j < col.valid.size(); ++j) col.valid[j] = mask[j] ? 1 : 0;
  }
  for (std::size_t j = 0; j < col.values.size(); ++j) {
    if (std::isnan(col.values[j])) col.valid[j] = 0;
  }
  return col;
}

// Cross-key reductions are decided by a RaggedPolicy (OpClass::Reduce defaults).
// The jagged kernel handles every domain (Longest, Shortest, Exact) and both
// head and tail alignment without padding; label alignment and a fill value
// need the padding path (tabelize) and are routed there.
bool jagged_applicable(const CatalogReduceOptions& options) {
  const bool custom = options.ended != RaggedEnd::Skip || options.anchor != RaggedAnchor::Head ||
                      options.min_streams != 1;
  PLUSS_CHECK(options.min_streams >= 1, "Catalog reduction: min_streams must be >= 1");
  if (options.axis != ReduceAxis::Key) {
    PLUSS_CHECK(!custom, "Catalog reduction: ended, anchor and min_streams apply only to ReduceAxis::Key");
    return false;
  }
  const bool native = (options.align == Align::Outer || options.align == Align::Inner ||
                       options.align == Align::Strict) &&
                      options.align_by == AlignBy::Pos && !options.fill_scalar.has_value();
  PLUSS_CHECK(native || !custom,
              "Catalog reduction: ended, anchor and min_streams require Align::Outer/Inner/Strict, "
              "AlignBy::Pos and no fill_scalar");
  return native;
}

// Several reductions over the same catalog in ONE pass: the columns are loaded
// once, the layout is computed once, and every index is visited once while the
// accumulators it needs (sum, count, min, max, squared deviations) stay hot.
// `ops` may repeat nothing and must be non-empty; results come back in order.
std::vector<Stream> jagged_reduce_many(const Catalog& catalog, const std::vector<JaggedOp>& ops,
                                       const CatalogReduceOptions& options) {
  constexpr std::int64_t kBlock = 8192;  // positions per block: acc arrays stay in L1/L2
  PLUSS_CHECK(!ops.empty(), "Catalog::reduce: no reductions requested");
  const auto policy = to_ragged(options);
  const auto layout = RaggedLayout::of(catalog, policy, "Catalog reduction");

  bool need_sum = false, need_min = false, need_max = false, need_ss = false;
  for (const auto op : ops) {
    need_sum = need_sum || op == JaggedOp::Sum || op == JaggedOp::Mean || op == JaggedOp::Var ||
               op == JaggedOp::Std;
    need_min = need_min || op == JaggedOp::Min;
    need_max = need_max || op == JaggedOp::Max;
    need_ss = need_ss || op == JaggedOp::Var || op == JaggedOp::Std;
  }

  // Columns are independent: load them in parallel (conversion + mask scan).
  std::vector<Stream> inputs;
  inputs.reserve(static_cast<std::size_t>(catalog.nkeys()));
  for (const auto& key : catalog.keys()) inputs.push_back(catalog.keys(key));
  std::vector<JaggedColumn> cols(inputs.size());
  pluss::msharp::parallel_for(0, static_cast<std::int64_t>(inputs.size()), 1,
                              [&](std::int64_t a, std::int64_t b) {
    for (std::int64_t c = a; c < b; ++c) cols[static_cast<std::size_t>(c)] = load_jagged_column(inputs[static_cast<std::size_t>(c)]);
  });
  const std::int64_t width = layout.domain();
  const auto n = static_cast<std::size_t>(width);
  const auto nstreams = static_cast<std::int64_t>(cols.size());

  // Stream c covers result indices [rel[c], rel[c] + size): its first record
  // sits `rel[c]` past the start of the window (negative when the window starts
  // inside the stream).
  std::vector<std::int64_t> rel(cols.size());
  for (std::size_t c = 0; c < cols.size(); ++c) rel[c] = layout.offset[c] - layout.window_begin;

  std::vector<double> sum, mn, mx, ss, cnt(n, 0.0);
  if (need_sum) sum.assign(n, 0.0);
  if (need_min) mn.assign(n, std::numeric_limits<double>::infinity());
  if (need_max) mx.assign(n, -std::numeric_limits<double>::infinity());
  if (need_ss) ss.assign(n, 0.0);
  std::vector<std::uint8_t> poison;
  const bool propagate_nulls = policy.nulls == RaggedNulls::Propagate;
  if (propagate_nulls) poison.assign(n, 0);

  const std::int64_t work = width * nstreams;
  const std::int64_t grain = work >= (1 << 18) ? kBlock : width + 1;  // serial when small

  // One sweep of a stream's slice [lo, hi); the flags are compile-time so each
  // combination is a tight loop the compiler can vectorize.
  auto sweep = [&](auto sum_tag, auto min_tag, auto max_tag, const JaggedColumn& col, std::int64_t off,
                   std::int64_t lo, std::int64_t hi) {
    constexpr bool S = decltype(sum_tag)::value, Mn = decltype(min_tag)::value, Mx = decltype(max_tag)::value;
    const double* x = col.values.data();
    const std::uint8_t* ok = col.valid.data();
    double* C = cnt.data();
    double* Sm = need_sum ? sum.data() : nullptr;
    double* Lo = need_min ? mn.data() : nullptr;
    double* Hi = need_max ? mx.data() : nullptr;
    for (std::int64_t j = lo; j < hi; ++j) {
      const std::int64_t i = j - off;
      if constexpr (S) Sm[j] += ok[i] ? x[i] : 0.0;
      if constexpr (Mn) Lo[j] = ok[i] ? std::min(Lo[j], x[i]) : Lo[j];
      if constexpr (Mx) Hi[j] = ok[i] ? std::max(Hi[j], x[i]) : Hi[j];
      C[j] += ok[i];
    }
  };
  using T = std::true_type;
  using F = std::false_type;
  auto pick = [](bool v, auto&& f) {
    if (v) f(T{});
    else f(F{});
  };

  pluss::msharp::parallel_for(0, width, grain, [&](std::int64_t a, std::int64_t b) {
    for (std::size_t c = 0; c < cols.size(); ++c) {
      const auto& col = cols[c];
      const std::int64_t off = rel[c];
      const std::int64_t lo = std::max(a, off);
      const std::int64_t hi = std::min(b, off + col.size);
      if (lo >= hi) continue;
      pick(need_sum, [&](auto st) {
        pick(need_min, [&](auto mt) {
          pick(need_max, [&](auto xt) { sweep(st, mt, xt, col, off, lo, hi); });
        });
      });
      if (propagate_nulls) {
        for (std::int64_t j = lo; j < hi; ++j) {
          poison[static_cast<std::size_t>(j)] |= static_cast<std::uint8_t>(!col.valid[static_cast<std::size_t>(j - off)]);
        }
      }
    }
  });

  if (need_ss) {
    pluss::msharp::parallel_for(0, width, grain, [&](std::int64_t a, std::int64_t b) {
      for (std::size_t c = 0; c < cols.size(); ++c) {
        const auto& col = cols[c];
        const std::int64_t off = rel[c];
        const std::int64_t lo = std::max(a, off);
        const std::int64_t hi = std::min(b, off + col.size);
        const double* x = col.values.data();
        const std::uint8_t* ok = col.valid.data();
        for (std::int64_t j = lo; j < hi; ++j) {
          const std::int64_t i = j - off;
          const auto u = static_cast<std::size_t>(j);
          const double mean = cnt[u] > 0.0 ? sum[u] / cnt[u] : 0.0;
          const double d = ok[i] ? x[i] - mean : 0.0;
          ss[u] += d * d;
        }
      }
    });
  }

  // How many streams reach each index (finished streams don't).
  std::vector<std::int64_t> present;
  const bool absent_nulls = policy.absent == RaggedAbsent::Null;
  if (absent_nulls) present = layout.presence();

  const double ddof = options.unbiased ? 1.0 : 0.0;
  const auto need = static_cast<double>(policy.min_support);

  // Positions that fail the policy (too few contributors, a null under
  // Propagate, a finished stream under absent=Null) are shared by all ops.
  std::vector<std::uint8_t> base_ok(n, 1);
  for (std::size_t j = 0; j < n; ++j) {
    bool ok = cnt[j] >= need;
    if (propagate_nulls && poison[j]) ok = false;
    if (absent_nulls && present[j] != nstreams) ok = false;
    base_ok[j] = ok ? 1 : 0;
  }

  std::vector<Stream> results;
  results.reserve(ops.size());
  for (const auto op : ops) {
    std::vector<double> out(n, 0.0);
    std::vector<double> valid_out(n, 1.0);
    bool any_invalid = false;
    for (std::size_t j = 0; j < n; ++j) {
      bool ok = base_ok[j] != 0;
      double v = 0.0;
      switch (op) {
        case JaggedOp::Sum: v = sum[j]; break;
        case JaggedOp::Mean: v = ok ? sum[j] / cnt[j] : 0.0; break;
        case JaggedOp::Min: v = mn[j]; break;
        case JaggedOp::Max: v = mx[j]; break;
        case JaggedOp::Count: v = cnt[j]; break;
        case JaggedOp::Var:
        case JaggedOp::Std: {
          ok = ok && cnt[j] - ddof > 0.0;
          const double var = ok ? ss[j] / (cnt[j] - ddof) : 0.0;
          v = op == JaggedOp::Std ? std::sqrt(var) : var;
          break;
        }
      }
      // Count is a fact about the data, not a statistic: it is defined everywhere.
      if (op == JaggedOp::Count) ok = true;
      out[j] = ok ? v : 0.0;
      valid_out[j] = ok ? 1.0 : 0.0;
      any_invalid = any_invalid || !ok;
    }
    Stream result(pluss::make_series(out, {width}));
    if (any_invalid) {
      result = pluss::kindle::detail::with_validity_mask(
          result, pluss::make_series(valid_out, {width}).to(pluss::codem::ScalarType::Bool));
    }
    results.push_back(std::move(result));
  }
  return results;
}

Stream jagged_reduce(const Catalog& catalog, JaggedOp op, const CatalogReduceOptions& options) {
  return jagged_reduce_many(catalog, {op}, options).front();
}

Catalog reduce_jagged(const Catalog& catalog, std::string_view out_key, JaggedOp op,
                      const CatalogReduceOptions& options, std::string_view where) {
  PLUSS_CHECK(catalog.defined(), std::string(where) + ": undefined handle");
  if (catalog.empty()) return Catalog{};
  require_series_streams(catalog, where);
  return Catalog(std::vector<std::pair<std::string, Stream>>{
      {std::string(out_key), jagged_reduce(catalog, op, options)},
  });
}

Catalog reduce_keywise(const Catalog& catalog,
                       std::string_view out_key,
                       const CatalogReduceOptions& options,
                       const std::function<pluss::table::Slate(const pluss::table::DataFrame&,
                                                                const CatalogReduceOptions&)>& reducer,
                       std::string_view where) {
  PLUSS_CHECK(catalog.defined(), std::string(where) + ": undefined handle");
  PLUSS_CHECK(static_cast<bool>(reducer), std::string(where) + ": reducer is empty");
  if (catalog.empty()) return Catalog{};

  require_series_streams(catalog, where);
  const auto frame = catalog.tabelize(reduce_tabelize_policy(options));
  const auto reduced = reducer(frame, options);
  return Catalog(std::vector<std::pair<std::string, Stream>>{
      {std::string(out_key), Stream::from_slate(reduced)},
  });
}

template <class RecordFn, class KeyFn>
Catalog reduce_catalog(const Catalog& catalog,
                       std::string_view out_key,
                       const CatalogReduceOptions& options,
                       RecordFn&& record_fn,
                       KeyFn&& key_fn,
                       std::string_view where) {
  switch (options.axis) {
    case ReduceAxis::Record:
      return reduce_recordwise(
          catalog,
          std::function<Stream(const Stream&)>(std::forward<RecordFn>(record_fn)));
    case ReduceAxis::Key:
      return reduce_keywise(
          catalog,
          out_key,
          options,
          std::function<pluss::table::Slate(const pluss::table::DataFrame&, const CatalogReduceOptions&)>(
              std::forward<KeyFn>(key_fn)),
          where);
  }
  PLUSS_FAIL(std::string(where) + ": unsupported reduction axis");
}

} // namespace

#define PLUSS_CATALOG_REDUCE_OVERLOADS(method) \
  Catalog Catalog::method() const { \
    return method(CatalogReduceOptions{}); \
  } \
  Catalog Catalog::method(ReduceAxis axis) const { \
    return method(with_reduce_axis(CatalogReduceOptions{}, axis)); \
  }

Catalog Catalog::reduce(std::initializer_list<std::string_view> ops, const CatalogReduceOptions& options) const {
  return reduce(std::vector<std::string>(ops.begin(), ops.end()), options);
}

Catalog Catalog::reduce(const std::vector<std::string>& ops, const CatalogReduceOptions& requested) const {
  // Fused reductions are always across keys, whatever `axis` says.
  auto options = requested;
  options.axis = ReduceAxis::Key;
  PLUSS_CHECK(defined(), "Catalog::reduce: undefined handle");
  PLUSS_CHECK(!ops.empty(), "Catalog::reduce: no reductions requested");
  std::vector<JaggedOp> kinds;
  for (std::size_t i = 0; i < ops.size(); ++i) {
    const auto& name = ops[i];
    for (std::size_t j = 0; j < i; ++j) {
      PLUSS_CHECK(ops[j] != name, "Catalog::reduce: '" + name + "' requested twice");
    }
    if (name == "sum") kinds.push_back(JaggedOp::Sum);
    else if (name == "mean") kinds.push_back(JaggedOp::Mean);
    else if (name == "min") kinds.push_back(JaggedOp::Min);
    else if (name == "max") kinds.push_back(JaggedOp::Max);
    else if (name == "var") kinds.push_back(JaggedOp::Var);
    else if (name == "std") kinds.push_back(JaggedOp::Std);
    else if (name == "count") kinds.push_back(JaggedOp::Count);
    else PLUSS_FAIL("Catalog::reduce: unknown reduction '" + name + "' (use sum, mean, min, max, var, std, count)");
  }
  PLUSS_CHECK(jagged_applicable(options),
              "Catalog::reduce: label alignment and fill_scalar need the padding path; call the single reductions");
  if (empty()) return Catalog{};
  require_series_streams(*this, "Catalog::reduce");
  auto streams = jagged_reduce_many(*this, kinds, options);
  std::vector<std::pair<std::string, Stream>> out;
  for (std::size_t i = 0; i < ops.size(); ++i) out.emplace_back(ops[i], std::move(streams[i]));
  return Catalog(std::move(out));
}

Catalog Catalog::combine(std::string_view lhs, std::string_view rhs, std::string_view op,
                         std::string_view out_key, const RaggedPolicy& policy) const {
  PLUSS_CHECK(defined(), "Catalog::combine: undefined handle");
  const auto a = get(lhs), b = get(rhs);
  PLUSS_CHECK(a.has_value(), "Catalog::combine: unknown key '" + std::string(lhs) + "'");
  PLUSS_CHECK(b.has_value(), "Catalog::combine: unknown key '" + std::string(rhs) + "'");
  const std::string name = out_key.empty() ? std::string(lhs) + std::string(op) + std::string(rhs) : std::string(out_key);
  return set(name, ragged_binary(*a, *b, op, policy));
}

PLUSS_CATALOG_REDUCE_OVERLOADS(sum)
Catalog Catalog::sum(const CatalogReduceOptions& options) const {
  if (jagged_applicable(options)) {
    return reduce_jagged(*this, "sum", JaggedOp::Sum, options, "Catalog::sum");
  }
  return reduce_catalog(
      *this,
      "sum",
      options,
      [](const Stream& stream) { return stream.sum(); },
      [](const pluss::table::DataFrame& frame, const CatalogReduceOptions& reduce) {
        return frame.sum(pluss::table::Axis::Feature, reduce.skipnull);
      },
      "Catalog::sum");
}

PLUSS_CATALOG_REDUCE_OVERLOADS(mean)
Catalog Catalog::mean(const CatalogReduceOptions& options) const {
  if (jagged_applicable(options)) {
    return reduce_jagged(*this, "mean", JaggedOp::Mean, options, "Catalog::mean");
  }
  return reduce_catalog(
      *this,
      "mean",
      options,
      [](const Stream& stream) { return stream.mean(); },
      [](const pluss::table::DataFrame& frame, const CatalogReduceOptions& reduce) {
        return frame.mean(pluss::table::Axis::Feature, reduce.skipnull);
      },
      "Catalog::mean");
}

PLUSS_CATALOG_REDUCE_OVERLOADS(min)
Catalog Catalog::min(const CatalogReduceOptions& options) const {
  if (jagged_applicable(options)) {
    return reduce_jagged(*this, "min", JaggedOp::Min, options, "Catalog::min");
  }
  return reduce_catalog(
      *this,
      "min",
      options,
      [](const Stream& stream) { return stream.min(); },
      [](const pluss::table::DataFrame& frame, const CatalogReduceOptions& reduce) {
        return frame.min(pluss::table::Axis::Feature, reduce.skipnull);
      },
      "Catalog::min");
}

PLUSS_CATALOG_REDUCE_OVERLOADS(max)
Catalog Catalog::max(const CatalogReduceOptions& options) const {
  if (jagged_applicable(options)) {
    return reduce_jagged(*this, "max", JaggedOp::Max, options, "Catalog::max");
  }
  return reduce_catalog(
      *this,
      "max",
      options,
      [](const Stream& stream) { return stream.max(); },
      [](const pluss::table::DataFrame& frame, const CatalogReduceOptions& reduce) {
        return frame.max(pluss::table::Axis::Feature, reduce.skipnull);
      },
      "Catalog::max");
}

PLUSS_CATALOG_REDUCE_OVERLOADS(var)
Catalog Catalog::var(const CatalogReduceOptions& options) const {
  if (jagged_applicable(options)) {
    return reduce_jagged(*this, "var", JaggedOp::Var, options, "Catalog::var");
  }
  return reduce_catalog(
      *this,
      "var",
      options,
      [&](const Stream& stream) { return stream.var(options.unbiased); },
      [](const pluss::table::DataFrame& frame, const CatalogReduceOptions& reduce) {
        return frame.var(
            pluss::table::Axis::Feature,
            pluss::table::StatisticOptions{
                .ddof = reduce.unbiased ? 1 : 0,
                .min_count = 1,
            });
      },
      "Catalog::var");
}

PLUSS_CATALOG_REDUCE_OVERLOADS(std)
Catalog Catalog::std(const CatalogReduceOptions& options) const {
  if (jagged_applicable(options)) {
    return reduce_jagged(*this, "std", JaggedOp::Std, options, "Catalog::std");
  }
  return reduce_catalog(
      *this,
      "std",
      options,
      [&](const Stream& stream) { return stream.std(options.unbiased); },
      [](const pluss::table::DataFrame& frame, const CatalogReduceOptions& reduce) {
        return frame.std(
            pluss::table::Axis::Feature,
            pluss::table::StatisticOptions{
                .ddof = reduce.unbiased ? 1 : 0,
                .min_count = 1,
            });
      },
      "Catalog::std");
}

#undef PLUSS_CATALOG_REDUCE_OVERLOADS
