Nexus::Nexus(std::vector<std::pair<std::string, pluss::table::DataFrame>> tables)
    : impl_(pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(tables))) {}

Nexus::Nexus(std::initializer_list<std::pair<std::string, pluss::table::DataFrame>> tables)
    : Nexus(std::vector<std::pair<std::string, pluss::table::DataFrame>>(tables.begin(), tables.end())) {}

Nexus::Nexus(const Nexus& other) = default;
Nexus::Nexus(Nexus&& other) noexcept = default;
Nexus& Nexus::operator=(const Nexus& other) = default;
Nexus& Nexus::operator=(Nexus&& other) noexcept = default;
Nexus::~Nexus() = default;

Nexus::Nexus(pluss::msharp::intrusive_ptr<detail::NexusImpl> impl)
    : impl_(std::move(impl)) {}

bool Nexus::defined() const noexcept { return static_cast<bool>(impl_); }
bool Nexus::unique() const noexcept { return impl_.unique(); }
bool Nexus::empty() const { return !defined() || ntables() == 0; }

const detail::NexusImpl& Nexus::impl_ref_() const {
  PLUSS_CHECK(defined(), "Nexus: undefined handle");
  return *impl_;
}

detail::NexusImpl& Nexus::mut_impl_() {
  PLUSS_CHECK(defined(), "Nexus: undefined handle");
  if (!impl_.unique()) {
    impl_ = clone_impl(*impl_);
  }
  return *impl_;
}

std::int64_t Nexus::size() const { return ntables(); }
std::int64_t Nexus::ntables() const { return static_cast<std::int64_t>(impl_ref_().keys_.size()); }

const std::vector<std::string>& Nexus::keys() const { return impl_ref_().keys_; }
const std::vector<std::string>& Nexus::names() const { return keys(); }
const std::vector<pluss::table::DataFrame>& Nexus::frames() const { return impl_ref_().frames_; }

bool Nexus::has(std::string_view key) const {
  const auto& src = impl_ref_();
  return src.name_to_pos_.find(std::string(key)) != src.name_to_pos_.end();
}

pluss::table::DataFrame Nexus::frame(std::string_view key) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != src.name_to_pos_.end(), "Nexus::frame: unknown key '" + std::string(key) + "'");
  return src.frames_[static_cast<std::size_t>(it->second)];
}

pluss::table::DataFrame Nexus::frame(std::int64_t index) const {
  const auto& src = impl_ref_();
  const auto idx = normalize_index(index, static_cast<std::int64_t>(src.frames_.size()));
  return src.frames_[static_cast<std::size_t>(idx)];
}

pluss::table::DataFrame Nexus::dloc(std::int64_t index) const {
  return frame(index);
}

std::optional<pluss::table::DataFrame> Nexus::get(std::string_view key) const {
  const auto& src = impl_ref_();
  const auto it = src.name_to_pos_.find(std::string(key));
  if (it == src.name_to_pos_.end()) {
    return std::nullopt;
  }
  return src.frames_[static_cast<std::size_t>(it->second)];
}

Nexus Nexus::head(std::int64_t n) const {
  const auto count = std::clamp<std::int64_t>(n, 0, ntables());
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(static_cast<std::size_t>(count));
  const auto& src = impl_ref_();
  for (std::int64_t i = 0; i < count; ++i) {
    out.emplace_back(src.keys_[static_cast<std::size_t>(i)], src.frames_[static_cast<std::size_t>(i)]);
  }
  auto next = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(out));
  carry_schema_subset(src, *next);
  return Nexus(std::move(next));
}

Nexus Nexus::tail(std::int64_t n) const {
  const auto count = std::clamp<std::int64_t>(n, 0, ntables());
  const auto& src = impl_ref_();
  const auto start = ntables() - count;
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(static_cast<std::size_t>(count));
  for (std::int64_t i = start; i < ntables(); ++i) {
    out.emplace_back(src.keys_[static_cast<std::size_t>(i)], src.frames_[static_cast<std::size_t>(i)]);
  }
  auto next = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(out));
  carry_schema_subset(src, *next);
  return Nexus(std::move(next));
}

Nexus Nexus::select(std::initializer_list<std::string_view> selected_keys) const {
  return select(pluss::msharp::RangeView::Of<std::string_view>(selected_keys));
}

