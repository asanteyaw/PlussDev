namespace {

std::int64_t table_meta_score(const std::optional<TableMeta>& meta) {
  if (!meta.has_value()) {
    return 0;
  }
  std::int64_t score = 0;
  score += meta->grain.has_value() ? 1 : 0;
  score += !meta->primary_key.empty() ? 1 : 0;
  score += !meta->candidate_keys.empty() ? 1 : 0;
  score += meta->time_field.has_value() ? 1 : 0;
  score += meta->case_labels != CaseLabelPolicy::Unknown ? 1 : 0;
  score += meta->source.has_value() ? 1 : 0;
  score += meta->provenance.has_value() ? 1 : 0;
  score += meta->note.has_value() ? 1 : 0;
  return score;
}

bool table_meta_ready(const std::optional<TableMeta>& meta) {
  return meta.has_value() && meta->grain.has_value() && !meta->primary_key.empty();
}

std::int64_t dataframe_missing_cells(const DataFrame& frame) {
  if (frame.nfeatures() == 0 || frame.ncases() == 0) {
    return 0;
  }
  return frame.isnull().sum(Axis::Case).sum().to_int();
}

double safe_ratio(std::int64_t top, std::int64_t bottom) {
  return bottom <= 0 ? 0.0 : static_cast<double>(top) / static_cast<double>(bottom);
}

std::optional<TableMeta> maybe_table_meta(const detail::NexusImpl& src, std::string_view table) {
  if (const auto it = src.table_meta_.find(std::string(table)); it != src.table_meta_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::string maybe_text(const std::optional<std::string>& value) {
  return value.value_or("");
}

std::string relationship_key_signature(
    const std::vector<std::pair<std::string, std::string>>& keys) {
  std::string out;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i > 0) {
      out += " + ";
    }
    out += keys[i].first + " -> " + keys[i].second;
  }
  return out;
}

struct NexusDeducedFeatureState {
  bool unique{false};
  bool declared{false};
};

std::string deduce_feature_cache_key(std::string_view table, std::string_view feature) {
  return std::string(table) + '\n' + std::string(feature);
}

bool declared_as_single_key(const std::optional<TableMeta>& meta, std::string_view feature);

const NexusDeducedFeatureState& cached_deduce_feature_state(
    const detail::NexusImpl& src,
    const DataFrame& frame,
    std::string_view table,
    const std::optional<TableMeta>& meta,
    std::string_view feature,
    std::unordered_map<std::string, NexusDeducedFeatureState>& cache,
    std::string_view where) {
  const auto cache_key = deduce_feature_cache_key(table, feature);
  if (const auto it = cache.find(cache_key); it != cache.end()) {
    return it->second;
  }
  const auto single = std::vector<std::string>{std::string(feature)};
  const auto& profile = cached_key_audit_profile(src, frame, table, single, where);
  auto [it, _] = cache.emplace(
      cache_key,
      NexusDeducedFeatureState{
          .unique = key_profile_valid(profile),
          .declared = declared_as_single_key(meta, feature),
      });
  return it->second;
}

bool declared_as_single_key(const std::optional<TableMeta>& meta, std::string_view feature) {
  if (!meta.has_value()) {
    return false;
  }
  if (meta->primary_key.size() == 1 && meta->primary_key.front() == feature) {
    return true;
  }
  return std::any_of(
      meta->candidate_keys.begin(),
      meta->candidate_keys.end(),
      [&](const std::vector<std::string>& keyset) {
        return keyset.size() == 1 && keyset.front() == feature;
      });
}

RelationshipCardinality deduced_cardinality(bool left_unique, bool right_unique) {
  if (left_unique && right_unique) {
    return RelationshipCardinality::OneToOne;
  }
  if (left_unique) {
    return RelationshipCardinality::OneToMany;
  }
  if (right_unique) {
    return RelationshipCardinality::ManyToOne;
  }
  return RelationshipCardinality::ManyToMany;
}

double deduced_confidence(bool left_unique,
                          bool right_unique,
                          bool left_declared,
                          bool right_declared) {
  double score = 0.35;
  score += left_unique ? 0.2 : 0.0;
  score += right_unique ? 0.2 : 0.0;
  score += left_declared ? 0.125 : 0.0;
  score += right_declared ? 0.125 : 0.0;
  return std::min(1.0, score);
}

std::string deduced_basis(bool left_unique,
                          bool right_unique,
                          bool left_declared,
                          bool right_declared) {
  std::vector<std::string> parts{"shared_name"};
  if (left_unique) {
    parts.push_back("left_unique");
  }
  if (right_unique) {
    parts.push_back("right_unique");
  }
  if (left_declared) {
    parts.push_back("left_declared");
  }
  if (right_declared) {
    parts.push_back("right_declared");
  }
  return join_compact(parts, '+');
}

struct NexusRelationshipCounts {
  std::unordered_map<std::string, std::int64_t> outgoing{};
  std::unordered_map<std::string, std::int64_t> incoming{};
};

NexusRelationshipCounts relationship_counts(const detail::NexusImpl& src) {
  NexusRelationshipCounts out;
  out.outgoing.reserve(src.keys_.size() * 2 + 1);
  out.incoming.reserve(src.keys_.size() * 2 + 1);
  for (const auto& key : src.keys_) {
    out.outgoing.emplace(key, 0);
    out.incoming.emplace(key, 0);
  }
  for (const auto& rel : src.relationships_) {
    ++out.outgoing[rel.left_table];
    ++out.incoming[rel.right_table];
  }
  return out;
}

void warm_declared_key_profiles(const detail::NexusImpl& src) {
  for (const auto& key : src.keys_) {
    const auto meta = maybe_table_meta(src, key);
    if (!meta.has_value()) {
      continue;
    }
    const auto& frame = frame_ref(src, key, "Nexus::warm");
    if (!meta->primary_key.empty()) {
      (void)cached_key_audit_profile(src, frame, key, meta->primary_key, "Nexus::warm");
    }
    for (const auto& candidate : meta->candidate_keys) {
      if (!candidate.empty()) {
        (void)cached_key_audit_profile(src, frame, key, candidate, "Nexus::warm");
      }
    }
  }
  for (const auto& rel : src.relationships_) {
    (void)cached_relationship_audit_profile(
        src,
        frame_ref(src, rel.left_table, "Nexus::warm"),
        frame_ref(src, rel.right_table, "Nexus::warm"),
        rel,
        "Nexus::warm");
  }
}

} // namespace

