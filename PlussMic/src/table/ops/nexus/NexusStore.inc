namespace {

constexpr char kNexusInnerSep = '\x1f';
constexpr char kNexusOuterSep = '\x1e';

std::string trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

std::string lower_trimmed(std::string_view text) {
  auto out = trim_ascii(text);
  for (auto& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

std::string store_kind_string(StoreKind kind) {
  switch (kind) {
    case StoreKind::Csv:
      return "csv";
    case StoreKind::Json:
      return "json";
    case StoreKind::Parquet:
      return "parquet";
  }
  return "json";
}

StoreKind parse_store_kind(std::string_view text) {
  const auto kind = lower_trimmed(text);
  if (kind == "csv") {
    return StoreKind::Csv;
  }
  if (kind == "json") {
    return StoreKind::Json;
  }
  if (kind == "parquet" || kind == "pq") {
    return StoreKind::Parquet;
  }
  PLUSS_FAIL("Nexus::load: unsupported store kind '" + std::string(text) + "'");
}

std::string join_compact(const std::vector<std::string>& values, char sep) {
  std::string out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out.push_back(sep);
    }
    out += values[i];
  }
  return out;
}

std::vector<std::string> split_compact(std::string_view text, char sep) {
  std::vector<std::string> out;
  if (text.empty()) {
    return out;
  }
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto pos = text.find(sep, start);
    if (pos == std::string_view::npos) {
      out.emplace_back(text.substr(start));
      break;
    }
    out.emplace_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return out;
}

std::string encode_candidate_keys(const std::vector<std::vector<std::string>>& keys) {
  std::vector<std::string> encoded;
  encoded.reserve(keys.size());
  for (const auto& keyset : keys) {
    encoded.push_back(join_compact(keyset, kNexusInnerSep));
  }
  return join_compact(encoded, kNexusOuterSep);
}

std::vector<std::vector<std::string>> decode_candidate_keys(std::string_view text) {
  std::vector<std::vector<std::string>> out;
  for (const auto& keyset : split_compact(text, kNexusOuterSep)) {
    out.push_back(split_compact(keyset, kNexusInnerSep));
  }
  return out;
}

std::string encode_relationship_keys(const std::vector<std::pair<std::string, std::string>>& keys) {
  std::vector<std::string> encoded;
  encoded.reserve(keys.size());
  for (const auto& [left, right] : keys) {
    encoded.push_back(left + std::string(1, kNexusInnerSep) + right);
  }
  return join_compact(encoded, kNexusOuterSep);
}

std::vector<std::pair<std::string, std::string>> decode_relationship_keys(std::string_view text) {
  std::vector<std::pair<std::string, std::string>> out;
  for (const auto& item : split_compact(text, kNexusOuterSep)) {
    const auto parts = split_compact(item, kNexusInnerSep);
    PLUSS_CHECK(parts.size() == 2,
                "Nexus::load: malformed relationship key payload");
    out.emplace_back(parts[0], parts[1]);
  }
  return out;
}

std::string case_label_policy_string(CaseLabelPolicy policy) {
  switch (policy) {
    case CaseLabelPolicy::Unknown:
      return "unknown";
    case CaseLabelPolicy::ImplicitRange:
      return "implicit_range";
    case CaseLabelPolicy::ExternalAxis:
      return "external_axis";
    case CaseLabelPolicy::FeatureDerived:
      return "feature_derived";
    case CaseLabelPolicy::Structured:
      return "structured";
  }
  return "unknown";
}

CaseLabelPolicy parse_case_label_policy(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value.empty() || value == "unknown") {
    return CaseLabelPolicy::Unknown;
  }
  if (value == "implicit_range") {
    return CaseLabelPolicy::ImplicitRange;
  }
  if (value == "external_axis") {
    return CaseLabelPolicy::ExternalAxis;
  }
  if (value == "feature_derived") {
    return CaseLabelPolicy::FeatureDerived;
  }
  if (value == "structured") {
    return CaseLabelPolicy::Structured;
  }
  PLUSS_FAIL("Nexus::load: unsupported case-label policy '" + std::string(text) + "'");
}

std::string relationship_cardinality_string(RelationshipCardinality cardinality) {
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
  return "many_to_many";
}

