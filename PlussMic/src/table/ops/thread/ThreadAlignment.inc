namespace {

struct ThreadPairAlignment_ {
  Thread left{};
  Thread right{};
  bool aligned{false};
};

Thread broadcast_thread_to_cases_(const Thread& source,
                                  const Thread& target,
                                  std::string_view where);

std::vector<std::string> thread_case_label_strings_(const Thread& thread) {
  PLUSS_CHECK(thread.defined(), "Thread alignment: Thread is undefined");
  const auto labels = detail::has_display_labels(thread)
                          ? detail::display_labels(thread)
                          : default_thread_cases_(thread.size());
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(labels.size()));
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    out.push_back(thread_scalar_string_at(labels, i));
  }
  return out;
}

std::vector<std::string> thread_payload_label_strings_(const Thread& labels,
                                                       std::string_view where) {
  PLUSS_CHECK(labels.defined(), std::string(where) + ": target labels are undefined");
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(labels.size()));
  for (std::int64_t i = 0; i < labels.size(); ++i) {
    out.push_back(thread_scalar_string_at(labels, i));
  }
  return out;
}

Thread thread_case_labels_from_strings_(const std::vector<std::string>& labels) {
  std::vector<std::int64_t> ints;
  ints.reserve(labels.size());
  bool all_ints = true;
  for (const auto& label : labels) {
    if (label.empty()) {
      all_ints = false;
      break;
    }
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoll(label.c_str(), &end, 10);
    if (errno != 0 || end == label.c_str() || *end != '\0' || std::to_string(value) != label) {
      all_ints = false;
      break;
    }
    ints.push_back(static_cast<std::int64_t>(value));
  }
  if (all_ints) {
    return Thread(int64_index_series(ints));
  }
  return Thread(make_utf8_note(labels));
}

pluss::Series bool_series_from_vector_(const std::vector<bool>& values,
                                       pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  auto out = pluss::Series::zeros(
      {static_cast<std::int64_t>(values.size())},
      pluss::codem::ScalarType::Bool,
      pluss::codem::Platform::CPU);
  auto* p = reinterpret_cast<bool*>(out.raw_data());
  for (std::size_t i = 0; i < values.size(); ++i) {
    p[i] = values[i];
  }
  return platform == pluss::codem::Platform::CPU ? out : out.to(platform);
}

pluss::codem::Platform thread_payload_platform_(const Thread& thread) {
  switch (thread.kind()) {
    case ThreadKind::Series:
      return thread.as_series().platform();
    case ThreadKind::Timeline:
      return thread.as_timeline().epoch_nanos().platform();
    case ThreadKind::TimeIndex:
      return thread.as_timeindex().timeline().epoch_nanos().platform();
    case ThreadKind::Note:
      return pluss::codem::Platform::CPU;
  }
  PLUSS_FAIL("Thread alignment: unsupported Thread kind");
}

bool thread_labels_are_default_range_(const Thread& labels, std::int64_t size) {
  if (!labels.defined() || labels.size() != size) {
    return false;
  }
  for (std::int64_t i = 0; i < size; ++i) {
    if (thread_scalar_string_at(labels, i) != std::to_string(i)) {
      return false;
    }
  }
  return true;
}

Thread with_alignment_labels_(Thread out, const Thread& labels, LabelKind kind = LabelKind::Custom) {
  if (kind == LabelKind::Default && thread_labels_are_default_range_(labels, out.size())) {
    return detail::with_label_kind(out, LabelKind::Default);
  }
  const auto resolved_kind = kind == LabelKind::Default ? LabelKind::Custom : kind;
  return detail::with_label_kind(detail::with_display_labels(out, labels), resolved_kind);
}

