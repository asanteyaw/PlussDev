pluss::table::DataFrame Catalog::tabelize(const TabelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (empty()) return pluss::table::DataFrame(std::vector<std::pair<std::string, pluss::table::Slate>>{});

  const auto plan = build_alignment_plan(impl_ref_().keys_, impl_ref_().streams_, policy);
  auto case_axis = plan.label_axis;

  std::vector<std::pair<std::string, pluss::table::Slate>> out_cols;
  out_cols.reserve(impl_ref_().streams_.size());
  for (std::size_t i = 0; i < impl_ref_().streams_.size(); ++i) {
    auto t = build_aligned_slate(impl_ref_().streams_[i], plan.source_index_by_stream[i], policy);
    out_cols.emplace_back(impl_ref_().keys_[i], std::move(t));
  }

  if (policy.include_label_feature) {
    std::unordered_set<std::string> names;
    names.reserve(out_cols.size() * 2);
    for (const auto& c : out_cols) names.insert(c.first);
    std::string case_name = policy.label_feature;
    const bool case_name_taken = names.find(case_name) != names.end();
    if (!case_name_taken) {
      out_cols.emplace_back(case_name, case_axis);
    } else {
      switch (policy.collision) {
        case CollisionPolicy::Error:
          pluss::codem::throw_error("Catalog::tabelize: label feature name collision '" + case_name + "'");
          break;
        case CollisionPolicy::Suffix:
          case_name = unique_label_feature_name(case_name, names);
          out_cols.emplace_back(std::move(case_name), case_axis);
          break;
        case CollisionPolicy::Overwrite:
          for (auto& c : out_cols) {
            if (c.first == case_name) {
              c.second = case_axis;
              break;
            }
          }
          break;
      }
    }
  }
  return pluss::table::DataFrame(std::move(out_cols), std::move(case_axis));
}

Catalog::const_iterator Catalog::begin() const {
  return keys().begin();
}

Catalog::const_iterator Catalog::end() const {
  return keys().end();
}

const detail::CatalogImpl& Catalog::impl_ref_() const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  return *impl_;
}

detail::CatalogImpl& Catalog::mut_impl_() {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (impl_.use_count() > 1) {
    std::vector<std::pair<std::string, Stream>> copied;
    copied.reserve(impl_ref_().keys_.size());
    for (std::size_t i = 0; i < impl_ref_().keys_.size(); ++i) {
      copied.emplace_back(impl_ref_().keys_[i], impl_ref_().streams_[i]);
    }
    impl_ = pluss::msharp::make_intrusive<detail::CatalogImpl>(std::move(copied));
  }
  return *impl_;
}

pluss::table::DataFrame Catalog::tabelize(const RaggedPolicy& policy) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  (void)RaggedLayout::of(*this, policy, "Catalog::tabelize");  // Exact / loss = Error are enforced here
  return tabelize(to_tabelize(policy, "Catalog::tabelize"));
}
