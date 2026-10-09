namespace {

constexpr char kNexusInnerSep = relational::kInnerSep;
constexpr char kNexusOuterSep = relational::kOuterSep;

// Snapshot-CSV helpers live in the table layer (table/io/SnapshotCsv.h) so the
// kindle stack and Locus read their metadata the same way.
using pluss::table::snapshot::bool_cell;
using pluss::table::snapshot::double_cell;
using pluss::table::snapshot::int_cell;
using pluss::table::snapshot::load_snapshot_csv;
using pluss::table::snapshot::lower_trimmed;
using pluss::table::snapshot::note_cell;
using pluss::table::snapshot::trim_ascii;

// Text codecs for links, table metadata and plans are shared with Locus
// (relational/Codec.h).
using relational::case_label_policy_string;
using relational::decode_candidate_keys;
using relational::decode_relationship_keys;
using relational::encode_candidate_keys;
using relational::encode_relationship_keys;
using relational::join_compact;
using relational::optional_text;
using relational::parse_case_label_policy;
using relational::parse_join_how;
using relational::parse_optional_text;
using relational::parse_relationship_cardinality;
using relational::parse_relationship_status;
using relational::parse_store_kind;
using relational::relationship_cardinality_string;
using relational::split_compact;
using relational::store_kind_string;

std::string normalize_directory_format(std::string_view text) {
  const auto value = lower_trimmed(text);
  if (value == "csv") {
    return "csv";
  }
  if (value == "txt" || value == "text" || value == "tsv") {
    return "txt";
  }
  if (value == "json") {
    return "json";
  }
  if (value == "parquet" || value == "pq") {
    return "parquet";
  }
  if (value == "excel" || value == "xlsx" || value == "xls") {
    PLUSS_FAIL("Nexus::load: Excel workbooks are not supported by the directory loader");
  }
  PLUSS_FAIL("Nexus::load: unsupported directory-load format '" + std::string(text) + "'");
}

std::optional<std::string> infer_directory_source_type(const std::filesystem::path& path) {
  const auto ext = lower_trimmed(path.extension().string());
  if (ext == ".csv") {
    return std::string("csv");
  }
  if (ext == ".txt" || ext == ".text" || ext == ".tsv") {
    return std::string("txt");
  }
  if (ext == ".json") {
    return std::string("json");
  }
  if (ext == ".parquet" || ext == ".pq") {
    return std::string("parquet");
  }
  return std::nullopt;
}

std::unordered_set<std::string> normalize_directory_formats(const std::vector<std::string>& formats) {
  std::unordered_set<std::string> out;
  if (formats.empty()) {
    out.insert("csv");
    out.insert("txt");
    out.insert("json");
    out.insert("parquet");
    return out;
  }
  for (const auto& format : formats) {
    out.insert(normalize_directory_format(format));
  }
  return out;
}

std::string normalized_relative_path(const std::filesystem::path& root, const std::filesystem::path& path) {
  std::error_code ec;
  auto rel = std::filesystem::relative(path, root, ec);
  if (ec) {
    rel = path.filename();
  }
  return lower_trimmed(rel.generic_string());
}

std::string normalize_selector_token(std::string_view text) {
  return lower_trimmed(text);
}

bool selector_matches_candidate(std::string_view selector,
                                std::string_view table_key,
                                const std::filesystem::path& path,
                                std::string_view relative) {
  const auto token = normalize_selector_token(selector);
  if (token.empty()) {
    return false;
  }
  if (token == normalize_selector_token(table_key)) {
    return true;
  }
  if (token == normalize_selector_token(path.stem().string())) {
    return true;
  }
  if (token == normalize_selector_token(path.filename().string())) {
    return true;
  }
  if (token == normalize_selector_token(relative)) {
    return true;
  }
  return false;
}

struct DirectoryLoadCandidate {
  std::filesystem::path path{};
  std::string source_type{};
  std::string table_key{};
  std::string relative{};
};

void validate_load_options(const LoadOptions& options) {
  PLUSS_CHECK(options.load.empty() || options.ignore.empty(),
              "Nexus::load: choose either LoadOptions.load or LoadOptions.ignore, not both");
}

std::vector<DirectoryLoadCandidate> collect_directory_load_candidates(
    const std::filesystem::path& root,
    const LoadOptions& options) {
  PLUSS_CHECK(std::filesystem::exists(root),
              "Nexus::load: directory '" + root.string() + "' does not exist");
  PLUSS_CHECK(std::filesystem::is_directory(root),
              "Nexus::load: path '" + root.string() + "' is not a directory");
  validate_load_options(options);

  const auto allowed_formats = normalize_directory_formats(options.formats);
  std::vector<DirectoryLoadCandidate> candidates;

  auto consider = [&](const std::filesystem::directory_entry& entry) {
    if (!entry.is_regular_file()) {
      return;
    }
    const auto inferred = infer_directory_source_type(entry.path());
    if (!inferred.has_value() || !allowed_formats.contains(*inferred)) {
      return;
    }

    DirectoryLoadCandidate candidate;
    candidate.path = entry.path();
    candidate.source_type = *inferred;
    candidate.table_key = options.keep_extension
        ? entry.path().filename().string()
        : entry.path().stem().string();
    candidate.relative = normalized_relative_path(root, entry.path());

    const bool included = options.load.empty() ||
        std::any_of(options.load.begin(), options.load.end(), [&](const auto& token) {
          return selector_matches_candidate(token, candidate.table_key, candidate.path, candidate.relative);
        });
    if (!included) {
      return;
    }
    const bool ignored = std::any_of(options.ignore.begin(), options.ignore.end(), [&](const auto& token) {
      return selector_matches_candidate(token, candidate.table_key, candidate.path, candidate.relative);
    });
    if (ignored) {
      return;
    }
    candidates.push_back(std::move(candidate));
  };

  if (options.recursive) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
      consider(entry);
    }
  } else {
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
      consider(entry);
    }
  }

  std::sort(candidates.begin(), candidates.end(), [](const auto& lhs, const auto& rhs) {
    if (lhs.table_key != rhs.table_key) {
      return lhs.table_key < rhs.table_key;
    }
    return lhs.relative < rhs.relative;
  });

  std::unordered_map<std::string, std::string> seen;
  for (const auto& candidate : candidates) {
    const auto [it, inserted] = seen.emplace(candidate.table_key, candidate.relative);
    if (!inserted) {
      PLUSS_FAIL("Nexus::load: multiple files resolve to table name '" + candidate.table_key +
                 "' (" + it->second + ", " + candidate.relative +
                 "). Use keep_extension or narrower load/ignore filters.");
    }
  }

  PLUSS_CHECK(!candidates.empty(),
              "Nexus::load: no supported raw table files were found in '" + root.string() + "'");
  return candidates;
}

