namespace {

std::string group_owned_name_(std::string_view value) {
  return std::string(value.data(), value.size());
}

pluss::Series group_bool_mask_(const std::vector<bool>& values,
                               pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  auto out = pluss::Series::empty(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Bool,
      platform);
  auto* raw = reinterpret_cast<bool*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    raw[i] = values[i];
  }
  return out;
}

Thread group_i64_thread_(const std::vector<std::int64_t>& values) {
  auto out = pluss::Series::empty(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Int64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(std::int64_t));
  }
  return Thread(out);
}

Thread group_f64_thread_(const std::vector<double>& values,
                         const std::vector<bool>& valid = {}) {
  auto out = pluss::Series::empty(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Float64);
  if (!values.empty()) {
    std::memcpy(out.raw_data(), values.data(), values.size() * sizeof(double));
  }
  if (valid.empty()) {
    return Thread(out);
  }
  return detail::with_validity_mask(Thread(out), group_bool_mask_(valid));
}

Thread group_note_thread_(const std::vector<std::string>& values,
                          const std::vector<bool>& valid = {}) {
  std::vector<std::string_view> views;
  views.reserve(values.size());
  for (const auto& value : values) {
    views.push_back(value);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto out = Thread(pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size())));
  if (!valid.empty()) {
    out = detail::with_validity_mask(out, group_bool_mask_(valid));
  }
  return out;
}

pluss::codem::Platform group_thread_platform_(const Thread& value) {
  switch (value.kind()) {
    case ThreadKind::Series:
      return value.as_series().platform();
    case ThreadKind::Timeline:
      return value.as_timeline().epoch_nanos().platform();
    case ThreadKind::TimeIndex:
      return value.as_timeindex().timeline().epoch_nanos().platform();
    case ThreadKind::Note:
      return pluss::codem::Platform::CPU;
  }
  return pluss::codem::Platform::CPU;
}

std::string group_key_name_(std::size_t index, const Thread& key) {
  if (key.named() && !key.name().empty()) {
    return key.name();
  }
  return index == 0 ? std::string("key") : fmt::format("key_{}", index + 1);
}

std::string group_row_part_(const Thread& value, std::int64_t row) {
  if (!detail::valid_at(value, row)) {
    return std::string("__NULL__");
  }
  return thread_scalar_string_at(value, row);
}

std::string group_row_token_(std::span<const Thread> keys, std::int64_t row) {
  std::string token;
  for (const auto& key : keys) {
    const auto part = group_row_part_(key, row);
    token.append(part);
    token.push_back('\x1E');
  }
  return token;
}

using GroupState_ = detail::GroupStateCache;

Thread group_count_thread_(const GroupState_& state,
                           std::optional<Thread> value = std::nullopt);
Thread group_ndistinct_thread_(const Thread& value, const GroupState_& state);
Thread group_note_mode_thread_(const Thread& value, const GroupState_& state);
Thread group_numeric_thread_(const Thread& value,
                             const GroupState_& state,
                             GroupOp op);

std::string group_op_token_(GroupOp op) {
  switch (op) {
    case GroupOp::Count:
      return "count";
    case GroupOp::Ndistinct:
      return "ndistinct";
    case GroupOp::Sum:
      return "sum";
    case GroupOp::Mean:
      return "mean";
    case GroupOp::Median:
      return "median";
    case GroupOp::Min:
      return "min";
    case GroupOp::Max:
      return "max";
    case GroupOp::Std:
      return "std";
    case GroupOp::Var:
      return "var";
    case GroupOp::Mode:
      return "mode";
    default:
      return "group";
  }
}

std::string group_value_output_name_(std::string_view value_name, GroupOp op) {
  const auto op_name = group_op_token_(op);
  if (value_name.empty()) {
    return op_name;
  }
  return fmt::format("{}_{}", value_name, op_name);
}

