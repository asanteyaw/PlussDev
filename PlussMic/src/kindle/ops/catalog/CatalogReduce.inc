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

Catalog reduce_keywise(const Catalog& catalog,
                       std::string_view out_key,
                       const CatalogReduceOptions& options,
                       const std::function<pluss::table::Thread(const pluss::table::DataFrame&,
                                                                const CatalogReduceOptions&)>& reducer,
                       std::string_view where) {
  PLUSS_CHECK(catalog.defined(), std::string(where) + ": undefined handle");
  PLUSS_CHECK(static_cast<bool>(reducer), std::string(where) + ": reducer is empty");
  if (catalog.empty()) return Catalog{};

  require_series_streams(catalog, where);
  const auto frame = catalog.tabelize(reduce_tabelize_policy(options));
  const auto reduced = reducer(frame, options);
  return Catalog(std::vector<std::pair<std::string, Stream>>{
      {std::string(out_key), Stream::from_thread(reduced)},
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
          std::function<pluss::table::Thread(const pluss::table::DataFrame&, const CatalogReduceOptions&)>(
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

PLUSS_CATALOG_REDUCE_OVERLOADS(sum)
Catalog Catalog::sum(const CatalogReduceOptions& options) const {
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