RelationshipCardinality parse_relationship_cardinality(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value == "one_to_one") {
    return RelationshipCardinality::OneToOne;
  }
  if (value == "one_to_many") {
    return RelationshipCardinality::OneToMany;
  }
  if (value == "many_to_one") {
    return RelationshipCardinality::ManyToOne;
  }
  if (value.empty() || value == "many_to_many") {
    return RelationshipCardinality::ManyToMany;
  }
  PLUSS_FAIL("Nexus::load: unsupported relationship cardinality '" + std::string(text) + "'");
}

RelationshipValidationStatus parse_relationship_status(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value.empty() || value == "declared") {
    return RelationshipValidationStatus::Declared;
  }
  if (value == "validated") {
    return RelationshipValidationStatus::Validated;
  }
  if (value == "warning") {
    return RelationshipValidationStatus::Warning;
  }
  if (value == "invalid") {
    return RelationshipValidationStatus::Invalid;
  }
  PLUSS_FAIL("Nexus::load: unsupported relationship status '" + std::string(text) + "'");
}

JoinHow parse_join_how(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value.empty() || value == "inner") {
    return JoinHow::Inner;
  }
  if (value == "left") {
    return JoinHow::Left;
  }
  if (value == "right") {
    return JoinHow::Right;
  }
  if (value == "outer") {
    return JoinHow::Outer;
  }
  if (value == "cross") {
    return JoinHow::Cross;
  }
  if (value == "left_only") {
    return JoinHow::LeftOnly;
  }
  if (value == "right_only") {
    return JoinHow::RightOnly;
  }
  PLUSS_FAIL("Nexus::load: unsupported join kind '" + std::string(text) + "'");
}

std::string optional_text(const std::optional<std::string>& value) {
  return value.value_or("");
}

std::optional<std::string> parse_optional_text(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  return std::string(text);
}

std::string dtypes_string(const DataFrame& frame) {
  std::vector<std::string> items;
  for (const auto& [name, dtype] : frame.dtypes().entries()) {
    items.push_back(name + std::string(1, kNexusInnerSep) + dtype);
  }
  return join_compact(items, kNexusOuterSep);
}

std::string safe_table_file(std::string_view key, std::size_t index, StoreKind kind) {
  std::string stem;
  stem.reserve(key.size());
  for (const char c : key) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
      stem.push_back(c);
    } else {
      stem.push_back('_');
    }
  }
  if (stem.empty()) {
    stem = "table";
  }
  return std::to_string(index) + "_" + stem + "." + store_kind_string(kind);
}

std::uint64_t fnv1a64(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const unsigned char c : text) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string hex64(std::uint64_t value) {
  constexpr char digits[] = "0123456789abcdef";
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i) {
    out[static_cast<std::size_t>(i)] = digits[value & 0xfULL];
    value >>= 4ULL;
  }
  return out;
}

std::string table_fingerprint_text(std::string_view key,
                                   const DataFrame& frame,
                                   const std::optional<TableMeta>& meta) {
  std::string out = std::string(key);
  out += "|";
  out += std::to_string(frame.ncases());
  out += "|";
  out += std::to_string(frame.nfeatures());
  out += "|";
  out += join_compact(frame.features(), kNexusInnerSep);
  out += "|";
  out += dtypes_string(frame);
  if (meta.has_value()) {
    out += "|";
    out += optional_text(meta->grain);
    out += "|";
    out += join_compact(meta->primary_key, kNexusInnerSep);
    out += "|";
    out += encode_candidate_keys(meta->candidate_keys);
    out += "|";
    out += optional_text(meta->time_field);
    out += "|";
    out += case_label_policy_string(meta->case_labels);
    out += "|";
    out += optional_text(meta->source);
    out += "|";
    out += optional_text(meta->provenance);
    out += "|";
    out += optional_text(meta->note);
  }
  return out;
}

std::string relationship_fingerprint_text(const Relationship& rel) {
  std::string out = rel.name;
  out += "|";
  out += rel.left_table;
  out += "|";
  out += rel.right_table;
  out += "|";
  out += encode_relationship_keys(rel.keys);
  out += "|";
  out += relationship_cardinality_string(rel.cardinality);
  out += "|";
  out += rel.left_optional ? "1" : "0";
  out += "|";
  out += rel.right_optional ? "1" : "0";
  out += "|";
  out += std::to_string(rel.confidence);
  out += "|";
  out += validation_status_string(rel.status);
  out += "|";
  out += optional_text(rel.note);
  return out;
}

