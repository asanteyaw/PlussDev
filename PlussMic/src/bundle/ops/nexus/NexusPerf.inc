namespace {

constexpr char kNexusCacheSep = '\x1d';

void invalidate_nexus_runtime(detail::NexusImpl& dst) {
  dst.key_profile_cache_.clear();
  dst.relationship_audit_cache_.clear();
  dst.link_cache_.clear();
  dst.neighbors_cache_.clear();
  dst.profile_cache_.reset();
  dst.scan_cache_.reset();
  dst.deduce_cache_.reset();
  dst.feature_owners_.clear();
  dst.feature_owners_built_ = false;
}

std::string nexus_key_profile_cache_key(std::string_view table,
                                        const std::vector<std::string>& keys) {
  std::string out(table);
  out.push_back(kNexusCacheSep);
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i > 0) {
      out.push_back(kNexusCacheSep);
    }
    out += keys[i];
  }
  return out;
}

const relational::LinkAdjacency& nexus_relationship_adjacency(const detail::NexusImpl& src) {
  return src.link_cache_.adjacency(src.keys_, src.relationships_);
}

const std::unordered_map<std::string, std::vector<std::size_t>>&
nexus_feature_owners(const detail::NexusImpl& src) {
  if (!src.feature_owners_built_) {
    src.feature_owners_.clear();
    std::size_t total_features = 0;
    for (const auto& frame : src.frames_) {
      total_features += static_cast<std::size_t>(frame.nfeatures());
    }
    src.feature_owners_.reserve(total_features * 2 + 1);
    for (std::size_t i = 0; i < src.frames_.size(); ++i) {
      for (const auto& feature : src.frames_[i].features()) {
        src.feature_owners_[feature].push_back(i);
      }
    }
    src.feature_owners_built_ = true;
  }
  return src.feature_owners_;
}

} // namespace