GroupState_ build_group_state_(std::span<const Thread> keys,
                               std::span<const std::string> names,
                               std::int64_t nrows,
                               const ThreadGroupOptions& options,
                               std::string_view where) {
  PLUSS_CHECK(!keys.empty(), std::string(where) + ": expected at least one key");
  PLUSS_CHECK(keys.size() == names.size(), std::string(where) + ": key/name size mismatch");

  GroupState_ state;
  state.keys.assign(keys.begin(), keys.end());
  state.names.assign(names.begin(), names.end());
  state.included.assign(static_cast<std::size_t>(nrows), true);
  state.group_ids.assign(static_cast<std::size_t>(nrows), -1);

  for (const auto& key : keys) {
    PLUSS_CHECK(key.defined(), std::string(where) + ": key thread is undefined");
    PLUSS_CHECK(key.size() == nrows, std::string(where) + ": key length mismatch");
    if (!options.dropnull || !detail::has_validity_mask(key)) {
      continue;
    }
    const auto valid = detail::validity_mask(key)->to_vec<bool>();
    std::transform(
        state.included.begin(),
        state.included.end(),
        valid.begin(),
        state.included.begin(),
        std::logical_and<bool>{});
  }
  state.all_included = std::all_of(
      state.included.begin(),
      state.included.end(),
      [](bool value) { return value; });

  std::unordered_map<std::string, std::size_t> index_by_key;
  index_by_key.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, nrows / 8)));
  for (std::int64_t row = 0; row < nrows; ++row) {
    if (!state.included[static_cast<std::size_t>(row)]) {
      continue;
    }
    const auto token = group_row_token_(keys, row);
    const auto [it, inserted] = index_by_key.emplace(token, state.first_rows.size());
    if (inserted) {
      state.first_rows.push_back(row);
    }
    state.group_ids[static_cast<std::size_t>(row)] = static_cast<std::int64_t>(it->second);
  }

  state.ngroups = static_cast<std::int64_t>(state.first_rows.size());
  state.order.resize(static_cast<std::size_t>(state.ngroups));
  std::iota(state.order.begin(), state.order.end(), 0);

  if (options.sort && state.ngroups > 1) {
    std::stable_sort(
        state.order.begin(),
        state.order.end(),
        [&](std::int64_t lhs_group, std::int64_t rhs_group) {
          const auto lhs_row = state.first_rows[static_cast<std::size_t>(lhs_group)];
          const auto rhs_row = state.first_rows[static_cast<std::size_t>(rhs_group)];
          for (const auto& key : keys) {
            const auto lhs = group_row_part_(key, lhs_row);
            const auto rhs = group_row_part_(key, rhs_row);
            if (lhs < rhs) {
              return true;
            }
            if (lhs > rhs) {
              return false;
            }
          }
          return false;
        });
  }

  state.order_positions.assign(static_cast<std::size_t>(state.ngroups), 0);
  state.ordered_first_rows.resize(static_cast<std::size_t>(state.ngroups));
  for (std::size_t sorted_pos = 0; sorted_pos < state.order.size(); ++sorted_pos) {
    const auto group_id = state.order[sorted_pos];
    state.order_positions[static_cast<std::size_t>(group_id)] =
        static_cast<std::int64_t>(sorted_pos);
    state.ordered_first_rows[sorted_pos] =
        state.first_rows[static_cast<std::size_t>(group_id)];
  }

  state.broadcast_positions.assign(static_cast<std::size_t>(nrows), 0);
  if (state.ngroups > 0) {
    for (std::int64_t row = 0; row < nrows; ++row) {
      if (!state.included[static_cast<std::size_t>(row)]) {
        continue;
      }
      const auto group_id = state.group_ids[static_cast<std::size_t>(row)];
      if (group_id < 0) {
        continue;
      }
      state.broadcast_positions[static_cast<std::size_t>(row)] =
          state.order_positions[static_cast<std::size_t>(group_id)];
    }
  }

  return state;
}

Thread reorder_group_thread_(const Thread& value, const GroupState_& state) {
  if (state.order.empty()) {
    return value;
  }
  return value.take(state.order);
}

std::vector<std::pair<std::string, Thread>> group_key_columns_(const GroupState_& state) {
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(state.keys.size());
  for (std::size_t i = 0; i < state.keys.size(); ++i) {
    out.emplace_back(state.names[i], state.keys[i].take(state.ordered_first_rows));
  }
  return out;
}