DataFrame Nexus::profile() const {
  const auto& src = impl_ref_();
  if (src.profile_cache_.has_value()) {
    return *src.profile_cache_;
  }
  warm_declared_key_profiles(src);
  const auto& adjacency = nexus_relationship_adjacency(src);
  const auto rel_counts = relationship_counts(src);

  std::vector<std::string> names;
  std::vector<std::int64_t> rows;
  std::vector<std::int64_t> cols;
  std::vector<std::int64_t> null_cells;
  std::vector<double> null_frac;
  std::vector<std::int64_t> meta_score;
  std::vector<bool> meta_ready;
  std::vector<std::string> primary_keys;
  std::vector<std::int64_t> key_missing_rows;
  std::vector<std::int64_t> key_duplicate_rows;
  std::vector<std::int64_t> links;
  std::vector<std::int64_t> left_links;
  std::vector<std::int64_t> right_links;
  std::vector<std::string> time_fields;
  std::vector<std::string> fingerprints;

  names.reserve(src.keys_.size());
  rows.reserve(src.keys_.size());
  cols.reserve(src.keys_.size());
  null_cells.reserve(src.keys_.size());
  null_frac.reserve(src.keys_.size());
  meta_score.reserve(src.keys_.size());
  meta_ready.reserve(src.keys_.size());
  primary_keys.reserve(src.keys_.size());
  key_missing_rows.reserve(src.keys_.size());
  key_duplicate_rows.reserve(src.keys_.size());
  links.reserve(src.keys_.size());
  left_links.reserve(src.keys_.size());
  right_links.reserve(src.keys_.size());
  time_fields.reserve(src.keys_.size());
  fingerprints.reserve(src.keys_.size());

  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    const auto& key = src.keys_[i];
    const auto& frame = src.frames_[i];
    const auto meta = maybe_table_meta(src, key);
    const auto missing = dataframe_missing_cells(frame);
    const auto total_cells = frame.ncases() * frame.nfeatures();
    std::int64_t primary_missing = -1;
    std::int64_t primary_duplicates = -1;
    std::string primary_signature;
    if (meta.has_value() && !meta->primary_key.empty()) {
      const auto& profile = cached_key_audit_profile(src, frame, key, meta->primary_key, "Nexus::profile");
      primary_missing = profile.missing_rows;
      primary_duplicates = profile.duplicate_frame.ncases();
      primary_signature = key_signature(profile.keys);
    }

    names.push_back(key);
    rows.push_back(frame.ncases());
    cols.push_back(frame.nfeatures());
    null_cells.push_back(missing);
    null_frac.push_back(safe_ratio(missing, total_cells));
    meta_score.push_back(table_meta_score(meta));
    meta_ready.push_back(table_meta_ready(meta));
    primary_keys.push_back(std::move(primary_signature));
    key_missing_rows.push_back(primary_missing);
    key_duplicate_rows.push_back(primary_duplicates);
    links.push_back(static_cast<std::int64_t>(adjacency.find(key) == adjacency.end() ? 0 : adjacency.at(key).size()));
    left_links.push_back(rel_counts.outgoing.contains(key) ? rel_counts.outgoing.at(key) : 0);
    right_links.push_back(rel_counts.incoming.contains(key) ? rel_counts.incoming.at(key) : 0);
    time_fields.push_back(meta.has_value() ? maybe_text(meta->time_field) : "");
    fingerprints.push_back(hex64(fnv1a64(table_fingerprint_text(key, frame, meta))).substr(0, 8));
  }

  src.profile_cache_ = DataFrame({
      {"table", Thread(pluss::make_note(names))},
      {"rows", Thread(pluss::make_series(rows))},
      {"cols", Thread(pluss::make_series(cols))},
      {"null_cells", Thread(pluss::make_series(null_cells))},
      {"null_frac", Thread(pluss::make_series(null_frac))},
      {"meta_score", Thread(pluss::make_series(meta_score))},
      {"meta_ready", Thread(pluss::make_series(meta_ready))},
      {"primary_key", Thread(pluss::make_note(primary_keys))},
      {"key_missing_rows", Thread(pluss::make_series(key_missing_rows))},
      {"key_duplicate_rows", Thread(pluss::make_series(key_duplicate_rows))},
      {"links", Thread(pluss::make_series(links))},
      {"left_links", Thread(pluss::make_series(left_links))},
      {"right_links", Thread(pluss::make_series(right_links))},
      {"time_field", Thread(pluss::make_note(time_fields))},
      {"fingerprint", Thread(pluss::make_note(fingerprints))},
  });
  return *src.profile_cache_;
}

