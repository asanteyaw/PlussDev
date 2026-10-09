// Ragged-native persistence: one file per stream (plus its label as a second
// column) and a small manifest. Lengths, nulls, kinds and labels round-trip
// exactly; nothing is padded or trimmed. Files are named by position so any key
// text is safe; the manifest maps keys to files.

namespace {

constexpr const char* kStreamManifest = "streams.csv";
constexpr const char* kStreamValue = "value";
constexpr const char* kStreamLabel = "label";

std::string stream_store_extension(const std::string& kind) {
  if (kind == "parquet") return ".parquet";
  if (kind == "csv") return ".csv";
  if (kind == "json") return ".json";
  PLUSS_FAIL("Catalog::save_streams: unsupported store kind '" + kind + "' (use parquet, csv or json)");
}

std::string stream_kind_text(StreamKind k) {
  switch (k) {
    case StreamKind::Series: return "series";
    case StreamKind::Note: return "note";
    case StreamKind::Timeline: return "timeline";
    case StreamKind::Duration: return "duration";
  }
  return "unknown";
}

}  // namespace

void Catalog::save_streams(std::string_view directory, std::string_view store_kind) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto kind = lowercase_copy(trim_copy(store_kind));
  const auto ext = stream_store_extension(kind);
  const std::filesystem::path dir{std::string(directory)};
  std::filesystem::create_directories(dir);

  std::vector<std::string> keys, kinds, files, dtypes, has_labels;
  std::vector<std::int64_t> records;
  const auto& names = this->keys();
  for (std::size_t i = 0; i < names.size(); ++i) {
    const Stream s = at(names[i]);
    const auto file = fmt::format("stream_{:06d}{}", i, ext);
    std::vector<std::pair<std::string, pluss::table::Slate>> cols;
    if (kind != "parquet") {
      // A single column with a null renders as a blank line, which text readers
      // skip; a position column keeps every record's line non-empty.
      std::vector<std::int64_t> pos(static_cast<std::size_t>(s.size()));
      std::iota(pos.begin(), pos.end(), std::int64_t{0});
      cols.emplace_back("pos", pluss::table::Slate(pluss::make_series(pos, {s.size()})));
    }
    cols.emplace_back(kStreamValue, s.clear_label().to_slate());
    if (s.has_label()) cols.emplace_back(kStreamLabel, *s.label());
    pluss::table::DataFrame(std::move(cols)).save_as(kind, (dir / file).string());
    keys.push_back(names[i]);
    kinds.push_back(stream_kind_text(s.kind()));
    files.push_back(file);
    dtypes.push_back(s.to_slate().dtype());
    has_labels.push_back(s.has_label() ? "true" : "false");
    records.push_back(s.size());
  }
  auto manifest = pluss::table::DataFrame({
      {"key", pluss::table::Slate(pluss::make_note(keys))},
      {"kind", pluss::table::Slate(pluss::make_note(kinds))},
      {"records", pluss::table::Slate(pluss::make_series(records, {static_cast<std::int64_t>(records.size())}))},
      {"file", pluss::table::Slate(pluss::make_note(files))},
      {"has_label", pluss::table::Slate(pluss::make_note(has_labels))},
      {"dtype", pluss::table::Slate(pluss::make_note(dtypes))},
      {"store", pluss::table::Slate(pluss::make_note(std::vector<std::string>(keys.size(), kind)))},
  });
  manifest.save_csv((dir / kStreamManifest).string());
}

Catalog Catalog::load_streams(std::string_view directory) {
  namespace snap = pluss::table::snapshot;
  const std::filesystem::path dir{std::string(directory)};
  const auto manifest_path = dir / kStreamManifest;
  PLUSS_CHECK(std::filesystem::exists(manifest_path),
              "Catalog::load_streams: no " + std::string(kStreamManifest) + " in '" + dir.string() + "'");
  const auto manifest = snap::load_snapshot_csv(manifest_path.string());
  std::vector<std::pair<std::string, Stream>> streams;
  for (std::int64_t i = 0; i < manifest.ncases(); ++i) {
    const auto key = snap::note_cell(manifest, "key", i);
    const auto file = snap::note_cell(manifest, "file", i);
    const auto store = snap::note_cell(manifest, "store", i);
    const auto expected = snap::int_cell(manifest, "records", i);
    const bool labeled = snap::bool_cell(manifest, "has_label", i);
    const auto path = dir / file;
    PLUSS_CHECK(std::filesystem::exists(path),
                "Catalog::load_streams: missing stream file '" + path.string() + "' for key '" + key + "'");
    auto frame = pluss::table::DataFrame::load(store, path.string());
    PLUSS_CHECK(frame.ncases() == expected,
                fmt::format("Catalog::load_streams: '{}' has {} records, manifest says {}", key,
                            frame.ncases(), expected));
    std::optional<pluss::table::Slate> label;
    if (labeled) label = frame.features(kStreamLabel);
    streams.emplace_back(key, Stream::from_slate(frame.features(kStreamValue), std::move(label)));
  }
  return Catalog(std::move(streams));
}