std::size_t resolve_directory_loader_workers(std::size_t tasks, const LoadOptions& options) {
  if (!options.parallel || tasks <= 1) {
    return 1;
  }
  if (options.max_workers.has_value()) {
    return std::max<std::size_t>(1, std::min<std::size_t>(*options.max_workers, tasks));
  }
  const auto hw = std::thread::hardware_concurrency();
  const auto suggested = hw == 0 ? static_cast<std::size_t>(4) : std::max<std::size_t>(1, hw / 2);
  return std::max<std::size_t>(1, std::min<std::size_t>(suggested, tasks));
}

struct LoadedDirectoryTable {
  std::size_t index{0};
  std::string key{};
  pluss::table::DataFrame frame{};
};

bool directory_load_config_has_csv_engine(const pluss::loader::Config& config) {
  for (const auto& [key, _] : config) {
    const auto normalized = lower_trimmed(key);
    if (normalized == "engine" || normalized == "csvengine") {
      return true;
    }
  }
  return false;
}

pluss::loader::Config effective_directory_load_config(std::string_view source_type,
                                                      const LoadOptions& options) {
  auto config = options.config;
  const auto normalized = lower_trimmed(source_type);
  if ((normalized == "csv" || normalized == "txt") &&
      !directory_load_config_has_csv_engine(config)) {
    config.insert_or_assign("engine", pluss::loader::ConfigValue("serial"));
  }
  return config;
}

LoadedDirectoryTable load_directory_table_candidate(std::size_t index,
                                                    const DirectoryLoadCandidate& candidate,
                                                    const LoadOptions& options) {
  auto frame = pluss::table::DataFrame::load(candidate.source_type,
                                             candidate.path.string(),
                                             effective_directory_load_config(candidate.source_type, options));
  if (frame.title().empty()) {
    frame = frame.title(candidate.table_key);
  }
  return LoadedDirectoryTable{
      .index = index,
      .key = candidate.table_key,
      .frame = std::move(frame),
  };
}

Nexus load_directory_tables_impl(const std::filesystem::path& root, const LoadOptions& options) {
  const auto candidates = collect_directory_load_candidates(root, options);
  const auto workers = resolve_directory_loader_workers(candidates.size(), options);

  std::vector<LoadedDirectoryTable> loaded(candidates.size());
  if (workers == 1) {
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      loaded[i] = load_directory_table_candidate(i, candidates[i], options);
    }
  } else {
    for (std::size_t begin = 0; begin < candidates.size(); begin += workers) {
      std::vector<std::future<LoadedDirectoryTable>> batch;
      batch.reserve(workers);
      const auto end = std::min<std::size_t>(candidates.size(), begin + workers);
      for (std::size_t i = begin; i < end; ++i) {
        batch.push_back(std::async(std::launch::async, [&, i]() {
          return load_directory_table_candidate(i, candidates[i], options);
        }));
      }
      for (auto& task : batch) {
        auto result = task.get();
        loaded[result.index] = std::move(result);
      }
    }
  }

  std::vector<std::pair<std::string, pluss::table::DataFrame>> tables;
  tables.reserve(loaded.size());
  for (auto& item : loaded) {
    tables.emplace_back(std::move(item.key), std::move(item.frame));
  }
  return Nexus(std::move(tables));
}

