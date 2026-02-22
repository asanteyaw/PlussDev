DataFrame DataFrame::head(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows());
  auto out = slice(0, clipped);
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::tail(std::int64_t n, bool render) const {
  const auto clipped = std::clamp<std::int64_t>(n, 0, nrows());
  auto out = slice(nrows() - clipped, nrows());
  if (render) {
    display::print(std::cout, out);
    out.suppress_next_stream_();
  }
  return out;
}

DataFrame DataFrame::sample(std::int64_t n, std::optional<std::uint64_t> random_state) const {
  const auto total = nrows();
  PLUSS_CHECK(total >= 0, "DataFrame::sample: invalid case count");
  PLUSS_CHECK(n >= 0, "DataFrame::sample: n must be >= 0");
  const auto draw = std::min<std::int64_t>(n, total);

  std::vector<std::int64_t> idx(static_cast<std::size_t>(total));
  std::iota(idx.begin(), idx.end(), 0);

  std::mt19937_64 rng(random_state.value_or(std::random_device{}()));
  std::shuffle(idx.begin(), idx.end(), rng);
  idx.resize(static_cast<std::size_t>(draw));
  return take(idx);
}

DataFrame DataFrame::slice(std::int64_t start, std::int64_t stop) const {
  const auto [s, e] = normalize_slice(start, stop, nrows());
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(e - s));
  for (std::int64_t i = s; i < e; ++i) {
    idx.push_back(i);
  }
  return take(idx);
}

DataFrame DataFrame::take(std::span<const std::int64_t> indices) const {
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.take(indices));
  }
  auto out_case = src.cases_.take(indices);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::where(const pluss::Series& bool_mask) const {
  return filter(bool_mask);
}

DataFrame DataFrame::filter(const pluss::Series& bool_mask) const {
  validate_bool_mask(bool_mask, nrows(), "DataFrame::filter");
  const auto& src = impl_ref_();
  std::vector<Thread> out_cols;
  out_cols.reserve(src.columns_.size());
  for (const auto& c : src.columns_) {
    out_cols.push_back(c.filter(bool_mask));
  }
  auto out_case = src.cases_.filter(bool_mask);
  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), std::move(out_case)));
}

DataFrame DataFrame::masked_select(const pluss::Series& bool_mask) const {
  return filter(bool_mask);
}

DataFrame DataFrame::query(const pluss::Series& bool_mask) const {
  return where(bool_mask);
}

DataFrame DataFrame::query(const QueryFn& predicate) const {
  PLUSS_CHECK(static_cast<bool>(predicate), "DataFrame::query: predicate is empty");
  return where(predicate(*this));
}

DataFrame DataFrame::query(std::string_view expression) const {
  const auto text = owned_name(expression);
  PLUSS_CHECK(!text.empty(), "DataFrame::query: expression cannot be empty");
  auto parser = QueryParser(*this, tokenize_query(text));
  auto eval = parser.parse();

  auto mask = pluss::Series::zeros({nrows()}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  for (std::int64_t i = 0; i < nrows(); ++i) {
    keep[i] = eval(i);
  }
  return where(mask);
}

DataFrame DataFrame::derive(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  DataFrame out = *this;
  for (const auto& assignment : assignments) {
    PLUSS_CHECK(!assignment.first.empty(), "DataFrame::derive: feature name cannot be empty");
    out = out.with_column(assignment.first, assignment.second);
  }
  return out;
}

DataFrame DataFrame::derive(std::initializer_list<DeriveSpec> specs) const {
  DataFrame out = *this;
  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::derive: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::derive: derive expression is empty");
    out = out.with_column(spec.feature, spec.expr(out));
  }
  return out;
}

DataFrame DataFrame::assign(std::initializer_list<std::pair<std::string, Thread>> assignments) const {
  return derive(assignments);
}

DataFrame DataFrame::assign(std::initializer_list<DeriveSpec> specs) const {
  return derive(specs);
}

DataFrame DataFrame::transform(const std::function<Thread(const Thread&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform: transform function is empty");
  const auto& src = impl_ref_();

  auto out_cols = pluss::transmit([&](const Thread& feature) {
    auto transformed = fn(feature);
    PLUSS_CHECK(transformed.defined(), "DataFrame::transform: transform returned undefined Thread");
    PLUSS_CHECK(transformed.size() == src.nrows_,
                "DataFrame::transform: transformed feature length must match DataFrame case count");
    return transformed;
  }, src.columns_);

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "DataFrame::transform(feature): transform function is empty");
  return transform({TransformSpec{owned_name(feature), fn}});
}

DataFrame DataFrame::transform(std::initializer_list<TransformSpec> specs) const {
  PLUSS_CHECK(specs.size() > 0, "DataFrame::transform: at least one TransformSpec is required");
  const auto& src = impl_ref_();

  auto out_cols = src.columns_;
  std::unordered_set<std::string> seen;
  seen.reserve(specs.size());

  for (const auto& spec : specs) {
    PLUSS_CHECK(!spec.feature.empty(), "DataFrame::transform: feature name cannot be empty");
    PLUSS_CHECK(static_cast<bool>(spec.expr), "DataFrame::transform: transform expression is empty");
    PLUSS_CHECK(seen.insert(spec.feature).second,
                "DataFrame::transform: duplicate transform target '" + spec.feature + "'");

    const auto it = src.name_to_idx_.find(spec.feature);
    PLUSS_CHECK(it != src.name_to_idx_.end(), "DataFrame::transform: unknown feature '" + spec.feature + "'");

    auto transformed = spec.expr(out_cols[it->second]);
    PLUSS_CHECK(transformed.defined(), "DataFrame::transform: transform returned undefined Thread");
    PLUSS_CHECK(transformed.size() == src.nrows_,
                "DataFrame::transform: transformed feature length must match DataFrame case count");
    out_cols[it->second] = std::move(transformed);
  }

  return DataFrame(pluss::msharp::make_intrusive<detail::DataFrameImpl>(src.names_, std::move(out_cols), src.cases_));
}

DataFrame DataFrame::apply(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const {
  return transform(feature, fn);
}

DataFrame& DataFrame::apply_(std::string_view feature, const std::function<Thread(const Thread&)>& fn) {
  *this = transform(feature, fn);
  return *this;
}