DataFrame Nexus::scan() const {
  const auto& src = impl_ref_();
  if (src.scan_cache_.has_value()) {
    return *src.scan_cache_;
  }
  warm_declared_key_profiles(src);

  std::vector<std::string> kinds;
  std::vector<std::string> names;
  std::vector<std::string> lefts;
  std::vector<std::string> rights;
  std::vector<std::string> key_signatures;
  std::vector<std::int64_t> rows;
  std::vector<std::int64_t> cols;
  std::vector<std::int64_t> left_missing_rows;
  std::vector<std::int64_t> right_missing_rows;
  std::vector<std::int64_t> left_duplicate_rows;
  std::vector<std::int64_t> right_duplicate_rows;
  std::vector<std::int64_t> left_orphan_rows;
  std::vector<std::int64_t> right_orphan_rows;
  std::vector<bool> valid;
  std::vector<std::string> declared_status;
  std::vector<std::string> computed_status;
  std::vector<std::int64_t> meta_scores;
  std::vector<bool> meta_ready;
  std::vector<double> confidences;
  std::vector<std::string> notes;

  const auto total_rows = src.keys_.size() + src.relationships_.size();
  kinds.reserve(total_rows);
  names.reserve(total_rows);
  lefts.reserve(total_rows);
  rights.reserve(total_rows);
  key_signatures.reserve(total_rows);
  rows.reserve(total_rows);
  cols.reserve(total_rows);
  left_missing_rows.reserve(total_rows);
  right_missing_rows.reserve(total_rows);
  left_duplicate_rows.reserve(total_rows);
  right_duplicate_rows.reserve(total_rows);
  left_orphan_rows.reserve(total_rows);
  right_orphan_rows.reserve(total_rows);
  valid.reserve(total_rows);
  declared_status.reserve(total_rows);
  computed_status.reserve(total_rows);
  meta_scores.reserve(total_rows);
  meta_ready.reserve(total_rows);
  confidences.reserve(total_rows);
  notes.reserve(total_rows);

  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    const auto& key = src.keys_[i];
    const auto& frame = src.frames_[i];
    const auto meta = maybe_table_meta(src, key);
    std::int64_t primary_missing = -1;
    std::int64_t primary_duplicates = -1;
    bool primary_valid = false;
    std::string signature;
    std::string status = "undeclared";
    if (meta.has_value() && !meta->primary_key.empty()) {
      const auto& profile = cached_key_audit_profile(src, frame, key, meta->primary_key, "Nexus::scan");
      primary_missing = profile.missing_rows;
      primary_duplicates = profile.duplicate_frame.ncases();
      primary_valid = key_profile_valid(profile);
      signature = key_signature(profile.keys);
      status = primary_valid ? "validated" : "invalid";
    }

    kinds.push_back("table");
    names.push_back(key);
    lefts.push_back(key);
    rights.emplace_back();
    key_signatures.push_back(std::move(signature));
    rows.push_back(frame.ncases());
    cols.push_back(frame.nfeatures());
    left_missing_rows.push_back(primary_missing);
    right_missing_rows.push_back(-1);
    left_duplicate_rows.push_back(primary_duplicates);
    right_duplicate_rows.push_back(-1);
    left_orphan_rows.push_back(-1);
    right_orphan_rows.push_back(-1);
    valid.push_back(primary_valid);
    declared_status.push_back("table");
    computed_status.push_back(std::move(status));
    meta_scores.push_back(table_meta_score(meta));
    meta_ready.push_back(table_meta_ready(meta));
    confidences.push_back(1.0);
    notes.push_back(meta.has_value() ? maybe_text(meta->note) : "");
  }

  for (const auto& rel : src.relationships_) {
    const auto& audit = cached_relationship_audit_profile(
        src,
        frame_ref(src, rel.left_table, "Nexus::scan"),
        frame_ref(src, rel.right_table, "Nexus::scan"),
        rel,
        "Nexus::scan");

    kinds.push_back("relationship");
    names.push_back(rel.name);
    lefts.push_back(rel.left_table);
    rights.push_back(rel.right_table);
    key_signatures.push_back(relationship_key_signature(rel.keys));
    rows.push_back(-1);
    cols.push_back(-1);
    left_missing_rows.push_back(audit.left.missing_rows);
    right_missing_rows.push_back(audit.right.missing_rows);
    left_duplicate_rows.push_back(audit.left.duplicate_frame.ncases());
    right_duplicate_rows.push_back(audit.right.duplicate_frame.ncases());
    left_orphan_rows.push_back(audit.left_orphan_rows);
    right_orphan_rows.push_back(audit.right_orphan_rows);
    valid.push_back(audit.valid);
    declared_status.push_back(validation_status_string(rel.status));
    computed_status.push_back(validation_status_string(audit.computed_status));
    meta_scores.push_back(0);
    meta_ready.push_back(false);
    confidences.push_back(rel.confidence);
    notes.push_back(maybe_text(rel.note));
  }

  src.scan_cache_ = DataFrame({
      {"kind", Thread(pluss::make_note(kinds))},
      {"name", Thread(pluss::make_note(names))},
      {"left", Thread(pluss::make_note(lefts))},
      {"right", Thread(pluss::make_note(rights))},
      {"key_signature", Thread(pluss::make_note(key_signatures))},
      {"rows", Thread(pluss::make_series(rows))},
      {"cols", Thread(pluss::make_series(cols))},
      {"left_missing_rows", Thread(pluss::make_series(left_missing_rows))},
      {"right_missing_rows", Thread(pluss::make_series(right_missing_rows))},
      {"left_duplicate_rows", Thread(pluss::make_series(left_duplicate_rows))},
      {"right_duplicate_rows", Thread(pluss::make_series(right_duplicate_rows))},
      {"left_orphans", Thread(pluss::make_series(left_orphan_rows))},
      {"right_orphans", Thread(pluss::make_series(right_orphan_rows))},
      {"valid", Thread(pluss::make_series(valid))},
      {"declared_status", Thread(pluss::make_note(declared_status))},
      {"computed_status", Thread(pluss::make_note(computed_status))},
      {"meta_score", Thread(pluss::make_series(meta_scores))},
      {"meta_ready", Thread(pluss::make_series(meta_ready))},
      {"confidence", Thread(pluss::make_series(confidences))},
      {"note", Thread(pluss::make_note(notes))},
  });
  return *src.scan_cache_;
}

