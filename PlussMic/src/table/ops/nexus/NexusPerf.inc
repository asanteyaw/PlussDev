namespace {

constexpr char kNexusCacheSep = '\x1d';

void invalidate_nexus_runtime(detail::NexusImpl& dst) {
  dst.key_profile_cache_.clear();
  dst.relationship_audit_cache_.clear();
  dst.path_cache_.clear();
  dst.adjacency_.clear();
  dst.adjacency_built_ = false;
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

std::string nexus_path_cache_key(std::string_view from, std::string_view to) {
  auto out = std::string(from);
  out.push_back(kNexusCacheSep);
  out += to;
  return out;
}

const std::unordered_map<std::string, std::vector<detail::NexusAdjacencyLink>>&
nexus_relationship_adjacency(const detail::NexusImpl& src) {
  if (!src.adjacency_built_) {
    src.adjacency_.clear();
    src.adjacency_.reserve(src.keys_.size() * 2 + 1);
    for (const auto& key : src.keys_) {
      src.adjacency_.try_emplace(key);
    }
    for (const auto& rel : src.relationships_) {
      src.adjacency_[rel.left_table].push_back(detail::NexusAdjacencyLink{
          .next = rel.right_table,
          .relationship_name = rel.name,
          .reversed = false,
      });
      src.adjacency_[rel.right_table].push_back(detail::NexusAdjacencyLink{
          .next = rel.left_table,
          .relationship_name = rel.name,
          .reversed = true,
      });
    }
    src.adjacency_built_ = true;
  }
  return src.adjacency_;
}

} // namespace