Thread group_empty_transform_thread_(const Thread& value,
                                     GroupOp op,
                                     std::int64_t size) {
  switch (op) {
    case GroupOp::Count:
    case GroupOp::Ndistinct:
      return detail::with_validity_mask(
          group_i64_thread_(std::vector<std::int64_t>(static_cast<std::size_t>(size), 0)),
          group_bool_mask_(std::vector<bool>(static_cast<std::size_t>(size), false)));
    case GroupOp::Mode:
      if (value.kind() == ThreadKind::Note) {
        return group_note_thread_(
            std::vector<std::string>(static_cast<std::size_t>(size)),
            std::vector<bool>(static_cast<std::size_t>(size), false));
      }
      [[fallthrough]];
    case GroupOp::Sum:
    case GroupOp::Mean:
    case GroupOp::Median:
    case GroupOp::Min:
    case GroupOp::Max:
    case GroupOp::Std:
    case GroupOp::Var:
      return group_f64_thread_(
          std::vector<double>(static_cast<std::size_t>(size), 0.0),
          std::vector<bool>(static_cast<std::size_t>(size), false));
  }
  PLUSS_FAIL("group: unsupported transform op");
}

Thread group_broadcast_thread_(const Thread& grouped, const GroupState_& state) {
  PLUSS_CHECK(grouped.size() == state.ngroups,
              "group: grouped transform result length mismatch");

  if (state.group_ids.empty()) {
    return grouped.take(std::span<const std::int64_t>{});
  }
  if (state.ngroups == 0) {
    return detail::with_validity_mask(
        grouped.take(std::span<const std::int64_t>{}),
        group_bool_mask_(std::vector<bool>(state.group_ids.size(), false)));
  }

  std::vector<std::int64_t> indices = state.broadcast_positions;
  std::vector<bool> valid(state.group_ids.size(), false);
  for (std::size_t row = 0; row < state.group_ids.size(); ++row) {
    if (!state.included[row]) {
      continue;
    }
    valid[row] = detail::valid_at(grouped, indices[row]);
  }

  auto out = grouped.take(indices);
  if (std::any_of(valid.begin(), valid.end(), [](bool value) { return !value; })) {
    auto validity = group_bool_mask_(valid, group_thread_platform_(out));
    PLUSS_CHECK(
        validity.numel() == out.size(),
        fmt::format(
            "group: broadcast validity size mismatch (mask={}, out={})",
            validity.numel(),
            out.size()));
    out = detail::with_validity_mask(out, validity);
  }
  return out;
}

pluss::Series group_keep_mask_(const Thread& keep,
                               const GroupState_& state,
                               std::string_view where) {
  PLUSS_CHECK(keep.defined(), std::string(where) + ": keep mask is undefined");
  PLUSS_CHECK(keep.kind() == ThreadKind::Series, std::string(where) + ": keep mask must be a Bool Series thread");
  PLUSS_CHECK(keep.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
              std::string(where) + ": keep mask must be Bool");
  PLUSS_CHECK(keep.size() == state.ngroups,
              std::string(where) + ": keep mask length must equal number of groups");

  const auto platform = group_thread_platform_(keep);
  if (state.group_ids.empty()) {
    return group_bool_mask_({}, platform);
  }
  if (state.ngroups == 0) {
    return group_bool_mask_(std::vector<bool>(state.group_ids.size(), false), platform);
  }

  auto keep_values = keep.as_series().to_vec<bool>();
  if (detail::has_validity_mask(keep)) {
    const auto keep_valid = detail::validity_mask(keep)->to_vec<bool>();
    std::transform(
        keep_values.begin(),
        keep_values.end(),
        keep_valid.begin(),
        keep_values.begin(),
        std::logical_and<bool>{});
  }

  std::vector<bool> row_keep(state.group_ids.size(), false);
  for (std::size_t row = 0; row < state.group_ids.size(); ++row) {
    if (!state.included[row]) {
      continue;
    }
    row_keep[row] = keep_values[static_cast<std::size_t>(state.broadcast_positions[row])];
  }
  return group_bool_mask_(row_keep, platform);
}