std::string join_plan_fingerprint_text(const std::string& name, const JoinPlan& plan) {
  std::string out = name;
  out += "|";
  out += plan.root_table;
  out += "|";
  out += plan.result_name;
  for (const auto& step : plan.steps) {
    out += "|";
    out += step.left_table;
    out += ">";
    out += step.right_table;
    out += "|";
    out += join_how_token(step.spec.how);
    out += "|";
    out += encode_relationship_keys(step.spec.keys);
    out += "|";
    out += step.spec.right_suffix;
    out += "|";
    out += step.spec.relationship_name.value_or("");
    out += "|";
    out += step.spec.validate.has_value()
        ? relationship_cardinality_string(*step.spec.validate)
        : "";
    out += "|";
    out += step.spec.result_name.value_or("");
  }
  return out;
}

std::string workspace_fingerprint_text(const detail::NexusImpl& src) {
  std::string out;
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    const auto meta = [&]() -> std::optional<TableMeta> {
      if (const auto it = src.table_meta_.find(src.keys_[i]); it != src.table_meta_.end()) {
        return it->second;
      }
      return std::nullopt;
    }();
    out += table_fingerprint_text(src.keys_[i], src.frames_[i], meta);
    out.push_back('\n');
  }
  out += "--\n";
  for (const auto& rel : src.relationships_) {
    out += relationship_fingerprint_text(rel);
    out.push_back('\n');
  }
  out += "--views--\n";
  std::vector<std::string> view_names;
  view_names.reserve(src.saved_views_.size());
  for (const auto& [name, _] : src.saved_views_) {
    view_names.push_back(name);
  }
  std::sort(view_names.begin(), view_names.end());
  for (const auto& name : view_names) {
    out += join_plan_fingerprint_text(name, src.saved_views_.at(name));
    out.push_back('\n');
  }
  return out;
}

ThreadValue::HostAny snapshot_cell_host(const DataFrame& frame, std::string_view feature, std::int64_t row) {
  PLUSS_CHECK(frame.get(feature).has_value(),
              "Nexus::load: missing snapshot feature '" + std::string(feature) + "'");
  const auto column = frame.features(feature);
  PLUSS_CHECK(row >= 0 && row < column.size(),
              "Nexus::load: snapshot row out of bounds for feature '" + std::string(feature) + "'");
  const auto index = row;
  return column.take(std::span<const std::int64_t>(&index, 1)).value().host();
}

std::string note_cell(const DataFrame& frame, std::string_view feature, std::int64_t row) {
  return snapshot_cell_host(frame, feature, row).to_str();
}

pluss::loader::ReadCsvOptions snapshot_read_options() {
  pluss::loader::ReadCsvOptions options;
  options.engine = pluss::loader::CsvEngine::Serial;
  options.infer_datetime_format = false;
  return options;
}

DataFrame load_snapshot_csv(std::string_view filepath) {
  const auto raw = pluss::loader::read_csv_raw_batch(filepath, snapshot_read_options());
  std::vector<std::pair<std::string, Thread>> features;
  features.reserve(raw.features.size());
  for (const auto& column : raw.features) {
    if (column.note_values.has_value()) {
      features.emplace_back(column.name, Thread(*column.note_values));
    } else {
      features.emplace_back(column.name, Thread(pluss::make_note(column.values)));
    }
  }
  return DataFrame(std::move(features));
}

std::int64_t int_cell(const DataFrame& frame, std::string_view feature, std::int64_t row) {
  const auto host = snapshot_cell_host(frame, feature, row);
  if (host.kind() == ThreadValue::HostAny::Kind::String) {
    return std::stoll(trim_ascii(host.to_str()));
  }
  return host.to_int();
}

double double_cell(const DataFrame& frame, std::string_view feature, std::int64_t row) {
  const auto host = snapshot_cell_host(frame, feature, row);
  if (host.kind() == ThreadValue::HostAny::Kind::String) {
    return std::stod(trim_ascii(host.to_str()));
  }
  return host.to_dbl();
}