Nexus Nexus::select(pluss::msharp::RangeView::Of<std::string_view> selected_keys) const {
  const auto owned = normalize_requested_keys(selected_keys, "Nexus::select");
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(owned.size());
  for (const auto& key : owned) {
    const auto it = src.name_to_pos_.find(key);
    PLUSS_CHECK(it != src.name_to_pos_.end(), "Nexus::select: unknown key '" + key + "'");
    out.emplace_back(key, src.frames_[static_cast<std::size_t>(it->second)]);
  }
  auto next = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(out));
  carry_schema_subset(src, *next);
  return Nexus(std::move(next));
}

Nexus Nexus::drop(std::initializer_list<std::string_view> dropped_keys) const {
  return drop(pluss::msharp::RangeView::Of<std::string_view>(dropped_keys));
}

Nexus Nexus::drop(pluss::msharp::RangeView::Of<std::string_view> dropped_keys) const {
  const auto owned = normalize_requested_keys(dropped_keys, "Nexus::drop");
  const auto& src = impl_ref_();
  std::unordered_set<std::string> to_drop;
  to_drop.reserve(owned.size());
  for (const auto& key : owned) {
    PLUSS_CHECK(src.name_to_pos_.find(key) != src.name_to_pos_.end(), "Nexus::drop: unknown key '" + key + "'");
    to_drop.insert(key);
  }
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(src.keys_.size() - to_drop.size());
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    if (to_drop.find(src.keys_[i]) == to_drop.end()) {
      out.emplace_back(src.keys_[i], src.frames_[i]);
    }
  }
  auto next = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(out));
  carry_schema_subset(src, *next);
  return Nexus(std::move(next));
}

Nexus Nexus::rename(std::string_view from, std::string_view to) const {
  auto out = *this;
  out.rename_(from, to);
  return out;
}