DataFrame Nexus::deduce(double min_confidence) const {
  const auto& src = impl_ref_();
  PLUSS_CHECK(min_confidence >= 0.0 && min_confidence <= 1.0,
              "Nexus::deduce: min_confidence must be in [0, 1]");
  if (min_confidence == 0.5 && src.deduce_cache_.has_value()) {
    return *src.deduce_cache_;
  }
  warm_declared_key_profiles(src);
  const auto& feature_owners = nexus_feature_owners(src);

  std::vector<std::string> names;
  std::vector<std::string> lefts;
  std::vector<std::string> rights;
  std::vector<std::string> left_keys;
  std::vector<std::string> right_keys;
  std::vector<std::string> cards;
  std::vector<bool> left_unique;
  std::vector<bool> right_unique;
  std::vector<bool> left_declared;
  std::vector<bool> right_declared;
  std::vector<double> confidences;
  std::vector<std::string> bases;
  std::unordered_map<std::string, NexusDeducedFeatureState> state_cache;
  state_cache.reserve(src.keys_.size() * 8 + 1);

  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    const auto& left_table = src.keys_[i];
    const auto& left_frame = src.frames_[i];
    const auto left_meta = maybe_table_meta(src, left_table);
    for (const auto& feature : left_frame.features()) {
      const auto owners_it = feature_owners.find(feature);
      if (owners_it == feature_owners.end() || owners_it->second.size() <= 1) {
        continue;
      }
      const auto& left_state = cached_deduce_feature_state(
          src, left_frame, left_table, left_meta, feature, state_cache, "Nexus::deduce");
      for (const auto owner_index : owners_it->second) {
        if (owner_index <= i) {
          continue;
        }
        const auto& right_table = src.keys_[owner_index];
        const auto& right_frame = src.frames_[owner_index];
        const auto right_meta = maybe_table_meta(src, right_table);
        const auto& right_state = cached_deduce_feature_state(
            src, right_frame, right_table, right_meta, feature, state_cache, "Nexus::deduce");
        const auto confidence = deduced_confidence(
            left_state.unique, right_state.unique, left_state.declared, right_state.declared);
        if (confidence < min_confidence) {
          continue;
        }
        names.push_back(left_table + "_" + right_table + "_" + feature);
        lefts.push_back(left_table);
        rights.push_back(right_table);
        left_keys.push_back(feature);
        right_keys.push_back(feature);
        cards.push_back(cardinality_string(
            deduced_cardinality(left_state.unique, right_state.unique)));
        left_unique.push_back(left_state.unique);
        right_unique.push_back(right_state.unique);
        left_declared.push_back(left_state.declared);
        right_declared.push_back(right_state.declared);
        confidences.push_back(confidence);
        bases.push_back(deduced_basis(
            left_state.unique, right_state.unique, left_state.declared, right_state.declared));
      }
    }
  }

  auto out = DataFrame({
      {"name", Thread(pluss::make_note(names))},
      {"left", Thread(pluss::make_note(lefts))},
      {"right", Thread(pluss::make_note(rights))},
      {"left_key", Thread(pluss::make_note(left_keys))},
      {"right_key", Thread(pluss::make_note(right_keys))},
      {"cardinality", Thread(pluss::make_note(cards))},
      {"left_unique", Thread(pluss::make_series(left_unique))},
      {"right_unique", Thread(pluss::make_series(right_unique))},
      {"left_declared", Thread(pluss::make_series(left_declared))},
      {"right_declared", Thread(pluss::make_series(right_declared))},
      {"confidence", Thread(pluss::make_series(confidences))},
      {"basis", Thread(pluss::make_note(bases))},
  });
  if (min_confidence == 0.5) {
    src.deduce_cache_ = out;
  }
  return out;
}

