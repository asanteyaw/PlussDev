namespace {

std::vector<std::string> default_table_keys(const detail::NexusImpl& src,
                                            std::string_view table,
                                            std::string_view where) {
  require_known_table(src, table, where);
  const auto meta_it = src.table_meta_.find(std::string(table));
  PLUSS_CHECK(meta_it != src.table_meta_.end(),
              std::string(where) + ": no metadata declared for table '" + std::string(table) + "'");
  PLUSS_CHECK(!meta_it->second.primary_key.empty(),
              std::string(where) + ": no primary key declared for table '" + std::string(table) + "'");
  return meta_it->second.primary_key;
}

void require_frame_features(const DataFrame& frame,
                            const std::vector<std::string>& keys,
                            std::string_view where) {
  for (const auto& key : keys) {
    PLUSS_CHECK(frame.get(key).has_value(),
                std::string(where) + ": unknown feature '" + key + "'");
  }
}

std::string key_signature(const std::vector<std::string>& keys) {
  std::string out;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i > 0) {
      out += " + ";
    }
    out += keys[i];
  }
  return out;
}

std::vector<std::string_view> key_views_of(const std::vector<std::string>& keys) {
  std::vector<std::string_view> out;
  out.reserve(keys.size());
  for (const auto& key : keys) {
    out.push_back(key);
  }
  return out;
}

detail::NexusKeyAuditProfile build_key_audit_profile(const DataFrame& frame,
                                                     std::vector<std::string> keys,
                                                     std::string_view where) {
  PLUSS_CHECK(!keys.empty(), std::string(where) + ": key set cannot be empty");
  require_frame_features(frame, keys, where);

  detail::NexusKeyAuditProfile profile;
  profile.keys = std::move(keys);
  profile.total_rows = frame.ncases();
  const auto key_views = key_views_of(profile.keys);
  const auto complete_mask = frame.features(pluss::msharp::RangeView::Of<std::string_view>(key_views))
                                 .notnull()
                                 .all(Axis::Feature);
  profile.complete_rows = complete_mask.count(true).to_int();
  profile.missing_rows = profile.total_rows - profile.complete_rows;
  profile.complete_frame = frame.filter(complete_mask);
  profile.duplicate_frame = profile.complete_frame.filter(
      profile.complete_frame.features(pluss::msharp::RangeView::Of<std::string_view>(key_views))
          .duplicated(false, false)
          .as_series());
  return profile;
}

const detail::NexusKeyAuditProfile& cached_key_audit_profile(const detail::NexusImpl& src,
                                                             const DataFrame& frame,
                                                             std::string_view table,
                                                             std::vector<std::string> keys,
                                                             std::string_view where) {
  auto cache_key = nexus_key_profile_cache_key(table, keys);
  if (const auto it = src.key_profile_cache_.find(cache_key); it != src.key_profile_cache_.end()) {
    return it->second;
  }
  auto profile = build_key_audit_profile(frame, std::move(keys), where);
  auto [it, _] = src.key_profile_cache_.emplace(std::move(cache_key), std::move(profile));
  return it->second;
}

bool key_profile_valid(const detail::NexusKeyAuditProfile& profile) {
  return profile.missing_rows == 0 && profile.duplicate_frame.ncases() == 0;
}

std::vector<std::string> relationship_left_keys(const Relationship& rel) {
  std::vector<std::string> out;
  out.reserve(rel.keys.size());
  for (const auto& [left_key, _] : rel.keys) {
    out.push_back(left_key);
  }
  return out;
}

std::vector<std::string> relationship_right_keys(const Relationship& rel) {
  std::vector<std::string> out;
  out.reserve(rel.keys.size());
  for (const auto& [_, right_key] : rel.keys) {
    out.push_back(right_key);
  }
  return out;
}

std::string cardinality_string(RelationshipCardinality cardinality) {
  switch (cardinality) {
    case RelationshipCardinality::OneToOne:
      return "one_to_one";
    case RelationshipCardinality::OneToMany:
      return "one_to_many";
    case RelationshipCardinality::ManyToOne:
      return "many_to_one";
    case RelationshipCardinality::ManyToMany:
      return "many_to_many";
  }
  return "many_to_one";
}

std::string validation_status_string(RelationshipValidationStatus status) {
  switch (status) {
    case RelationshipValidationStatus::Declared:
      return "declared";
    case RelationshipValidationStatus::Validated:
      return "validated";
    case RelationshipValidationStatus::Warning:
      return "warning";
    case RelationshipValidationStatus::Invalid:
      return "invalid";
  }
  return "declared";
}