void set_series_value_from_double_(pluss::Series& dst, std::int64_t idx, double value) {
  switch (dst.scalar_type()) {
    case pluss::codem::ScalarType::Bool:
      reinterpret_cast<bool*>(dst.raw_data())[idx] = value != 0.0;
      return;
    case pluss::codem::ScalarType::Int8:
      reinterpret_cast<std::int8_t*>(dst.raw_data())[idx] = static_cast<std::int8_t>(value);
      return;
    case pluss::codem::ScalarType::Int16:
      reinterpret_cast<std::int16_t*>(dst.raw_data())[idx] = static_cast<std::int16_t>(value);
      return;
    case pluss::codem::ScalarType::Int32:
      reinterpret_cast<std::int32_t*>(dst.raw_data())[idx] = static_cast<std::int32_t>(value);
      return;
    case pluss::codem::ScalarType::Int64:
      reinterpret_cast<std::int64_t*>(dst.raw_data())[idx] = static_cast<std::int64_t>(value);
      return;
    case pluss::codem::ScalarType::UInt8:
      reinterpret_cast<std::uint8_t*>(dst.raw_data())[idx] = static_cast<std::uint8_t>(value);
      return;
    case pluss::codem::ScalarType::UInt16:
      reinterpret_cast<std::uint16_t*>(dst.raw_data())[idx] = static_cast<std::uint16_t>(value);
      return;
    case pluss::codem::ScalarType::UInt32:
      reinterpret_cast<std::uint32_t*>(dst.raw_data())[idx] = static_cast<std::uint32_t>(value);
      return;
    case pluss::codem::ScalarType::UInt64:
      reinterpret_cast<std::uint64_t*>(dst.raw_data())[idx] = static_cast<std::uint64_t>(value);
      return;
    case pluss::codem::ScalarType::Float16:
      reinterpret_cast<pluss::codem::Float16*>(dst.raw_data())[idx] =
          static_cast<pluss::codem::Float16>(value);
      return;
    case pluss::codem::ScalarType::BFloat16:
      reinterpret_cast<pluss::codem::BFloat16*>(dst.raw_data())[idx] =
          static_cast<pluss::codem::BFloat16>(value);
      return;
    case pluss::codem::ScalarType::Float32:
      reinterpret_cast<float*>(dst.raw_data())[idx] = static_cast<float>(value);
      return;
    case pluss::codem::ScalarType::Float64:
      reinterpret_cast<double*>(dst.raw_data())[idx] = value;
      return;
    default:
      PLUSS_FAIL("Thread alignment: numeric fill requires a real numeric Series dtype");
  }
}

Thread fill_missing_for_alignment_(const Thread& source,
                                   std::optional<double> fill,
                                   std::string_view where) {
  if (!fill.has_value()) {
    return source;
  }
  PLUSS_CHECK(source.kind() == ThreadKind::Series && is_numeric_series_type(source.as_series()),
              std::string(where) + ": numeric fill requires a numeric Series-backed Thread");
  if (!detail::has_validity_mask(source)) {
    return source;
  }

  auto valid = detail::validity_mask(source)->to_vec<bool>();
  if (std::all_of(valid.begin(), valid.end(), [](bool v) { return v; })) {
    return detail::clear_validity_mask(source);
  }

  auto values = source.as_series().clone();
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(valid.size()); ++i) {
    if (!valid[static_cast<std::size_t>(i)]) {
      set_series_value_from_double_(values, i, *fill);
      valid[static_cast<std::size_t>(i)] = true;
    }
  }
  auto out = Thread(values);
  if (detail::has_display_labels(source)) {
    out = detail::with_display_labels(out, detail::display_labels(source));
  }
  out = detail::with_label_kind(out, detail::label_kind(source));
  return out;
}