DataFrame Nexus::neighbors(std::string_view table) const {
  const auto& src = impl_ref_();
  require_known_table(src, table, "Nexus::neighbors");
  if (const auto it = src.neighbors_cache_.find(std::string(table));
      it != src.neighbors_cache_.end()) {
    return it->second;
  }

  const auto& adjacency = nexus_relationship_adjacency(src);
  std::vector<std::string> anchor;
  std::vector<std::string> relationships;
  std::vector<std::string> others;
  std::vector<std::string> cards;
  std::vector<std::string> keys;
  std::vector<bool> reversed;
  std::vector<bool> from_optional;
  std::vector<bool> to_optional;
  std::vector<double> confidences;
  std::vector<std::string> statuses;
  std::vector<std::string> notes;

  if (const auto adj_it = adjacency.find(std::string(table)); adj_it != adjacency.end()) {
    const auto& edges = adj_it->second;
    anchor.reserve(edges.size());
    relationships.reserve(edges.size());
    others.reserve(edges.size());
    cards.reserve(edges.size());
    keys.reserve(edges.size());
    reversed.reserve(edges.size());
    from_optional.reserve(edges.size());
    to_optional.reserve(edges.size());
    confidences.reserve(edges.size());
    statuses.reserve(edges.size());
    notes.reserve(edges.size());

    for (const auto& edge : edges) {
      const auto rel_it = src.rel_name_to_pos_.find(edge.relationship_name);
      PLUSS_CHECK(rel_it != src.rel_name_to_pos_.end(),
                  "Nexus::neighbors: internal relationship index mismatch");
      const auto& rel = src.relationships_[static_cast<std::size_t>(rel_it->second)];
      const auto oriented_keys = oriented_relationship_keys(rel, edge.reversed);
      anchor.push_back(std::string(table));
      relationships.push_back(rel.name);
      others.push_back(edge.next);
      cards.push_back(cardinality_string(edge.reversed
          ? reverse_relationship_cardinality(rel.cardinality)
          : rel.cardinality));
      keys.push_back(relationship_key_signature(oriented_keys));
      reversed.push_back(edge.reversed);
      from_optional.push_back(edge.reversed ? rel.right_optional : rel.left_optional);
      to_optional.push_back(edge.reversed ? rel.left_optional : rel.right_optional);
      confidences.push_back(rel.confidence);
      statuses.push_back(validation_status_string(rel.status));
      notes.push_back(maybe_text(rel.note));
    }
  }

  auto out = DataFrame({
      {"table", Thread(pluss::make_note(anchor))},
      {"relationship", Thread(pluss::make_note(relationships))},
      {"other", Thread(pluss::make_note(others))},
      {"cardinality", Thread(pluss::make_note(cards))},
      {"keys", Thread(pluss::make_note(keys))},
      {"reversed", Thread(pluss::make_series(reversed))},
      {"from_optional", Thread(pluss::make_series(from_optional))},
      {"to_optional", Thread(pluss::make_series(to_optional))},
      {"confidence", Thread(pluss::make_series(confidences))},
      {"status", Thread(pluss::make_note(statuses))},
      {"note", Thread(pluss::make_note(notes))},
  });
  auto [it, _] = src.neighbors_cache_.emplace(std::string(table), out);
  return it->second;
}