DataFrame orphan_rows(const DataFrame& source,
                      const std::vector<std::string>& source_keys,
                      const DataFrame& other,
                      const std::vector<std::string>& other_keys,
                      std::string_view right_suffix) {
  const auto source_views = key_views_of(source_keys);
  const auto other_views = key_views_of(other_keys);
  auto orphans = source.unify(pluss::msharp::RangeView::Of<std::string_view>(source_views),
                              other,
                              pluss::msharp::RangeView::Of<std::string_view>(other_views),
                              "left_only",
                              std::string(right_suffix));
  return orphans.features(source.features());
}

detail::NexusRelationshipAuditProfile build_relationship_audit_profile(const detail::NexusImpl& src,
                                                                       const DataFrame& left_frame,
                                                                       const DataFrame& right_frame,
                                                                       const Relationship& rel,
                                                                       std::string_view where) {
  const auto left_keys = relationship_left_keys(rel);
  const auto right_keys = relationship_right_keys(rel);

  detail::NexusRelationshipAuditProfile audit;
  audit.rel = rel;
  audit.left = cached_key_audit_profile(src, left_frame, rel.left_table, left_keys, where);
  audit.right = cached_key_audit_profile(src, right_frame, rel.right_table, right_keys, where);
  audit.left_orphans = orphan_rows(
      audit.left.complete_frame,
      audit.left.keys,
      audit.right.complete_frame,
      audit.right.keys,
      "__right__");
  audit.right_orphans = orphan_rows(
      audit.right.complete_frame,
      audit.right.keys,
      audit.left.complete_frame,
      audit.left.keys,
      "__left__");
  audit.left_orphan_rows = audit.left_orphans.ncases();
  audit.right_orphan_rows = audit.right_orphans.ncases();
  audit.left_unique_required =
      rel.cardinality == RelationshipCardinality::OneToOne ||
      rel.cardinality == RelationshipCardinality::OneToMany;
  audit.right_unique_required =
      rel.cardinality == RelationshipCardinality::OneToOne ||
      rel.cardinality == RelationshipCardinality::ManyToOne;

  const auto left_unique_ok =
      !audit.left_unique_required || audit.left.duplicate_frame.ncases() == 0;
  const auto right_unique_ok =
      !audit.right_unique_required || audit.right.duplicate_frame.ncases() == 0;
  const auto left_coverage_ok =
      rel.right_optional || (audit.left.missing_rows == 0 && audit.left_orphan_rows == 0);
  const auto right_coverage_ok =
      rel.left_optional || (audit.right.missing_rows == 0 && audit.right_orphan_rows == 0);

  audit.valid = left_unique_ok && right_unique_ok && left_coverage_ok && right_coverage_ok;

  const auto any_soft_gaps =
      audit.left.missing_rows > 0 || audit.right.missing_rows > 0 ||
      audit.left_orphan_rows > 0 || audit.right_orphan_rows > 0;
  audit.computed_status = audit.valid
      ? (any_soft_gaps ? RelationshipValidationStatus::Warning
                       : RelationshipValidationStatus::Validated)
      : RelationshipValidationStatus::Invalid;

  return audit;
}

const detail::NexusRelationshipAuditProfile& cached_relationship_audit_profile(
    const detail::NexusImpl& src,
    const DataFrame& left_frame,
    const DataFrame& right_frame,
    const Relationship& rel,
    std::string_view where) {
  if (const auto it = src.relationship_audit_cache_.find(rel.name);
      it != src.relationship_audit_cache_.end()) {
    return it->second;
  }
  auto audit = build_relationship_audit_profile(src, left_frame, right_frame, rel, where);
  auto [it, _] = src.relationship_audit_cache_.emplace(rel.name, std::move(audit));
  return it->second;
}

} // namespace

bool Nexus::validate_keys(std::string_view table) const {
  const auto& src = impl_ref_();
  const auto keys = default_table_keys(src, table, "Nexus::validate_keys");
  const auto& profile = cached_key_audit_profile(src, frame_ref(src, table, "Nexus::validate_keys"), table, keys, "Nexus::validate_keys");
  return key_profile_valid(profile);
}

bool Nexus::validate_keys(std::string_view table,
                          std::initializer_list<std::string_view> keys) const {
  return validate_keys(table, pluss::msharp::RangeView::Of<std::string_view>(keys));
}