bool bool_cell(const DataFrame& frame, std::string_view feature, std::int64_t row) {
  const auto column = frame.features(feature);
  PLUSS_CHECK(row >= 0 && row < column.size(),
              "Nexus::load: snapshot row out of bounds for feature '" + std::string(feature) + "'");
  const auto index = row;
  const auto host = column.take(std::span<const std::int64_t>(&index, 1)).value().host();
  switch (host.kind()) {
    case ThreadValue::HostAny::Kind::Bool:
    case ThreadValue::HostAny::Kind::Int:
    case ThreadValue::HostAny::Kind::UInt:
    case ThreadValue::HostAny::Kind::Double:
      return host.to_int() != 0;
    case ThreadValue::HostAny::Kind::String: {
      const auto value = lower_trimmed(host.to_str());
      if (value == "1" || value == "true") {
        return true;
      }
      if (value.empty() || value == "0" || value == "false") {
        return false;
      }
      PLUSS_FAIL("Nexus::load: malformed boolean value '" + value + "'");
    }
    case ThreadValue::HostAny::Kind::Time:
      PLUSS_FAIL("Nexus::load: time snapshots cannot be decoded as boolean");
  }
  PLUSS_FAIL("Nexus::load: unsupported boolean snapshot scalar kind");
}

} // namespace

DataFrame Nexus::schema() const {
  const auto& src = impl_ref_();
  std::vector<std::string> names;
  std::vector<std::int64_t> rows;
  std::vector<std::int64_t> cols;
  std::vector<std::string> feats;
  std::vector<std::string> dtypes;
  std::vector<std::string> grains;
  std::vector<std::string> pkeys;
  std::vector<std::string> ckeys;
  std::vector<std::string> times;
  std::vector<std::string> labels;
  std::vector<std::string> sources;
  std::vector<std::string> provenances;
  std::vector<std::string> notes;
  std::vector<std::string> fingerprints;

  const auto workspace_fp = hex64(fnv1a64(workspace_fingerprint_text(src)));

  names.reserve(src.keys_.size());
  rows.reserve(src.keys_.size());
  cols.reserve(src.keys_.size());
  feats.reserve(src.keys_.size());
  dtypes.reserve(src.keys_.size());
  grains.reserve(src.keys_.size());
  pkeys.reserve(src.keys_.size());
  ckeys.reserve(src.keys_.size());
  times.reserve(src.keys_.size());
  labels.reserve(src.keys_.size());
  sources.reserve(src.keys_.size());
  provenances.reserve(src.keys_.size());
  notes.reserve(src.keys_.size());
  fingerprints.reserve(src.keys_.size());

  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    names.push_back(src.keys_[i]);
    rows.push_back(src.frames_[i].ncases());
    cols.push_back(src.frames_[i].nfeatures());
    feats.push_back(join_compact(src.frames_[i].features(), kNexusInnerSep));
    dtypes.push_back(dtypes_string(src.frames_[i]));
    const auto meta_it = src.table_meta_.find(src.keys_[i]);
    const auto meta = meta_it != src.table_meta_.end() ? std::optional<TableMeta>(meta_it->second) : std::nullopt;
    grains.push_back(meta.has_value() ? optional_text(meta->grain) : "");
    pkeys.push_back(meta.has_value() ? join_compact(meta->primary_key, kNexusInnerSep) : "");
    ckeys.push_back(meta.has_value() ? encode_candidate_keys(meta->candidate_keys) : "");
    times.push_back(meta.has_value() ? optional_text(meta->time_field) : "");
    labels.push_back(meta.has_value() ? case_label_policy_string(meta->case_labels) : "unknown");
    sources.push_back(meta.has_value() ? optional_text(meta->source) : "");
    provenances.push_back(meta.has_value() ? optional_text(meta->provenance) : "");
    notes.push_back(meta.has_value() ? optional_text(meta->note) : "");
    fingerprints.push_back(hex64(fnv1a64(table_fingerprint_text(src.keys_[i], src.frames_[i], meta))) + "_" + workspace_fp);
  }

  return DataFrame({
      {"table", Thread(pluss::make_note(names))},
      {"rows", Thread(pluss::make_series(rows))},
      {"cols", Thread(pluss::make_series(cols))},
      {"feats", Thread(pluss::make_note(feats))},
      {"dtypes", Thread(pluss::make_note(dtypes))},
      {"grain", Thread(pluss::make_note(grains))},
      {"pkey", Thread(pluss::make_note(pkeys))},
      {"ckeys", Thread(pluss::make_note(ckeys))},
      {"time", Thread(pluss::make_note(times))},
      {"labels", Thread(pluss::make_note(labels))},
      {"source", Thread(pluss::make_note(sources))},
      {"provenance", Thread(pluss::make_note(provenances))},
      {"note", Thread(pluss::make_note(notes))},
      {"fingerprint", Thread(pluss::make_note(fingerprints))},
  });
}