std::string dtypes_string(const pluss::table::DataFrame& frame) {
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
                                   const pluss::table::DataFrame& frame,
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

} // namespace

pluss::table::DataFrame Nexus::schema() const {
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

  return pluss::table::DataFrame({
      {"table", pluss::table::Slate(pluss::make_note(names))},
      {"rows", pluss::table::Slate(pluss::make_series(rows))},
      {"cols", pluss::table::Slate(pluss::make_series(cols))},
      {"feats", pluss::table::Slate(pluss::make_note(feats))},
      {"dtypes", pluss::table::Slate(pluss::make_note(dtypes))},
      {"grain", pluss::table::Slate(pluss::make_note(grains))},
      {"pkey", pluss::table::Slate(pluss::make_note(pkeys))},
      {"ckeys", pluss::table::Slate(pluss::make_note(ckeys))},
      {"time", pluss::table::Slate(pluss::make_note(times))},
      {"labels", pluss::table::Slate(pluss::make_note(labels))},
      {"source", pluss::table::Slate(pluss::make_note(sources))},
      {"provenance", pluss::table::Slate(pluss::make_note(provenances))},
      {"note", pluss::table::Slate(pluss::make_note(notes))},
      {"fingerprint", pluss::table::Slate(pluss::make_note(fingerprints))},
  });
}

pluss::table::DataFrame Nexus::graph() const {
  return store::graph_frame(impl_ref_().relationships_);
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
    std::vector<std::pair<std::string, JoinPlan>> plans(src.saved_views_.begin(), src.saved_views_.end());
    std::sort(plans.begin(), plans.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    store::views_frame(plans).save_csv((root / options.views).string());
  }

  pluss::table::DataFrame({
      {"kind", pluss::table::Slate(pluss::make_note(std::vector<std::string>{kind}))},
      {"fingerprint", pluss::table::Slate(pluss::make_note(std::vector<std::string>{fingerprint()}))},
      {"ntables", pluss::table::Slate(pluss::make_series(std::vector<std::int64_t>{ntables()}))},
      {"nlinks", pluss::table::Slate(pluss::make_series(std::vector<std::int64_t>{nrelationships()}))},
      {"tables", pluss::table::Slate(pluss::make_note(std::vector<std::string>{options.tables}))},
      {"schema", pluss::table::Slate(pluss::make_note(std::vector<std::string>{options.schema}))},
      {"graph", pluss::table::Slate(pluss::make_note(std::vector<std::string>{options.graph}))},
      {"views", pluss::table::Slate(pluss::make_note(std::vector<std::string>{options.views}))},
  }).save_csv((root / options.manifest).string());
}

Nexus Nexus::read(std::string_view directory, const StoreOptions& options) {
  const auto root = std::filesystem::path(std::string(directory));
  PLUSS_CHECK(std::filesystem::exists(root / options.manifest),
              "Nexus::read: manifest '" + (root / options.manifest).string() +
                  "' was not found for store restore");
  const auto manifest = load_snapshot_csv((root / options.manifest).string());
  PLUSS_CHECK(manifest.ncases() == 1, "Nexus::read: manifest must contain exactly one row");

  const auto kind = parse_store_kind(note_cell(manifest, "kind", 0));
  const auto tables_dir = note_cell(manifest, "tables", 0);
  const auto schema_file = note_cell(manifest, "schema", 0);
  const auto graph_file = note_cell(manifest, "graph", 0);
  const auto views_file = manifest.get("views").has_value() ? note_cell(manifest, "views", 0) : "";

  const auto schema_frame = load_snapshot_csv((root / schema_file).string());
  const auto graph_frame = load_snapshot_csv((root / graph_file).string());

  std::vector<std::pair<std::string, pluss::table::DataFrame>> tables;
  tables.reserve(static_cast<std::size_t>(schema_frame.ncases()));
  for (std::int64_t i = 0; i < schema_frame.ncases(); ++i) {
    const auto name = note_cell(schema_frame, "table", i);
    const auto file = (root / tables_dir / safe_table_file(name, static_cast<std::size_t>(i), kind)).string();
    tables.emplace_back(name, pluss::table::DataFrame::load(store_kind_string(kind), file));
  }

  auto nx = Nexus(std::move(tables));
  for (auto& [table, meta] : store::parse_meta_frame(schema_frame)) {
    nx = nx.meta(table, meta);
  }
  for (const auto& rel : store::parse_graph_frame(graph_frame)) {
    nx = nx.links(rel);
  }

  if (!views_file.empty() && std::filesystem::exists(root / views_file)) {
    for (const auto& [name, plan] : store::parse_views_frame(load_snapshot_csv((root / views_file).string()))) {
      nx = nx.store(name, plan);
    }
  }
  return nx;
}

Nexus Nexus::load(std::string_view directory, const LoadOptions& options) {
  return load_directory_tables_impl(std::filesystem::path(std::string(directory)), options);
}