Thread group_agg_thread_(const GroupState_& state,
                         const Thread* value,
                         GroupOp op,
                         std::string_view where) {
  PLUSS_CHECK(op == GroupOp::Count || value != nullptr,
              std::string(where) + ": feature is required for non-count aggregations");
  if (value == nullptr) {
    return group_count_thread_(state);
  }

  switch (op) {
    case GroupOp::Count:
      return group_count_thread_(state, *value);
    case GroupOp::Ndistinct:
      return group_ndistinct_thread_(*value, state);
    case GroupOp::Mode:
      return value->kind() == ThreadKind::Note
          ? group_note_mode_thread_(*value, state)
          : group_numeric_thread_(*value, state, op);
    case GroupOp::Sum:
    case GroupOp::Mean:
    case GroupOp::Median:
    case GroupOp::Min:
    case GroupOp::Max:
    case GroupOp::Std:
    case GroupOp::Var:
      return group_numeric_thread_(*value, state, op);
  }
  PLUSS_FAIL(std::string(where) + ": unsupported group aggregation op");
}

Thread group_count_thread_(const GroupState_& state,
                           std::optional<Thread> value) {
  pluss::codem::Platform platform = pluss::codem::Platform::CPU;
  if (value.has_value() && value->kind() == ThreadKind::Series) {
    platform = value->as_series().platform();
  }

  auto ids = pluss::Series::empty(
      {static_cast<std::int64_t>(state.group_ids.size())},
      pluss::codem::ScalarType::Int64,
      platform);
  if (!state.group_ids.empty()) {
    std::memcpy(ids.raw_data(), state.group_ids.data(), state.group_ids.size() * sizeof(std::int64_t));
  }
  auto active = group_bool_mask_(state.included, platform);
  if (value.has_value() && detail::has_validity_mask(*value)) {
    auto valid = *detail::validity_mask(*value);
    if (valid.platform() != platform) {
      valid = valid.to(platform);
    }
    active = active.logical_and(valid);
  }
  ids = ids.masked_select(active);

  auto counts = pluss::Series::zeros(
      {state.ngroups},
      pluss::codem::ScalarType::Float64,
      platform);
  auto ones = pluss::Series::ones(
      {ids.numel()},
      pluss::codem::ScalarType::Float64,
      platform);
  counts = counts.scatter_add(0, ids, ones);
  return reorder_group_thread_(Thread(counts.to(pluss::codem::ScalarType::Int64)), state);
}

Thread group_ndistinct_thread_(const Thread& value, const GroupState_& state) {
  std::vector<std::int64_t> counts(static_cast<std::size_t>(state.ngroups), 0);
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(std::max<std::int64_t>(16, value.size())));
  for (std::int64_t row = 0; row < value.size(); ++row) {
    if (!state.included[static_cast<std::size_t>(row)] || !detail::valid_at(value, row)) {
      continue;
    }
    const auto group_id = state.group_ids[static_cast<std::size_t>(row)];
    if (group_id < 0) {
      continue;
    }
    auto token = std::to_string(group_id);
    token.push_back('\x1D');
    token.append(thread_scalar_string_at(value, row));
    if (seen.insert(token).second) {
      ++counts[static_cast<std::size_t>(group_id)];
    }
  }
  return reorder_group_thread_(group_i64_thread_(counts), state);
}

Thread group_note_mode_thread_(const Thread& value, const GroupState_& state) {
  struct ModeBucket {
    std::int64_t count{0};
    std::int64_t first_row{-1};
  };

  std::vector<std::unordered_map<std::string, ModeBucket>> buckets(
      static_cast<std::size_t>(state.ngroups));
  for (std::int64_t row = 0; row < value.size(); ++row) {
    if (!state.included[static_cast<std::size_t>(row)] || !detail::valid_at(value, row)) {
      continue;
    }
    const auto group_id = state.group_ids[static_cast<std::size_t>(row)];
    if (group_id < 0) {
      continue;
    }
    auto& bucket = buckets[static_cast<std::size_t>(group_id)][thread_scalar_string_at(value, row)];
    if (bucket.count == 0) {
      bucket.first_row = row;
    }
    ++bucket.count;
  }

  std::vector<std::string> out(static_cast<std::size_t>(state.ngroups));
  std::vector<bool> valid(static_cast<std::size_t>(state.ngroups), false);
  for (std::int64_t group = 0; group < state.ngroups; ++group) {
    std::int64_t best_count = -1;
    std::int64_t best_row = -1;
    std::string best_value;
    for (const auto& [candidate, bucket] : buckets[static_cast<std::size_t>(group)]) {
      if (bucket.count > best_count ||
          (bucket.count == best_count && (best_row < 0 || bucket.first_row < best_row))) {
        best_count = bucket.count;
        best_row = bucket.first_row;
        best_value = candidate;
      }
    }
    if (best_row >= 0) {
      out[static_cast<std::size_t>(group)] = std::move(best_value);
      valid[static_cast<std::size_t>(group)] = true;
    }
  }

  std::vector<std::string_view> views;
  views.reserve(out.size());
  for (const auto& value : out) {
    views.push_back(value);
  }
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(views.size())};
  auto note = Thread(pluss::coden::ops_internal::note_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(shape.data(), shape.size())));
  if (std::any_of(valid.begin(), valid.end(), [](bool value) { return !value; })) {
    note = detail::with_validity_mask(note, group_bool_mask_(valid));
  }
  return reorder_group_thread_(note, state);
}