DataFrame Nexus::graph() const {
  const auto& src = impl_ref_();
  std::vector<std::string> names;
  std::vector<std::string> lefts;
  std::vector<std::string> rights;
  std::vector<std::string> keys;
  std::vector<std::string> cards;
  std::vector<bool> left_optional;
  std::vector<bool> right_optional;
  std::vector<double> confidences;
  std::vector<std::string> statuses;
  std::vector<std::string> notes;

  names.reserve(src.relationships_.size());
  lefts.reserve(src.relationships_.size());
  rights.reserve(src.relationships_.size());
  keys.reserve(src.relationships_.size());
  cards.reserve(src.relationships_.size());
  left_optional.reserve(src.relationships_.size());
  right_optional.reserve(src.relationships_.size());
  confidences.reserve(src.relationships_.size());
  statuses.reserve(src.relationships_.size());
  notes.reserve(src.relationships_.size());

  for (const auto& rel : src.relationships_) {
    names.push_back(rel.name);
    lefts.push_back(rel.left_table);
    rights.push_back(rel.right_table);
    keys.push_back(encode_relationship_keys(rel.keys));
    cards.push_back(relationship_cardinality_string(rel.cardinality));
    left_optional.push_back(rel.left_optional);
    right_optional.push_back(rel.right_optional);
    confidences.push_back(rel.confidence);
    statuses.push_back(validation_status_string(rel.status));
    notes.push_back(optional_text(rel.note));
  }

  return DataFrame({
      {"name", Thread(pluss::make_note(names))},
      {"left", Thread(pluss::make_note(lefts))},
      {"right", Thread(pluss::make_note(rights))},
      {"keys", Thread(pluss::make_note(keys))},
      {"cardinality", Thread(pluss::make_note(cards))},
      {"left_optional", Thread(pluss::make_series(left_optional))},
      {"right_optional", Thread(pluss::make_series(right_optional))},
      {"confidence", Thread(pluss::make_series(confidences))},
      {"status", Thread(pluss::make_note(statuses))},
      {"note", Thread(pluss::make_note(notes))},
  });
}

std::string Nexus::fingerprint() const {
  return hex64(fnv1a64(workspace_fingerprint_text(impl_ref_())));
}

