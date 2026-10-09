Stream Stream::gather(const std::vector<std::int64_t>& source, std::int64_t shift, bool trim_trailing) const {
  PLUSS_CHECK(defined(), "Stream: undefined handle");
  const std::int64_t n = size();
  const auto rows = static_cast<std::int64_t>(source.size());
  const auto position_ok = [&](std::int64_t r) { return r >= 0 && r + shift >= 0 && r + shift < n; };

  // A row is "absent" when it asks for a record the stream does not have;
  // a row with no partner (-1) is a present null. Only trailing absence is trimmed.
  std::int64_t out_len = rows;
  if (trim_trailing) {
    out_len = 0;
    for (std::int64_t p = rows - 1; p >= 0; --p) {
      const auto r = source[static_cast<std::size_t>(p)];
      if (r < 0 || position_ok(r)) {
        out_len = p + 1;
        break;
      }
    }
  }
  if (out_len == 0) return head(0);

  if (n == 0) {
    // Nothing to take from: an all-null float stream of the right length.
    std::vector<double> zeros(static_cast<std::size_t>(out_len), 0.0);
    std::vector<double> none(static_cast<std::size_t>(out_len), 0.0);
    return detail::with_validity_mask(
        Stream(pluss::make_series(zeros, {out_len})),
        pluss::make_series(none, {out_len}).to(pluss::codem::ScalarType::Bool));
  }

  std::vector<std::int64_t> idx(static_cast<std::size_t>(out_len));
  std::vector<double> present(static_cast<std::size_t>(out_len), 1.0);
  bool all_present = true;
  for (std::int64_t p = 0; p < out_len; ++p) {
    const auto r = source[static_cast<std::size_t>(p)];
    if (position_ok(r)) {
      idx[static_cast<std::size_t>(p)] = r + shift;
    } else {
      idx[static_cast<std::size_t>(p)] = 0;  // placeholder, masked out below
      present[static_cast<std::size_t>(p)] = 0.0;
      all_present = false;
    }
  }
  auto out = take(idx);
  if (all_present) return out;

  // Combine with whatever nullness the taken values already carry.
  if (detail::has_validity_mask(out)) {
    const auto existing = detail::validity_mask(out)->to_vec<bool>();
    for (std::size_t p = 0; p < present.size(); ++p) {
      if (!existing[p]) present[p] = 0.0;
    }
  }
  return detail::with_validity_mask(
      out, pluss::make_series(present, {out_len}).to(pluss::codem::ScalarType::Bool));
}