// Per-group set of every value tied for the highest count, sorted ascending.
// Index i of the returned vector corresponds to *original* group id i (not
// the sort-adjusted "order" position) — callers that care about output order
// must walk state.order themselves, the same way group_key_columns_ does via
// ordered_first_rows. An empty inner vector means the group had zero valid
// (non-null) observations at all.
std::vector<std::vector<std::string>> group_note_mode_ties_(const Thread& value,
                                                             const GroupState_& state) {
  std::vector<std::unordered_map<std::string, std::int64_t>> counts(
      static_cast<std::size_t>(state.ngroups));
  for (std::int64_t row = 0; row < value.size(); ++row) {
    if (!state.included[static_cast<std::size_t>(row)] || !detail::valid_at(value, row)) {
      continue;
    }
    const auto group_id = state.group_ids[static_cast<std::size_t>(row)];
    if (group_id < 0) {
      continue;
    }
    ++counts[static_cast<std::size_t>(group_id)][thread_scalar_string_at(value, row)];
  }

  std::vector<std::vector<std::string>> out(static_cast<std::size_t>(state.ngroups));
  for (std::int64_t group = 0; group < state.ngroups; ++group) {
    const auto& bucket = counts[static_cast<std::size_t>(group)];
    if (bucket.empty()) {
      continue;
    }
    std::int64_t best_count = 0;
    for (const auto& [candidate, count] : bucket) {
      best_count = std::max(best_count, count);
    }
    std::vector<std::string> tied;
    tied.reserve(bucket.size());
    for (const auto& [candidate, count] : bucket) {
      if (count == best_count) {
        tied.push_back(candidate);
      }
    }
    std::sort(tied.begin(), tied.end());
    out[static_cast<std::size_t>(group)] = std::move(tied);
  }
  return out;
}

// Numeric counterpart of group_note_mode_ties_: same contract, but sorted
// ascending by numeric value rather than lexicographically by string.
std::vector<std::vector<double>> group_numeric_mode_ties_(const Thread& value,
                                                           const GroupState_& state) {
  PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
              "DataFrameGrouped::mode_ties: value feature must be Note or a numeric Series");
  const auto values = value.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();

  std::vector<std::unordered_map<double, std::int64_t>> counts(
      static_cast<std::size_t>(state.ngroups));
  for (std::int64_t row = 0; row < value.size(); ++row) {
    if (!state.included[static_cast<std::size_t>(row)] || !detail::valid_at(value, row)) {
      continue;
    }
    const auto group_id = state.group_ids[static_cast<std::size_t>(row)];
    if (group_id < 0) {
      continue;
    }
    ++counts[static_cast<std::size_t>(group_id)][values[static_cast<std::size_t>(row)]];
  }

  std::vector<std::vector<double>> out(static_cast<std::size_t>(state.ngroups));
  for (std::int64_t group = 0; group < state.ngroups; ++group) {
    const auto& bucket = counts[static_cast<std::size_t>(group)];
    if (bucket.empty()) {
      continue;
    }
    std::int64_t best_count = 0;
    for (const auto& [candidate, count] : bucket) {
      best_count = std::max(best_count, count);
    }
    std::vector<double> tied;
    tied.reserve(bucket.size());
    for (const auto& [candidate, count] : bucket) {
      if (count == best_count) {
        tied.push_back(candidate);
      }
    }
    std::sort(tied.begin(), tied.end());
    out[static_cast<std::size_t>(group)] = std::move(tied);
  }
  return out;
}