bool Nexus::validate_keys(std::string_view table,
                          pluss::msharp::RangeView::Of<std::string_view> keys) const {
  const auto& src = impl_ref_();
  const auto owned = normalize_requested_keys(keys, "Nexus::validate_keys");
  const auto& profile = cached_key_audit_profile(src, frame_ref(src, table, "Nexus::validate_keys"), table, owned, "Nexus::validate_keys");
  return key_profile_valid(profile);
}

DataFrame Nexus::detect_duplicates(std::string_view table) const {
  const auto& src = impl_ref_();
  const auto keys = default_table_keys(src, table, "Nexus::detect_duplicates");
  const auto& profile = cached_key_audit_profile(src, frame_ref(src, table, "Nexus::detect_duplicates"), table, keys, "Nexus::detect_duplicates");
  return profile.duplicate_frame;
}

DataFrame Nexus::detect_duplicates(std::string_view table,
                                   std::initializer_list<std::string_view> keys) const {
  return detect_duplicates(table, pluss::msharp::RangeView::Of<std::string_view>(keys));
}

DataFrame Nexus::detect_duplicates(std::string_view table,
                                   pluss::msharp::RangeView::Of<std::string_view> keys) const {
  const auto& src = impl_ref_();
  const auto owned = normalize_requested_keys(keys, "Nexus::detect_duplicates");
  const auto& profile = cached_key_audit_profile(src, frame_ref(src, table, "Nexus::detect_duplicates"), table, owned, "Nexus::detect_duplicates");
  return profile.duplicate_frame;
}

DataFrame Nexus::grain_audit(std::string_view table) const {
  const auto& src = impl_ref_();
  require_known_table(src, table, "Nexus::grain_audit");
  const auto meta_it = src.table_meta_.find(std::string(table));
  PLUSS_CHECK(meta_it != src.table_meta_.end(),
              "Nexus::grain_audit: no metadata declared for table '" + std::string(table) + "'");

  std::vector<std::string> table_names;
  std::vector<std::string> audit_names;
  std::vector<std::string> signatures;
  std::vector<std::int64_t> key_widths;
  std::vector<std::int64_t> total_rows;
  std::vector<std::int64_t> complete_rows;
  std::vector<std::int64_t> missing_rows;
  std::vector<std::int64_t> duplicate_rows;
  std::vector<bool> valid_rows;
  const auto table_frame = frame(table);

  const auto push_profile = [&](std::string audit_name, std::vector<std::string> keys) {
    const auto& profile = cached_key_audit_profile(src, table_frame, table, keys, "Nexus::grain_audit");
    table_names.push_back(std::string(table));
    audit_names.push_back(std::move(audit_name));
    signatures.push_back(key_signature(profile.keys));
    key_widths.push_back(static_cast<std::int64_t>(profile.keys.size()));
    total_rows.push_back(profile.total_rows);
    complete_rows.push_back(profile.complete_rows);
    missing_rows.push_back(profile.missing_rows);
    duplicate_rows.push_back(profile.duplicate_frame.ncases());
    valid_rows.push_back(key_profile_valid(profile));
  };

  if (!meta_it->second.primary_key.empty()) {
    push_profile("primary", meta_it->second.primary_key);
  }
  for (std::size_t i = 0; i < meta_it->second.candidate_keys.size(); ++i) {
    push_profile("candidate_" + std::to_string(i), meta_it->second.candidate_keys[i]);
  }

  PLUSS_CHECK(!table_names.empty(),
              "Nexus::grain_audit: no key declarations available for table '" + std::string(table) + "'");

  return DataFrame({
      {"table", Thread(pluss::make_note(table_names))},
      {"key_name", Thread(pluss::make_note(audit_names))},
      {"key_signature", Thread(pluss::make_note(signatures))},
      {"key_width", Thread(pluss::make_series(key_widths))},
      {"rows", Thread(pluss::make_series(total_rows))},
      {"complete_rows", Thread(pluss::make_series(complete_rows))},
      {"missing_rows", Thread(pluss::make_series(missing_rows))},
      {"duplicate_rows", Thread(pluss::make_series(duplicate_rows))},
      {"valid", Thread(pluss::make_series(valid_rows))},
  });
}

bool Nexus::validate_relationship(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto rel = relationship(name);
  PLUSS_CHECK(rel.has_value(),
              "Nexus::validate_relationship: unknown relationship '" + std::string(name) + "'");
  return cached_relationship_audit_profile(
             src,
             frame_ref(src, rel->left_table, "Nexus::validate_relationship"),
             frame_ref(src, rel->right_table, "Nexus::validate_relationship"),
             *rel,
             "Nexus::validate_relationship")
      .valid;
}

