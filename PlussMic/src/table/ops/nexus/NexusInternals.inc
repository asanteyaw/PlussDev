namespace {

std::int64_t normalize_index(std::int64_t idx, std::int64_t size) {
  std::int64_t out = idx;
  if (out < 0) {
    out += size;
  }
  PLUSS_CHECK(out >= 0 && out < size, "Nexus: table index out of bounds");
  return out;
}

bool parse_auto_key_index(std::string_view key, std::int64_t& out) {
  constexpr std::string_view prefix = "table_";
  if (!key.starts_with(prefix) || key.size() == prefix.size()) {
    return false;
  }
  const auto digits = key.substr(prefix.size());
  auto value = std::int64_t{0};
  const auto* begin = digits.data();
  const auto* end = digits.data() + digits.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end || value < 0) {
    return false;
  }
  out = value;
  return true;
}

std::int64_t infer_next_auto_id(const std::vector<std::string>& keys) {
  std::int64_t next = 0;
  for (const auto& key : keys) {
    std::int64_t parsed = 0;
    if (parse_auto_key_index(key, parsed)) {
      next = std::max(next, parsed + 1);
    }
  }
  return next;
}

std::vector<std::string> normalize_requested_keys(std::initializer_list<std::string_view> keys,
                                                  std::string_view where) {
  std::vector<std::string> out;
  out.reserve(keys.size());
  std::unordered_set<std::string> seen;
  seen.reserve(keys.size());
  for (const auto key_sv : keys) {
    const auto key = std::string(key_sv);
    PLUSS_CHECK(!key.empty(), std::string(where) + ": key cannot be empty");
    PLUSS_CHECK(seen.insert(key).second, std::string(where) + ": duplicate key '" + key + "'");
    out.push_back(key);
  }
  return out;
}

pluss::msharp::intrusive_ptr<detail::NexusImpl> clone_impl(const detail::NexusImpl& src) {
  std::vector<std::pair<std::string, DataFrame>> copied;
  copied.reserve(src.keys_.size());
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    copied.emplace_back(src.keys_[i], src.frames_[i]);
  }
  auto out = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(copied));
  out->table_meta_ = src.table_meta_;
  out->relationships_ = src.relationships_;
  out->rel_name_to_pos_ = src.rel_name_to_pos_;
  out->saved_views_ = src.saved_views_;
  out->next_auto_id_ = src.next_auto_id_;
  return out;
}

std::string next_auto_key(detail::NexusImpl& dst) {
  while (true) {
    const auto candidate = std::string("table_") + std::to_string(dst.next_auto_id_++);
    if (dst.name_to_pos_.find(candidate) == dst.name_to_pos_.end()) {
      return candidate;
    }
  }
}

void append_named_frame(detail::NexusImpl& dst, std::string key, const DataFrame& frame, std::string_view where) {
  PLUSS_CHECK(!key.empty(), std::string(where) + ": key cannot be empty");
  PLUSS_CHECK(frame.defined(), std::string(where) + ": frame is undefined");
  PLUSS_CHECK(dst.name_to_pos_.find(key) == dst.name_to_pos_.end(),
              std::string(where) + ": duplicate key '" + key + "'");
  std::int64_t parsed = 0;
  if (parse_auto_key_index(key, parsed)) {
    dst.next_auto_id_ = std::max(dst.next_auto_id_, parsed + 1);
  }
  const auto pos = static_cast<std::int64_t>(dst.keys_.size());
  dst.name_to_pos_.emplace(key, pos);
  dst.keys_.push_back(std::move(key));
  dst.frames_.push_back(frame);
}

void append_auto_frame(detail::NexusImpl& dst, const DataFrame& frame, std::string_view where) {
  append_named_frame(dst, next_auto_key(dst), frame, where);
}

std::string nexus_label_string_at(const Thread& labels, std::int64_t i) {
  switch (labels.kind()) {
    case ThreadKind::Series: {
      const auto series = labels.as_series();
      switch (series.scalar_type()) {
        case pluss::codem::ScalarType::Int64:
          return std::to_string(series.to_vec<std::int64_t>(i));
        case pluss::codem::ScalarType::Float64:
          return std::to_string(series.to_vec<double>(i));
        default:
          return std::string("table_") + std::to_string(i);
      }
    }
    case ThreadKind::Note:
      return std::string(labels.as_note().at(i));
    case ThreadKind::Timeline:
      return std::to_string(labels.as_timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(i)]);
    case ThreadKind::TimeIndex:
      return std::to_string(
          labels.as_timeindex().timeline().to_epoch_nanos_vector()[static_cast<std::size_t>(i)]);
  }
  return std::string("table_") + std::to_string(i);
}

Thread default_atlas_labels(const std::vector<std::string>& keys) {
  return Thread(pluss::make_note(keys));
}

std::unordered_set<std::string> retained_key_set(const detail::NexusImpl& dst) {
  return std::unordered_set<std::string>(dst.keys_.begin(), dst.keys_.end());
}