Thread null_thread_like_for_alignment_(const Thread& like,
                                       std::int64_t size,
                                       const Thread& labels,
                                       std::string_view where) {
  PLUSS_CHECK(size >= 0, std::string(where) + ": aligned size must be non-negative");
  Thread out;
  switch (like.kind()) {
    case ThreadKind::Series:
      out = Thread(pluss::Series::zeros(
          {size}, like.as_series().scalar_type(), like.as_series().platform()));
      break;
    case ThreadKind::Note:
      out = Thread(make_utf8_note(std::vector<std::string>(
          static_cast<std::size_t>(size), std::string{})));
      break;
    case ThreadKind::Timeline:
      out = Thread(pluss::make_timeline(std::vector<std::string>(
          static_cast<std::size_t>(size), "1970-01-01T00:00:00Z")));
      break;
    case ThreadKind::TimeIndex:
      out = Thread(pluss::make_timeindex(std::vector<std::string>(
          static_cast<std::size_t>(size), "1970-01-01T00:00:00Z")));
      break;
  }
  return with_alignment_labels_(
      detail::with_validity_mask(
          out,
          pluss::Series::zeros({size}, pluss::codem::ScalarType::Bool,
                               thread_payload_platform_(out))),
      labels,
      detail::label_kind(labels));
}

Thread remap_thread_for_alignment_(const Thread& source,
                                   const std::vector<std::int64_t>& rows,
                                   const std::vector<bool>& present,
                                   const Thread& labels,
                                   std::string_view where) {
  PLUSS_CHECK(rows.size() == present.size(),
              std::string(where) + ": internal alignment plan size mismatch");
  const auto out_size = static_cast<std::int64_t>(rows.size());
  if (out_size == 0 || source.size() == 0) {
    return null_thread_like_for_alignment_(source, out_size, labels, where);
  }

  std::vector<std::int64_t> take_rows;
  take_rows.reserve(rows.size());
  for (std::size_t i = 0; i < rows.size(); ++i) {
    take_rows.push_back(present[i] ? rows[i] : 0);
  }

  auto out = source.take(std::span<const std::int64_t>(take_rows.data(), take_rows.size()));
  std::optional<pluss::Series> validity = std::nullopt;
  const auto present_mask = bool_series_from_vector_(present, thread_payload_platform_(out));
  const bool all_present = std::all_of(present.begin(), present.end(), [](bool v) { return v; });
  if (!all_present) {
    validity = present_mask;
  }
  if (detail::has_validity_mask(out)) {
    auto source_validity = *detail::validity_mask(out);
    if (source_validity.platform() != present_mask.platform()) {
      source_validity = source_validity.to(present_mask.platform());
    }
    validity = validity.has_value() ? validity->logical_and(source_validity) : source_validity;
  }
  if (validity.has_value()) {
    out = detail::with_validity_mask(out, *validity);
  }
  return with_alignment_labels_(out, labels, detail::label_kind(labels));
}

