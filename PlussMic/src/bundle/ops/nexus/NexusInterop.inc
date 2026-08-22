Nexus Nexus::from_atlas(const pluss::table::Atlas& atlas) {
  if (atlas.axis0_labels().has_value()) {
    return from_atlas(atlas, *atlas.axis0_labels());
  }
  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(static_cast<std::size_t>(atlas.depth()));
  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    out.emplace_back(std::string("table_") + std::to_string(i), atlas.frame(i));
  }
  return Nexus(std::move(out));
}

Nexus Nexus::from_atlas(const pluss::table::Atlas& atlas, const pluss::table::Thread& labels) {
  PLUSS_CHECK(labels.defined(), "Nexus::from_atlas: labels are undefined");
  PLUSS_CHECK(labels.size() == atlas.depth(), "Nexus::from_atlas: labels length must equal atlas depth");

  std::vector<std::pair<std::string, pluss::table::DataFrame>> out;
  out.reserve(static_cast<std::size_t>(atlas.depth()));
  std::unordered_set<std::string> seen;
  seen.reserve(static_cast<std::size_t>(atlas.depth()) * 2 + 1);

  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    auto key = nexus_label_string_at(labels, i);
    if (key.empty()) {
      key = std::string("table_") + std::to_string(i);
    }
    PLUSS_CHECK(seen.insert(key).second, "Nexus::from_atlas: duplicate derived key '" + key + "'");
    out.emplace_back(std::move(key), atlas.frame(i));
  }

  return Nexus(std::move(out));
}

Nexus Nexus::inject(const pluss::table::Atlas& atlas) const {
  auto out = *this;
  out.inject_(atlas);
  return out;
}

Nexus& Nexus::inject_(const pluss::table::Atlas& atlas) {
  if (atlas.axis0_labels().has_value()) {
    const auto staged = Nexus::from_atlas(atlas, *atlas.axis0_labels());
    auto& dst = mut_impl_();
    dst.keys_.reserve(dst.keys_.size() + staged.keys().size());
    dst.frames_.reserve(dst.frames_.size() + staged.frames().size());
    dst.name_to_pos_.reserve((dst.keys_.size() + staged.keys().size()) * 2 + 1);
    for (std::size_t i = 0; i < staged.keys().size(); ++i) {
      append_named_frame(dst, staged.keys()[i], staged.frames()[i], "Nexus::inject(atlas)");
    }
    return *this;
  }

  auto& dst = mut_impl_();
  dst.keys_.reserve(dst.keys_.size() + static_cast<std::size_t>(atlas.depth()));
  dst.frames_.reserve(dst.frames_.size() + static_cast<std::size_t>(atlas.depth()));
  dst.name_to_pos_.reserve((dst.keys_.size() + static_cast<std::size_t>(atlas.depth())) * 2 + 1);
  for (std::int64_t i = 0; i < atlas.depth(); ++i) {
    append_auto_frame(dst, atlas.frame(i), "Nexus::inject(atlas)");
  }
  return *this;
}

pluss::table::Atlas Nexus::to_atlas(std::initializer_list<std::string_view> selected_keys,
                      std::optional<pluss::table::Thread> labels) const {
  return to_atlas(pluss::msharp::RangeView::Of<std::string_view>(selected_keys), std::move(labels));
}

pluss::table::Atlas Nexus::to_atlas(pluss::msharp::RangeView::Of<std::string_view> selected_keys,
                      std::optional<pluss::table::Thread> labels) const {
  const auto owned = normalize_requested_keys(selected_keys, "Nexus::to_atlas");
  const auto& src = impl_ref_();
  std::vector<pluss::table::DataFrame> out;
  std::vector<std::string> out_keys;
  out.reserve(owned.size());
  out_keys.reserve(owned.size());
  for (const auto& key : owned) {
    const auto it = src.name_to_pos_.find(key);
    PLUSS_CHECK(it != src.name_to_pos_.end(), "Nexus::to_atlas: unknown key '" + key + "'");
    out.push_back(src.frames_[static_cast<std::size_t>(it->second)]);
    out_keys.push_back(key);
  }
  if (!labels.has_value()) {
    labels = default_atlas_labels(out_keys);
  }
  return pluss::table::Atlas(std::move(out), std::move(labels));
}