void Nexus::save(std::string_view directory, const StoreOptions& options) const {
  const auto& src = impl_ref_();
  const auto root = std::filesystem::path(std::string(directory));
  const auto tables_dir = root / options.tables;
  std::filesystem::create_directories(tables_dir);

  const auto kind = store_kind_string(options.kind);
  for (std::size_t i = 0; i < src.keys_.size(); ++i) {
    const auto path = tables_dir / safe_table_file(src.keys_[i], i, options.kind);
    src.frames_[i].save_as(kind, path.string());
  }

  schema().save_csv((root / options.schema).string());
  graph().save_csv((root / options.graph).string());
  {
    std::vector<std::string> view_names;
    std::vector<std::int64_t> step_indices;
    std::vector<std::string> roots;
    std::vector<std::string> results;
    std::vector<std::string> lefts;
    std::vector<std::string> rights;
    std::vector<std::string> hows;
    std::vector<std::string> keys;
    std::vector<std::string> suffixes;
    std::vector<std::string> validates;
    std::vector<std::string> relationships;
    std::vector<std::string> step_results;

    std::vector<std::string> ordered;
    ordered.reserve(src.saved_views_.size());
    for (const auto& [name, _] : src.saved_views_) {
      ordered.push_back(name);
    }
    std::sort(ordered.begin(), ordered.end());

    for (const auto& name : ordered) {
      const auto& plan = src.saved_views_.at(name);
      if (plan.steps.empty()) {
        view_names.push_back(name);
        step_indices.push_back(-1);
        roots.push_back(plan.root_table);
        results.push_back(plan.result_name);
        lefts.emplace_back();
        rights.emplace_back();
        hows.emplace_back();
        keys.emplace_back();
        suffixes.emplace_back();
        validates.emplace_back();
        relationships.emplace_back();
        step_results.emplace_back();
        continue;
      }
      for (std::size_t i = 0; i < plan.steps.size(); ++i) {
        const auto& step = plan.steps[i];
        view_names.push_back(name);
        step_indices.push_back(static_cast<std::int64_t>(i));
        roots.push_back(plan.root_table);
        results.push_back(plan.result_name);
        lefts.push_back(step.left_table);
        rights.push_back(step.right_table);
        hows.push_back(join_how_token(step.spec.how));
        keys.push_back(encode_relationship_keys(step.spec.keys));
        suffixes.push_back(step.spec.right_suffix);
        validates.push_back(step.spec.validate.has_value()
            ? relationship_cardinality_string(*step.spec.validate)
            : "");
        relationships.push_back(step.spec.relationship_name.value_or(""));
        step_results.push_back(step.spec.result_name.value_or(""));
      }
    }

    DataFrame({
        {"view", Thread(pluss::make_note(view_names))},
        {"step", Thread(pluss::make_series(step_indices))},
        {"root", Thread(pluss::make_note(roots))},
        {"result", Thread(pluss::make_note(results))},
        {"left", Thread(pluss::make_note(lefts))},
        {"right", Thread(pluss::make_note(rights))},
        {"how", Thread(pluss::make_note(hows))},
        {"keys", Thread(pluss::make_note(keys))},
        {"right_suffix", Thread(pluss::make_note(suffixes))},
        {"validate", Thread(pluss::make_note(validates))},
        {"relationship", Thread(pluss::make_note(relationships))},
        {"step_result", Thread(pluss::make_note(step_results))},
    }).save_csv((root / options.views).string());
  }
  DataFrame({
      {"kind", Thread(pluss::make_note(std::vector<std::string>{kind}))},
      {"fingerprint", Thread(pluss::make_note(std::vector<std::string>{fingerprint()}))},
      {"ntables", Thread(pluss::make_series(std::vector<std::int64_t>{ntables()}))},
      {"nlinks", Thread(pluss::make_series(std::vector<std::int64_t>{nrelationships()}))},
      {"tables", Thread(pluss::make_note(std::vector<std::string>{options.tables}))},
      {"schema", Thread(pluss::make_note(std::vector<std::string>{options.schema}))},
      {"graph", Thread(pluss::make_note(std::vector<std::string>{options.graph}))},
      {"views", Thread(pluss::make_note(std::vector<std::string>{options.views}))},
  }).save_csv((root / options.manifest).string());
}