Nexus& Nexus::rename_(std::string_view from, std::string_view to) {
  const auto from_key = std::string(from);
  const auto to_key = std::string(to);
  PLUSS_CHECK(!from_key.empty(), "Nexus::rename: source key cannot be empty");
  PLUSS_CHECK(!to_key.empty(), "Nexus::rename: target key cannot be empty");
  PLUSS_CHECK(from_key != to_key, "Nexus::rename: source and target keys must differ");

  auto& dst = mut_impl_();
  const auto it = dst.name_to_pos_.find(from_key);
  PLUSS_CHECK(it != dst.name_to_pos_.end(), "Nexus::rename: unknown key '" + from_key + "'");
  PLUSS_CHECK(dst.name_to_pos_.find(to_key) == dst.name_to_pos_.end(),
              "Nexus::rename: target key already exists '" + to_key + "'");

  const auto pos = it->second;
  dst.name_to_pos_.erase(it);
  dst.keys_[static_cast<std::size_t>(pos)] = to_key;
  dst.name_to_pos_.emplace(to_key, pos);
  if (const auto meta_it = dst.table_meta_.find(from_key); meta_it != dst.table_meta_.end()) {
    auto moved = meta_it->second;
    dst.table_meta_.erase(meta_it);
    dst.table_meta_.emplace(to_key, std::move(moved));
  }
  for (auto& rel : dst.relationships_) {
    if (rel.left_table == from_key) {
      rel.left_table = to_key;
    }
    if (rel.right_table == from_key) {
      rel.right_table = to_key;
    }
  }
  for (auto& [_, plan] : dst.saved_views_) {
    rewrite_join_plan_table(plan, from_key, to_key);
  }
  std::int64_t parsed = 0;
  if (parse_auto_key_index(to_key, parsed)) {
    dst.next_auto_id_ = std::max(dst.next_auto_id_, parsed + 1);
  }
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::inject(std::string_view key, const pluss::table::DataFrame& frame_value) const {
  auto out = *this;
  out.inject_(key, frame_value);
  return out;
}

Nexus& Nexus::inject_(std::string_view key, const pluss::table::DataFrame& frame_value) {
  auto& dst = mut_impl_();
  append_named_frame(dst, std::string(key), frame_value, "Nexus::inject");
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::inject(std::initializer_list<std::pair<std::string_view, pluss::table::DataFrame>> tables) const {
  auto out = *this;
  out.inject_(tables);
  return out;
}

Nexus& Nexus::inject_(std::initializer_list<std::pair<std::string_view, pluss::table::DataFrame>> tables) {
  auto& dst = mut_impl_();
  dst.keys_.reserve(dst.keys_.size() + tables.size());
  dst.frames_.reserve(dst.frames_.size() + tables.size());
  dst.name_to_pos_.reserve((dst.keys_.size() + tables.size()) * 2 + 1);
  for (const auto& [key, frame_value] : tables) {
    append_named_frame(dst, std::string(key), frame_value, "Nexus::inject");
  }
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::inject(const pluss::table::DataFrame& frame_value) const {
  auto out = *this;
  out.inject_(frame_value);
  return out;
}

Nexus& Nexus::inject_(const pluss::table::DataFrame& frame_value) {
  auto& dst = mut_impl_();
  append_auto_frame(dst, frame_value, "Nexus::inject");
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::inject(std::initializer_list<pluss::table::DataFrame> tables) const {
  auto out = *this;
  out.inject_(tables);
  return out;
}

Nexus& Nexus::inject_(std::initializer_list<pluss::table::DataFrame> tables) {
  auto& dst = mut_impl_();
  dst.keys_.reserve(dst.keys_.size() + tables.size());
  dst.frames_.reserve(dst.frames_.size() + tables.size());
  dst.name_to_pos_.reserve((dst.keys_.size() + tables.size()) * 2 + 1);
  for (const auto& frame_value : tables) {
    append_auto_frame(dst, frame_value, "Nexus::inject");
  }
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::apply(std::string_view key, const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) const {
  auto out = *this;
  out.apply_(key, fn);
  return out;
}

Nexus& Nexus::apply_(std::string_view key, const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) {
  PLUSS_CHECK(static_cast<bool>(fn), "Nexus::apply: callback is empty");
  auto& dst = mut_impl_();
  const auto it = dst.name_to_pos_.find(std::string(key));
  PLUSS_CHECK(it != dst.name_to_pos_.end(), "Nexus::apply: unknown key '" + std::string(key) + "'");
  const auto updated = fn(dst.frames_[static_cast<std::size_t>(it->second)]);
  PLUSS_CHECK(updated.defined(), "Nexus::apply: callback returned undefined DataFrame");
  dst.frames_[static_cast<std::size_t>(it->second)] = updated;
  invalidate_nexus_runtime(dst);
  return *this;
}

Nexus Nexus::apply_all(const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Nexus::apply_all: callback is empty");
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(src.keys_.size());
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    auto mapped = fn(src.frames_[i]);
    PLUSS_CHECK(mapped.defined(), "Nexus::apply_all: callback returned undefined DataFrame");
    out.emplace_back(src.keys_[i], std::move(mapped));
  }
  auto next = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(out));
  carry_schema_subset(src, *next);
  return Nexus(std::move(next));
}

Nexus Nexus::map_frames(const std::function<pluss::table::DataFrame(const pluss::table::DataFrame&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Nexus::map_frames: callback is empty");
  const auto& src = impl_ref_();
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(src.keys_.size());
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    auto mapped = fn(src.frames_[i]);
    PLUSS_CHECK(mapped.defined(), "Nexus::map_frames: callback returned undefined DataFrame");
    out.emplace_back(src.keys_[i], std::move(mapped));
  }
  auto next = pluss::msharp::make_intrusive<detail::NexusImpl>(std::move(out));
  carry_schema_subset(src, *next);
  return Nexus(std::move(next));
}

Nexus Nexus::flow(const std::function<Nexus(const Nexus&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Nexus::flow: callback is empty");
  auto out = fn(*this);
  PLUSS_CHECK(out.defined(), "Nexus::flow: callback returned undefined Nexus");
  return out;
}

pluss::table::DataFrame Nexus::summary() const {
  const auto& src = impl_ref_();
  std::vector<std::string> names;
  std::vector<std::int64_t> row_counts;
  std::vector<std::int64_t> feature_counts;
  names.reserve(src.keys_.size());
  row_counts.reserve(src.keys_.size());
  feature_counts.reserve(src.keys_.size());

  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    names.push_back(src.keys_[i]);
    row_counts.push_back(src.frames_[i].ncases());
    feature_counts.push_back(src.frames_[i].nfeatures());
  }

  return pluss::table::DataFrame({
      {"table", pluss::table::Slate(pluss::make_note(names))},
      {"ncases", pluss::table::Slate(pluss::make_series(row_counts))},
      {"nfeatures", pluss::table::Slate(pluss::make_series(feature_counts))},
  });
}