ThreadPairAlignment_ align_thread_pair_for_cases_(const Thread& left,
                                                  const Thread& right,
                                                  std::string_view where) {
  const bool left_labeled = detail::has_display_labels(left);
  const bool right_labeled = detail::has_display_labels(right);

  if (!left_labeled && !right_labeled) {
    if (left.size() == right.size()) {
      return {left, right, false};
    }
  }

  if (left_labeled != right_labeled) {
    const auto labels = left_labeled ? detail::display_labels(left) : detail::display_labels(right);
    if (!left_labeled && left.size() == right.size()) {
      return {detail::with_display_labels(left, labels), right, true};
    }
    if (!right_labeled && right.size() == left.size()) {
      return {left, detail::with_display_labels(right, labels), true};
    }
  }

  const auto left_labels = thread_case_label_strings_(left);
  const auto right_labels = thread_case_label_strings_(right);
  if (left_labels == right_labels) {
    return {left, right, false};
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> right_positions;
  right_positions.reserve(right_labels.size());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(right_labels.size()); ++i) {
    right_positions[right_labels[static_cast<std::size_t>(i)]].push_back(i);
  }

  std::vector<std::string> output_labels;
  std::vector<std::int64_t> left_rows;
  std::vector<std::int64_t> right_rows;
  std::vector<bool> left_present;
  std::vector<bool> right_present;
  output_labels.reserve(left_labels.size() + right_labels.size());
  left_rows.reserve(left_labels.size() + right_labels.size());
  right_rows.reserve(left_labels.size() + right_labels.size());
  left_present.reserve(left_labels.size() + right_labels.size());
  right_present.reserve(left_labels.size() + right_labels.size());

  std::unordered_map<std::string, std::size_t> left_seen;
  left_seen.reserve(left_labels.size());
  std::unordered_map<std::string, std::size_t> matched_right_count;
  matched_right_count.reserve(right_positions.size());
  for (std::int64_t row = 0; row < static_cast<std::int64_t>(left_labels.size()); ++row) {
    const auto& key = left_labels[static_cast<std::size_t>(row)];
    const auto occurrence = left_seen[key]++;
    const auto found = right_positions.find(key);
    output_labels.push_back(key);
    left_rows.push_back(row);
    left_present.push_back(true);
    if (found != right_positions.end() && occurrence < found->second.size()) {
      const auto right_row = found->second[occurrence];
      right_rows.push_back(right_row);
      right_present.push_back(true);
      matched_right_count[key] = std::max<std::size_t>(matched_right_count[key], occurrence + 1);
    } else {
      right_rows.push_back(0);
      right_present.push_back(false);
    }
  }

  std::unordered_map<std::string, std::size_t> right_seen;
  right_seen.reserve(right_labels.size());
  for (std::int64_t row = 0; row < static_cast<std::int64_t>(right_labels.size()); ++row) {
    const auto& key = right_labels[static_cast<std::size_t>(row)];
    const auto occurrence = right_seen[key]++;
    if (occurrence < matched_right_count[key]) {
      continue;
    }
    output_labels.push_back(key);
    left_rows.push_back(0);
    left_present.push_back(false);
    right_rows.push_back(row);
    right_present.push_back(true);
  }

  const auto labels = thread_case_labels_from_strings_(output_labels);
  return {
      remap_thread_for_alignment_(left, left_rows, left_present, labels, where),
      remap_thread_for_alignment_(right, right_rows, right_present, labels, where),
      true};
}

Thread align_thread_to_cases_(const Thread& source,
                              const Thread& target,
                              std::string_view where,
                              bool allow_scalar = false) {
  PLUSS_CHECK(source.defined() && target.defined(),
              std::string(where) + ": alignment source/target must be defined");
  if (allow_scalar && source.size() == 1) {
    return source;
  }

  const bool source_labeled = detail::has_display_labels(source);
  const bool target_labeled = detail::has_display_labels(target);
  if (!source_labeled && !target_labeled) {
    if (source.size() == target.size()) {
      return source;
    }
  }

  const auto target_labels = thread_case_label_strings_(target);
  const auto labels = thread_case_labels_from_strings_(target_labels);

  const auto source_labels = thread_case_label_strings_(source);
  if (source_labels == target_labels) {
    return source;
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> source_positions;
  source_positions.reserve(source_labels.size());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(source_labels.size()); ++i) {
    source_positions[source_labels[static_cast<std::size_t>(i)]].push_back(i);
  }

  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(target_labels.size());
  std::vector<std::int64_t> rows;
  std::vector<bool> present;
  rows.reserve(target_labels.size());
  present.reserve(target_labels.size());
  for (const auto& key : target_labels) {
    const auto occurrence = seen[key]++;
    const auto found = source_positions.find(key);
    if (found != source_positions.end() && occurrence < found->second.size()) {
      rows.push_back(found->second[occurrence]);
      present.push_back(true);
    } else {
      rows.push_back(0);
      present.push_back(false);
    }
  }
  return remap_thread_for_alignment_(source, rows, present, labels, where);
}