Nexus Nexus::load(std::string_view directory, const StoreOptions& options) {
  const auto root = std::filesystem::path(std::string(directory));
  const auto manifest = load_snapshot_csv((root / options.manifest).string());
  PLUSS_CHECK(manifest.ncases() == 1, "Nexus::load: manifest must contain exactly one row");

  const auto kind = parse_store_kind(note_cell(manifest, "kind", 0));
  const auto tables_dir = note_cell(manifest, "tables", 0);
  const auto schema_file = note_cell(manifest, "schema", 0);
  const auto graph_file = note_cell(manifest, "graph", 0);
  const auto views_file = manifest.get("views").has_value() ? note_cell(manifest, "views", 0) : "";

  const auto schema_frame = load_snapshot_csv((root / schema_file).string());
  const auto graph_frame = load_snapshot_csv((root / graph_file).string());

  std::vector<std::pair<std::string, DataFrame>> tables;
  tables.reserve(static_cast<std::size_t>(schema_frame.ncases()));
  for (std::int64_t i = 0; i < schema_frame.ncases(); ++i) {
    const auto name = note_cell(schema_frame, "table", i);
    const auto file = (root / tables_dir / safe_table_file(name, static_cast<std::size_t>(i), kind)).string();
    tables.emplace_back(name, DataFrame::load(store_kind_string(kind), file));
  }

  auto nx = Nexus(std::move(tables));
  for (std::int64_t i = 0; i < schema_frame.ncases(); ++i) {
    TableMeta meta;
    meta.grain = parse_optional_text(note_cell(schema_frame, "grain", i));
    meta.primary_key = split_compact(note_cell(schema_frame, "pkey", i), kNexusInnerSep);
    meta.candidate_keys = decode_candidate_keys(note_cell(schema_frame, "ckeys", i));
    meta.time_field = parse_optional_text(note_cell(schema_frame, "time", i));
    meta.case_labels = parse_case_label_policy(note_cell(schema_frame, "labels", i));
    meta.source = parse_optional_text(note_cell(schema_frame, "source", i));
    meta.provenance = parse_optional_text(note_cell(schema_frame, "provenance", i));
    meta.note = parse_optional_text(note_cell(schema_frame, "note", i));

    const auto table = note_cell(schema_frame, "table", i);
    if (meta.grain.has_value() || !meta.primary_key.empty() || !meta.candidate_keys.empty() ||
        meta.time_field.has_value() || meta.case_labels != CaseLabelPolicy::Unknown ||
        meta.source.has_value() || meta.provenance.has_value() || meta.note.has_value()) {
      nx = nx.meta(table, meta);
    }
  }

  std::vector<Relationship> rels;
  rels.reserve(static_cast<std::size_t>(graph_frame.ncases()));
  for (std::int64_t i = 0; i < graph_frame.ncases(); ++i) {
    Relationship rel;
    rel.name = note_cell(graph_frame, "name", i);
    rel.left_table = note_cell(graph_frame, "left", i);
    rel.right_table = note_cell(graph_frame, "right", i);
    rel.keys = decode_relationship_keys(note_cell(graph_frame, "keys", i));
    rel.cardinality = parse_relationship_cardinality(note_cell(graph_frame, "cardinality", i));
    rel.left_optional = bool_cell(graph_frame, "left_optional", i);
    rel.right_optional = bool_cell(graph_frame, "right_optional", i);
    rel.confidence = double_cell(graph_frame, "confidence", i);
    rel.status = parse_relationship_status(note_cell(graph_frame, "status", i));
    rel.note = parse_optional_text(note_cell(graph_frame, "note", i));
    rels.push_back(std::move(rel));
  }
  if (!rels.empty()) {
    for (const auto& rel : rels) {
      nx = nx.links(rel);
    }
  }

  if (!views_file.empty() && std::filesystem::exists(root / views_file)) {
    const auto views_frame = load_snapshot_csv((root / views_file).string());
    std::unordered_map<std::string, JoinPlan> plans;
    std::vector<std::string> order;
    for (std::int64_t i = 0; i < views_frame.ncases(); ++i) {
      const auto name = note_cell(views_frame, "view", i);
      auto [it, inserted] = plans.emplace(name, JoinPlan{});
      if (inserted) {
        order.push_back(name);
        it->second.root_table = note_cell(views_frame, "root", i);
        it->second.result_name = note_cell(views_frame, "result", i);
      }
      const auto step_index = int_cell(views_frame, "step", i);
      if (step_index < 0) {
        continue;
      }
      JoinSpec spec;
      spec.how = parse_join_how(note_cell(views_frame, "how", i));
      spec.keys = decode_relationship_keys(note_cell(views_frame, "keys", i));
      spec.right_suffix = note_cell(views_frame, "right_suffix", i);
      spec.relationship_name = parse_optional_text(note_cell(views_frame, "relationship", i));
      const auto validate_text = note_cell(views_frame, "validate", i);
      if (!validate_text.empty()) {
        spec.validate = parse_relationship_cardinality(validate_text);
      }
      spec.result_name = parse_optional_text(note_cell(views_frame, "step_result", i));
      it->second.steps.push_back(JoinPlanStep{
          .left_table = note_cell(views_frame, "left", i),
          .right_table = note_cell(views_frame, "right", i),
          .spec = std::move(spec),
      });
    }
    for (const auto& name : order) {
      nx = nx.store(name, plans.at(name));
    }
  }
  return nx;
}