DataFrame Nexus::detect_orphans(std::string_view relationship_name,
                                RelationshipSide side) const {
  const auto& src = impl_ref_();
  const auto rel = relationship(relationship_name);
  PLUSS_CHECK(rel.has_value(),
              "Nexus::detect_orphans: unknown relationship '" + std::string(relationship_name) + "'");
  const auto& audit = cached_relationship_audit_profile(
      src,
      frame_ref(src, rel->left_table, "Nexus::detect_orphans"),
      frame_ref(src, rel->right_table, "Nexus::detect_orphans"),
      *rel,
      "Nexus::detect_orphans");
  if (side == RelationshipSide::Left) {
    return audit.left_orphans;
  }
  return audit.right_orphans;
}

DataFrame Nexus::relationship_audit(std::string_view name) const {
  const auto& src = impl_ref_();
  const auto rel = relationship(name);
  PLUSS_CHECK(rel.has_value(),
              "Nexus::relationship_audit: unknown relationship '" + std::string(name) + "'");
  const auto& audit = cached_relationship_audit_profile(
      src,
      frame_ref(src, rel->left_table, "Nexus::relationship_audit"),
      frame_ref(src, rel->right_table, "Nexus::relationship_audit"),
      *rel,
      "Nexus::relationship_audit");

  const auto left_unique_valid =
      !audit.left_unique_required || audit.left.duplicate_frame.ncases() == 0;
  const auto right_unique_valid =
      !audit.right_unique_required || audit.right.duplicate_frame.ncases() == 0;

  return DataFrame({
      {"relationship", Thread(pluss::make_note(std::vector<std::string>{audit.rel.name}))},
      {"left_table", Thread(pluss::make_note(std::vector<std::string>{audit.rel.left_table}))},
      {"right_table", Thread(pluss::make_note(std::vector<std::string>{audit.rel.right_table}))},
      {"cardinality", Thread(pluss::make_note(std::vector<std::string>{cardinality_string(audit.rel.cardinality)}))},
      {"declared_status", Thread(pluss::make_note(std::vector<std::string>{validation_status_string(audit.rel.status)}))},
      {"computed_status", Thread(pluss::make_note(std::vector<std::string>{validation_status_string(audit.computed_status)}))},
      {"left_rows", Thread(pluss::make_series(std::vector<std::int64_t>{audit.left.total_rows}))},
      {"right_rows", Thread(pluss::make_series(std::vector<std::int64_t>{audit.right.total_rows}))},
      {"left_complete_rows", Thread(pluss::make_series(std::vector<std::int64_t>{audit.left.complete_rows}))},
      {"right_complete_rows", Thread(pluss::make_series(std::vector<std::int64_t>{audit.right.complete_rows}))},
      {"left_missing_keys", Thread(pluss::make_series(std::vector<std::int64_t>{audit.left.missing_rows}))},
      {"right_missing_keys", Thread(pluss::make_series(std::vector<std::int64_t>{audit.right.missing_rows}))},
      {"left_duplicate_rows", Thread(pluss::make_series(std::vector<std::int64_t>{audit.left.duplicate_frame.ncases()}))},
      {"right_duplicate_rows", Thread(pluss::make_series(std::vector<std::int64_t>{audit.right.duplicate_frame.ncases()}))},
      {"left_orphans", Thread(pluss::make_series(std::vector<std::int64_t>{audit.left_orphan_rows}))},
      {"right_orphans", Thread(pluss::make_series(std::vector<std::int64_t>{audit.right_orphan_rows}))},
      {"left_unique_required", Thread(pluss::make_series(std::vector<bool>{audit.left_unique_required}))},
      {"right_unique_required", Thread(pluss::make_series(std::vector<bool>{audit.right_unique_required}))},
      {"left_unique_valid", Thread(pluss::make_series(std::vector<bool>{left_unique_valid}))},
      {"right_unique_valid", Thread(pluss::make_series(std::vector<bool>{right_unique_valid}))},
      {"left_optional", Thread(pluss::make_series(std::vector<bool>{audit.rel.left_optional}))},
      {"right_optional", Thread(pluss::make_series(std::vector<bool>{audit.rel.right_optional}))},
      {"valid", Thread(pluss::make_series(std::vector<bool>{audit.valid}))},
  });
}