LabelKind label_kind_for_align_by_(AlignBy align) {
  switch (align) {
    case AlignBy::Pos:
      return LabelKind::Default;
    case AlignBy::Caselbl:
      return LabelKind::Case;
    case AlignBy::Feature:
      return LabelKind::Feature;
    case AlignBy::Both:
      throw ::pluss::Error("Thread alignment: AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
  }
  PLUSS_FAIL("Thread alignment: unsupported AlignBy value");
}

Thread align_thread_to_label_values_(const Thread& source,
                                     const Thread& target_labels,
                                     const AlignOptions& options,
                                     std::string_view where,
                                     bool allow_scalar = false) {
  PLUSS_CHECK(source.defined(), std::string(where) + ": source Thread is undefined");
  PLUSS_CHECK(target_labels.defined(), std::string(where) + ": target labels are undefined");

  if (options.align == AlignBy::Both) {
    throw ::pluss::Error(
        std::string(where) + ": AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
  }

  if (allow_scalar && source.size() == 1) {
    return source;
  }

  if (options.align == AlignBy::Pos) {
    if (source.size() != target_labels.size()) {
      throw ::pluss::Error(std::string(where) + ": positional alignment requires identical lengths");
    }
    return source;
  }

  const auto target_label_strings = thread_payload_label_strings_(target_labels, where);
  const auto labels = detail::with_label_kind(
      thread_case_labels_from_strings_(target_label_strings),
      label_kind_for_align_by_(options.align));

  const auto source_label_strings = thread_payload_label_strings_(source.caselbl(), where);
  if (source_label_strings == target_label_strings) {
    return fill_missing_for_alignment_(
        with_alignment_labels_(source, labels, label_kind_for_align_by_(options.align)),
        options.fill,
        where);
  }

  std::unordered_map<std::string, std::vector<std::int64_t>> source_positions;
  source_positions.reserve(source_label_strings.size());
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(source_label_strings.size()); ++i) {
    source_positions[source_label_strings[static_cast<std::size_t>(i)]].push_back(i);
  }

  std::unordered_map<std::string, std::size_t> seen;
  seen.reserve(target_label_strings.size());
  std::vector<std::int64_t> rows;
  std::vector<bool> present;
  rows.reserve(target_label_strings.size());
  present.reserve(target_label_strings.size());
  for (const auto& key : target_label_strings) {
    const auto occurrence = seen[key]++;
    const auto found = source_positions.find(key);
    if (found != source_positions.end() && occurrence < found->second.size()) {
      rows.push_back(found->second[occurrence]);
      present.push_back(true);
    } else {
      rows.push_back(0);
      present.push_back(false);
    }
  }

  return fill_missing_for_alignment_(
      remap_thread_for_alignment_(source, rows, present, labels, where),
      options.fill,
      where);
}

ThreadPairAlignment_ align_thread_pair_(const Thread& left,
                                        const Thread& right,
                                        const AlignOptions& options,
                                        std::string_view where) {
  if (options.align == AlignBy::Both) {
    throw ::pluss::Error(
        std::string(where) + ": AlignBy::Both is only valid for DataFrame-to-DataFrame alignment");
  }
  if (options.align == AlignBy::Pos) {
    if (left.size() != right.size()) {
      throw ::pluss::Error(std::string(where) + ": positional alignment requires identical lengths");
    }
    return {left, right, false};
  }

  const auto aligned = align_thread_pair_for_cases_(left, right, where);
  return {
      fill_missing_for_alignment_(aligned.left, options.fill, where),
      fill_missing_for_alignment_(aligned.right, options.fill, where),
      aligned.aligned || options.fill.has_value()};
}

Thread broadcast_thread_to_cases_(const Thread& source,
                                  const Thread& target,
                                  std::string_view where) {
  PLUSS_CHECK(source.defined() && target.defined(),
              std::string(where) + ": broadcast source/target must be defined");
  PLUSS_CHECK(source.size() == 1,
              std::string(where) + ": broadcast source must have one value");
  std::vector<std::int64_t> rows(static_cast<std::size_t>(target.size()), 0);
  auto out = source.take(std::span<const std::int64_t>(rows.data(), rows.size()));
  if (detail::has_display_labels(target)) {
    out = detail::with_display_labels(out, detail::display_labels(target));
  }
  return out;
}

Thread carry_case_labels_for_alignment_(const Thread& value, const Thread& reference) {
  return detail::has_display_labels(reference)
             ? detail::with_display_labels(value, detail::display_labels(reference))
             : value;
}

Thread aligned_numeric_binary_(const Thread& left,
                               const Thread& right,
                               ThreadBinaryMathOp op,
                               std::string_view where) {
  const auto aligned = align_thread_pair_(left, right, AlignOptions{}, where);
  const auto& lhs = aligned.left;
  const auto& rhs = aligned.right;
  PLUSS_CHECK(lhs.kind() == ThreadKind::Series && rhs.kind() == ThreadKind::Series,
              std::string(where) + ": both operands must be Series-backed Threads");
  PLUSS_CHECK(is_numeric_series_type(lhs.as_series()) && is_numeric_series_type(rhs.as_series()),
              std::string(where) + ": both operands must be numeric Series payloads");
  return carry_case_labels_for_alignment_(
      Thread(apply_math_series_series(lhs.as_series(), rhs.as_series(), op),
             combine_validity_masks(lhs, rhs)),
      lhs);
}

Thread aligned_compare_(const Thread& left,
                        const Thread& right,
                        ThreadBinaryCompareOp op,
                        std::string_view where) {
  const auto aligned = align_thread_pair_(left, right, AlignOptions{}, where);
  const auto& lhs = aligned.left;
  const auto& rhs = aligned.right;
  const auto lk = lhs.kind();
  const auto rk = rhs.kind();

  if (lk == ThreadKind::Series && rk == ThreadKind::Series) {
    return carry_case_labels_for_alignment_(
        Thread(apply_compare_series_series(lhs.as_series(), rhs.as_series(), op),
               combine_validity_masks(lhs, rhs)),
        lhs);
  }

  if (lk == ThreadKind::Note && rk == ThreadKind::Note) {
    const auto& ln = lhs.as_note();
    const auto& rn = rhs.as_note();
    std::vector<std::uint8_t> raw;
    switch (op) {
      case ThreadBinaryCompareOp::Eq:
        raw = (ln == rn);
        break;
      case ThreadBinaryCompareOp::Ne:
        raw = (ln != rn);
        break;
      case ThreadBinaryCompareOp::Lt:
        raw = (ln < rn);
        break;
      case ThreadBinaryCompareOp::Le:
        raw = (ln <= rn);
        break;
      case ThreadBinaryCompareOp::Gt:
        raw = (ln > rn);
        break;
      case ThreadBinaryCompareOp::Ge:
        raw = (ln >= rn);
        break;
    }
    return carry_case_labels_for_alignment_(
        Thread(bool_series_from_u8(raw), combine_validity_masks(lhs, rhs)),
        lhs);
  }

  if ((lk == ThreadKind::Timeline || lk == ThreadKind::TimeIndex) &&
      (rk == ThreadKind::Timeline || rk == ThreadKind::TimeIndex)) {
    const auto& lt = lk == ThreadKind::Timeline ? lhs.as_timeline()
                                                : lhs.as_timeindex().timeline();
    const auto& rt = rk == ThreadKind::Timeline ? rhs.as_timeline()
                                                : rhs.as_timeindex().timeline();
    return carry_case_labels_for_alignment_(
        Thread(apply_compare_timeline_timeline(lt, rt, op),
               combine_validity_masks(lhs, rhs)),
        lhs);
  }

  PLUSS_FAIL(std::string(where) + ": unsupported payload combination for aligned compare");
}

} // namespace
