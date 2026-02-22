pluss::table::DataFrame Catalog::tabelize(const TabelizePolicy& policy) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  if (empty()) return pluss::table::DataFrame(std::vector<std::pair<std::string, pluss::table::Thread>>{});

  const auto anchor_idx = resolve_anchor_idx(impl_ref_().keys_, policy);
  auto effective_index_mode = policy.index_mode;
  if (effective_index_mode == IndexMode::ByCaseLabel) {
    const bool all_have_cases = std::all_of(
        impl_ref_().streams_.begin(), impl_ref_().streams_.end(), [](const Stream& s) { return s.has_cases(); });
    if (!all_have_cases) {
      effective_index_mode = IndexMode::Positional; // default fallback policy
    }
  }

  std::int64_t target_len = 0;
  std::vector<std::string> target_labels;
  std::vector<std::vector<std::int64_t>> source_index_by_stream;
  source_index_by_stream.reserve(impl_ref_().streams_.size());

  if (effective_index_mode == IndexMode::Positional) {
    target_len = positional_target_len(impl_ref_().streams_, anchor_idx, policy.align);
    for (const auto& s : impl_ref_().streams_) {
      source_index_by_stream.push_back(positional_mapping(s.size(), target_len));
    }
  } else {
    std::vector<std::vector<std::string>> labels;
    labels.reserve(impl_ref_().streams_.size());
    std::vector<std::unordered_map<std::string, std::int64_t>> label_to_idx;
    label_to_idx.reserve(impl_ref_().streams_.size());
    for (const auto& s : impl_ref_().streams_) {
      labels.push_back(case_labels_from_stream(s));
      label_to_idx.push_back(make_unique_label_to_idx(labels.back()));
    }

    const auto& anchor_labels = labels[static_cast<std::size_t>(anchor_idx)];
    switch (policy.align) {
      case AlignPolicy::Strict: {
        for (const auto& l : labels) {
          PLUSS_CHECK(l == anchor_labels,
                      "Catalog::tabelize(strict, by_case_label): case-label order mismatch");
        }
        target_labels = anchor_labels;
        break;
      }
      case AlignPolicy::Inner: {
        for (const auto& lbl : anchor_labels) {
          bool in_all = true;
          for (const auto& lut : label_to_idx) {
            in_all = in_all && (lut.find(lbl) != lut.end());
          }
          if (in_all) target_labels.push_back(lbl);
        }
        break;
      }
      case AlignPolicy::Left:
      case AlignPolicy::Right: {
        target_labels = anchor_labels;
        break;
      }
      case AlignPolicy::Outer: {
        std::unordered_set<std::string> seen;
        target_labels.reserve(anchor_labels.size() * 2);
        for (const auto& lbl : anchor_labels) {
          seen.insert(lbl);
          target_labels.push_back(lbl);
        }
        for (const auto& l : labels) {
          for (const auto& lbl : l) {
            if (seen.insert(lbl).second) target_labels.push_back(lbl);
          }
        }
        break;
      }
    }

    target_len = static_cast<std::int64_t>(target_labels.size());
    for (const auto& lut : label_to_idx) {
      std::vector<std::int64_t> map(static_cast<std::size_t>(target_len), -1);
      for (std::int64_t i = 0; i < target_len; ++i) {
        auto it = lut.find(target_labels[static_cast<std::size_t>(i)]);
        if (it != lut.end()) map[static_cast<std::size_t>(i)] = it->second;
      }
      source_index_by_stream.push_back(std::move(map));
    }
  }

  pluss::table::Thread case_axis;
  if (effective_index_mode == IndexMode::Positional) {
    std::vector<std::int64_t> idx(static_cast<std::size_t>(target_len));
    for (std::int64_t i = 0; i < target_len; ++i) idx[static_cast<std::size_t>(i)] = i;
    case_axis = pluss::table::Thread(pluss::make_series(idx, {target_len}));
  } else {
    const std::array<std::int64_t, 1> shape{target_len};
    case_axis = pluss::table::Thread(pluss::make_note(target_labels, shape));
  }

  std::vector<std::pair<std::string, pluss::table::Thread>> out_cols;
  out_cols.reserve(impl_ref_().streams_.size());
  for (std::size_t i = 0; i < impl_ref_().streams_.size(); ++i) {
    auto t = build_aligned_thread(impl_ref_().streams_[i], source_index_by_stream[i], policy);
    out_cols.emplace_back(impl_ref_().keys_[i], std::move(t));
  }

  if (policy.include_case_feature) {
    std::unordered_set<std::string> names;
    names.reserve(out_cols.size() * 2);
    for (const auto& c : out_cols) names.insert(c.first);
    std::string case_name = policy.case_feature_name;
    const bool case_name_taken = names.find(case_name) != names.end();
    if (!case_name_taken) {
      out_cols.emplace_back(case_name, case_axis);
    } else {
      switch (policy.collision) {
        case CollisionPolicy::Error:
          pluss::codem::throw_error("Catalog::tabelize: case feature name collision '" + case_name + "'");
          break;
        case CollisionPolicy::Suffix:
          case_name = unique_case_feature_name(case_name, names);
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