std::vector<std::string> join_plan_tables(const JoinPlan& plan) {
  std::vector<std::string> out;
  if (plan.root_table.empty()) {
    return out;
  }
  out.push_back(plan.root_table);
  out.reserve(plan.steps.size() + 1);
  for (const auto& step : plan.steps) {
    if (out.back() != step.left_table) {
      out.push_back(step.left_table);
    }
    if (out.back() != step.right_table) {
      out.push_back(step.right_table);
    }
  }
  return out;
}

bool join_plan_supported_by(const JoinPlan& plan,
                            const std::unordered_set<std::string>& retained) {
  const auto tables = join_plan_tables(plan);
  return std::all_of(tables.begin(), tables.end(), [&](const std::string& table) {
    return retained.find(table) != retained.end();
  });
}

void rewrite_join_plan_table(JoinPlan& plan,
                             std::string_view from,
                             std::string_view to) {
  const auto from_key = std::string(from);
  const auto to_key = std::string(to);
  if (plan.root_table == from_key) {
    plan.root_table = to_key;
  }
  for (auto& step : plan.steps) {
    if (step.left_table == from_key) {
      step.left_table = to_key;
    }
    if (step.right_table == from_key) {
      step.right_table = to_key;
    }
  }
}

void rebuild_relationship_index(detail::NexusImpl& dst) {
  dst.rel_name_to_pos_.clear();
  dst.rel_name_to_pos_.reserve(dst.relationships_.size() * 2 + 1);
  for (std::size_t i = 0; i < dst.relationships_.size(); ++i) {
    dst.rel_name_to_pos_.emplace(dst.relationships_[i].name, static_cast<std::int64_t>(i));
  }
}

void carry_schema_subset(const detail::NexusImpl& src, detail::NexusImpl& dst) {
  const auto retained = retained_key_set(dst);
  dst.table_meta_.clear();
  for (const auto& key : dst.keys_) {
    const auto meta_it = src.table_meta_.find(key);
    if (meta_it != src.table_meta_.end()) {
      dst.table_meta_.emplace(key, meta_it->second);
    }
  }

  dst.relationships_.clear();
  dst.relationships_.reserve(src.relationships_.size());
  for (const auto& rel : src.relationships_) {
    if (retained.find(rel.left_table) != retained.end() &&
        retained.find(rel.right_table) != retained.end()) {
      dst.relationships_.push_back(rel);
    }
  }
  rebuild_relationship_index(dst);

  dst.saved_views_.clear();
  for (const auto& [name, plan] : src.saved_views_) {
    if (join_plan_supported_by(plan, retained)) {
      dst.saved_views_.emplace(name, plan);
    }
  }
  dst.next_auto_id_ = src.next_auto_id_;
}

void require_known_table(const detail::NexusImpl& src, std::string_view key, std::string_view where) {
  PLUSS_CHECK(src.name_to_pos_.find(std::string(key)) != src.name_to_pos_.end(),
              std::string(where) + ": unknown table '" + std::string(key) + "'");
}

const DataFrame& frame_ref(const detail::NexusImpl& src,
                           std::string_view key,
                           std::string_view where) {
  const auto it = src.name_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != src.name_to_pos_.end(),
              std::string(where) + ": unknown table '" + std::string(key) + "'");
  return src.frames_[static_cast<std::size_t>(it->second)];
}

void validate_relationship_payload(const detail::NexusImpl& src, const Relationship& rel, std::string_view where) {
  PLUSS_CHECK(!rel.name.empty(), std::string(where) + ": relationship name cannot be empty");
  PLUSS_CHECK(!rel.left_table.empty(), std::string(where) + ": left_table cannot be empty");
  PLUSS_CHECK(!rel.right_table.empty(), std::string(where) + ": right_table cannot be empty");
  PLUSS_CHECK(!rel.keys.empty(), std::string(where) + ": relationship keys cannot be empty");
  require_known_table(src, rel.left_table, where);
  require_known_table(src, rel.right_table, where);
  PLUSS_CHECK(rel.confidence >= 0.0 && rel.confidence <= 1.0,
              std::string(where) + ": confidence must be in [0, 1]");
  for (const auto& [left_key, right_key] : rel.keys) {
    PLUSS_CHECK(!left_key.empty() && !right_key.empty(),
                std::string(where) + ": relationship key pairs cannot be empty");
  }
}

} // namespace

detail::NexusImpl::NexusImpl(std::vector<std::pair<std::string, DataFrame>> tables) {
  keys_.reserve(tables.size());
  frames_.reserve(tables.size());
  name_to_pos_.reserve(tables.size() * 2 + 1);

  for (auto& [key, frame] : tables) {
    PLUSS_CHECK(!key.empty(), "Nexus: key cannot be empty");
    PLUSS_CHECK(frame.defined(), "Nexus: frame is undefined");
    PLUSS_CHECK(name_to_pos_.find(key) == name_to_pos_.end(), "Nexus: duplicate key '" + key + "'");
    const auto pos = static_cast<std::int64_t>(keys_.size());
    name_to_pos_.emplace(key, pos);
    keys_.push_back(std::move(key));
    frames_.push_back(std::move(frame));
  }

  next_auto_id_ = infer_next_auto_id(keys_);
}