// Same idea as group_key_columns_, but each output row is one entry in
// `expanded_order_positions` (a "order" position, i.e. an index into
// state.ordered_first_rows) rather than assuming exactly one output row per
// group — the mechanism that lets a single group contribute more than one
// row to mode_ties()'s output.
std::vector<std::pair<std::string, Thread>> group_key_columns_expanded_(
    const GroupState_& state,
    const std::vector<std::int64_t>& expanded_order_positions) {
  std::vector<std::int64_t> expanded_rows;
  expanded_rows.reserve(expanded_order_positions.size());
  for (const auto pos : expanded_order_positions) {
    expanded_rows.push_back(state.ordered_first_rows[static_cast<std::size_t>(pos)]);
  }
  std::vector<std::pair<std::string, Thread>> out;
  out.reserve(state.keys.size());
  for (std::size_t i = 0; i < state.keys.size(); ++i) {
    out.emplace_back(state.names[i], state.keys[i].take(expanded_rows));
  }
  return out;
}

Thread group_numeric_thread_(const Thread& value,
                             const GroupState_& state,
                             GroupOp op) {
  PLUSS_CHECK(value.kind() == ThreadKind::Series && is_numeric_series_type(value.as_series()),
              "group: numeric aggregation requires a numeric Series thread");
  const auto platform = value.as_series().platform();

  auto ids = pluss::Series::empty(
      {static_cast<std::int64_t>(state.group_ids.size())},
      pluss::codem::ScalarType::Int64,
      platform);
  if (!state.group_ids.empty()) {
    std::memcpy(ids.raw_data(), state.group_ids.data(), state.group_ids.size() * sizeof(std::int64_t));
  }

  auto active = group_bool_mask_(state.included, platform);
  if (detail::has_validity_mask(value)) {
    auto valid = *detail::validity_mask(value);
    if (valid.platform() != platform) {
      valid = valid.to(platform);
    }
    active = active.logical_and(valid);
  }

  ids = ids.masked_select(active);
  auto values = value.as_series().to(pluss::codem::ScalarType::Float64).masked_select(active);

  auto counts = pluss::Series::zeros(
      {state.ngroups},
      pluss::codem::ScalarType::Float64,
      platform);
  auto ones = pluss::Series::ones(
      {ids.numel()},
      pluss::codem::ScalarType::Float64,
      platform);
  counts = counts.scatter_add(0, ids, ones);
  const auto any_value = counts.gt(pluss::Series::zeros(
      {state.ngroups},
      pluss::codem::ScalarType::Float64,
      platform));

  pluss::Series out;
  pluss::Series validity = any_value;
  switch (op) {
    case GroupOp::Sum:
      out = values.grouped_sum(ids, state.ngroups);
      break;
    case GroupOp::Mean:
      out = values.grouped_mean(ids, state.ngroups);
      break;
    case GroupOp::Median:
      out = values.grouped_median(ids, state.ngroups);
      break;
    case GroupOp::Min:
      out = values.grouped_min(ids, state.ngroups);
      break;
    case GroupOp::Max:
      out = values.grouped_max(ids, state.ngroups);
      break;
    case GroupOp::Mode:
      out = values.grouped_mode(ids, state.ngroups);
      break;
    case GroupOp::Std:
    case GroupOp::Var: {
      out = op == GroupOp::Std
                ? values.grouped_std(ids, state.ngroups, 1)
                : values.grouped_var(ids, state.ngroups, 1);
      validity = counts.gt(pluss::Series::ones(
          {state.ngroups},
          pluss::codem::ScalarType::Float64,
          platform));
      break;
    }
    case GroupOp::Count:
    case GroupOp::Ndistinct:
      PLUSS_FAIL("group: invalid numeric aggregation dispatch");
  }

  PLUSS_CHECK(
      validity.numel() == out.numel(),
      fmt::format(
          "group: numeric validity size mismatch (op={}, validity={}, out={})",
          group_op_token_(op),
          validity.numel(),
          out.numel()));

  return reorder_group_thread_(Thread(out, validity), state);
}

} // namespace
