Catalog Catalog::clean(std::string_view field) const {
  return keys(field).map([](const Stream& s) {
    const auto t = s.to_thread().cast().note().as_note();
    std::vector<std::string> out;
    out.reserve(static_cast<std::size_t>(s.size()));
    for (std::int64_t i = 0; i < s.size(); ++i) {
      auto tokens = tokenize_ascii_words(t.at(i));
      std::string joined;
      for (std::size_t j = 0; j < tokens.size(); ++j) {
        if (j) joined.push_back(' ');
        joined += tokens[j];
      }
      out.push_back(std::move(joined));
    }
    return Stream(pluss::make_note(out), detail::validity_mask(s), s.label());
  });
}

Catalog Catalog::keywords(std::string_view field, int top_n) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::keywords: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::keywords: unknown key '" + key + "'");
  const auto src = at(key).to_thread().cast().note().as_note();

  std::unordered_map<std::string, std::int64_t> counts;
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    for (const auto& tok : tokenize_ascii_words(src.at(i))) {
      if (!tok.empty()) counts[tok] += 1;
    }
  }

  std::vector<std::pair<std::string, std::int64_t>> items(counts.begin(), counts.end());
  std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    return a.first < b.first;
  });

  const auto limit = std::max<std::int64_t>(0, static_cast<std::int64_t>(top_n));
  if (limit > 0 && static_cast<std::int64_t>(items.size()) > limit) {
    items.resize(static_cast<std::size_t>(limit));
  }

  std::vector<std::string> words;
  std::vector<std::int64_t> freqs;
  words.reserve(items.size());
  freqs.reserve(items.size());
  for (const auto& [w, c] : items) {
    words.push_back(w);
    freqs.push_back(c);
  }

  std::vector<std::pair<std::string, Stream>> out_cols;
  out_cols.reserve(2);
  out_cols.emplace_back("keyword", Stream(pluss::make_note(words)));
  out_cols.emplace_back("count",
                        Stream(pluss::make_series(freqs, {static_cast<std::int64_t>(freqs.size())})));
  return Catalog(std::move(out_cols));
}

Catalog Catalog::similarity(std::string_view field1,
                            std::string_view field2,
                            std::string_view method) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto left_name = trim_copy(field1);
  const auto right_name = trim_copy(field2);
  PLUSS_CHECK(!left_name.empty() && !right_name.empty(),
              "Catalog::similarity: field names cannot be empty");
  PLUSS_CHECK(has(left_name), "Catalog::similarity: unknown field '" + left_name + "'");
  PLUSS_CHECK(has(right_name), "Catalog::similarity: unknown field '" + right_name + "'");
  PLUSS_CHECK(lowercase_copy(trim_copy(method)) == "cosine",
              "Catalog::similarity currently supports only method='cosine'");

  const auto lhs = at(left_name).to_thread().cast().note().as_note();
  const auto rhs = at(right_name).to_thread().cast().note().as_note();
  PLUSS_CHECK(lhs.numel() == rhs.numel(), "Catalog::similarity: fields must have same length");

  std::vector<double> sim;
  sim.reserve(static_cast<std::size_t>(lhs.numel()));
  for (std::int64_t i = 0; i < lhs.numel(); ++i) {
    std::unordered_map<std::string, double> lv;
    std::unordered_map<std::string, double> rv;
    for (const auto& tok : tokenize_ascii_words(lhs.at(i))) lv[tok] += 1.0;
    for (const auto& tok : tokenize_ascii_words(rhs.at(i))) rv[tok] += 1.0;

    double dot = 0.0;
    double ln = 0.0;
    double rn = 0.0;
    for (const auto& [w, v] : lv) {
      ln += v * v;
      auto it = rv.find(w);
      if (it != rv.end()) dot += v * it->second;
    }
    for (const auto& [_, v] : rv) rn += v * v;
    const double denom = std::sqrt(ln) * std::sqrt(rn);
    sim.push_back(denom > 0.0 ? dot / denom : 0.0);
  }

  const auto sim_name = left_name + "__" + right_name + "_similarity";
  return set(sim_name, Stream(pluss::make_series(sim, {static_cast<std::int64_t>(sim.size())})));
}

Catalog Catalog::grep(std::string_view field, std::string_view pattern) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  const auto pat = lowercase_copy(trim_copy(pattern));
  PLUSS_CHECK(!key.empty(), "Catalog::grep: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::grep: unknown field '" + key + "'");
  const auto src = at(key).to_thread().cast().note().as_note();
  std::vector<bool> keep(static_cast<std::size_t>(src.numel()), false);
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    const auto txt = lowercase_copy(src.at(i));
    keep[static_cast<std::size_t>(i)] = (txt.find(pat) != std::string::npos);
  }
  return filter(bool_series_from_mask(keep));
}

Catalog Catalog::featurize(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::featurize: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::featurize: unknown field '" + key + "'");
  const auto src = at(key).to_thread().cast().note().as_note();

  std::vector<std::int64_t> char_len;
  std::vector<std::int64_t> word_count;
  char_len.reserve(static_cast<std::size_t>(src.numel()));
  word_count.reserve(static_cast<std::size_t>(src.numel()));
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    const auto text = std::string(src.at(i));
    char_len.push_back(static_cast<std::int64_t>(text.size()));
    word_count.push_back(static_cast<std::int64_t>(tokenize_ascii_words(text).size()));
  }

  auto out = *this;
  out = out.set(key + "_char_len",
                Stream(pluss::make_series(char_len, {static_cast<std::int64_t>(char_len.size())})));
  out = out.set(key + "_word_count",
                Stream(pluss::make_series(word_count, {static_cast<std::int64_t>(word_count.size())})));
  return out;
}

Catalog Catalog::stats(std::string_view field, std::string_view stat_type) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  const auto stat = lowercase_copy(trim_copy(stat_type));
  PLUSS_CHECK(!key.empty(), "Catalog::stats: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::stats: unknown field '" + key + "'");
  const auto src = at(key).to_thread().cast().note().as_note();

  std::vector<std::int64_t> lens;
  lens.reserve(static_cast<std::size_t>(src.numel()));
  std::unordered_set<std::string> vocab;
  for (std::int64_t i = 0; i < src.numel(); ++i) {
    const auto text = std::string(src.at(i));
    lens.push_back(static_cast<std::int64_t>(text.size()));
    for (const auto& tok : tokenize_ascii_words(text)) vocab.insert(tok);
  }
  const auto n = static_cast<double>(lens.size());
  const auto minmax = std::minmax_element(lens.begin(), lens.end());
  const auto sum_len = std::accumulate(lens.begin(), lens.end(), 0.0);

  double value = 0.0;
  if (stat == "mean" || stat == "avg") {
    value = n > 0.0 ? (sum_len / n) : 0.0;
  } else if (stat == "min") {
    value = lens.empty() ? 0.0 : static_cast<double>(*minmax.first);
  } else if (stat == "max") {
    value = lens.empty() ? 0.0 : static_cast<double>(*minmax.second);
  } else if (stat == "count") {
    value = n;
  } else if (stat == "vocab" || stat == "vocab_size") {
    value = static_cast<double>(vocab.size());
  } else {
    PLUSS_FAIL("Catalog::stats: unsupported stat_type '" + stat + "'");
  }

  std::vector<std::pair<std::string, Stream>> out_cols;
  out_cols.reserve(2);
  out_cols.emplace_back("stat", Stream(pluss::make_note(std::vector<std::string>{stat})));
  out_cols.emplace_back("value", Stream(pluss::make_series(std::vector<double>{value}, {1})));
  return Catalog(std::move(out_cols));
}

Catalog Catalog::tfidf(std::string_view field) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  const auto key = trim_copy(field);
  PLUSS_CHECK(!key.empty(), "Catalog::tfidf: field cannot be empty");
  PLUSS_CHECK(has(key), "Catalog::tfidf: unknown field '" + key + "'");
  const auto src = at(key).to_thread().cast().note().as_note();
  const auto n = src.numel();

  std::vector<std::vector<std::string>> docs;
  docs.reserve(static_cast<std::size_t>(n));
  std::unordered_map<std::string, std::int64_t> df;
  for (std::int64_t i = 0; i < n; ++i) {
    auto tok = tokenize_ascii_words(src.at(i));
    docs.push_back(tok);
    std::unordered_set<std::string> seen(tok.begin(), tok.end());
    for (const auto& w : seen) df[w] += 1;
  }

  std::vector<double> norm;
  norm.reserve(static_cast<std::size_t>(n));
  for (const auto& doc : docs) {
    std::unordered_map<std::string, double> tf;
    for (const auto& w : doc) tf[w] += 1.0;
    const double denom_tf = doc.empty() ? 1.0 : static_cast<double>(doc.size());
    double sum_sq = 0.0;
    for (auto& [w, c] : tf) {
      c /= denom_tf;
      const auto it = df.find(w);
      const double idf = std::log((static_cast<double>(n) + 1.0) /
                                  (static_cast<double>(it == df.end() ? 0 : it->second) + 1.0)) + 1.0;
      const double wv = c * idf;
      sum_sq += wv * wv;
    }
    norm.push_back(std::sqrt(sum_sq));
  }

  return set(key + "_tfidf_norm",
             Stream(pluss::make_series(norm, {static_cast<std::int64_t>(norm.size())})));
}

Catalog Catalog::set_stream(std::string_view key, const Stream& value) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto out = *this;
  out.set_stream_(key, value);
  return out;
}

Catalog& Catalog::set_stream_(std::string_view key, const Stream& value) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  PLUSS_CHECK(value.defined(), "Catalog::set_stream_: stream is undefined");
  std::string k(key);
  PLUSS_CHECK(!k.empty(), "Catalog::set_stream_: key cannot be empty");

  auto& w = mut_impl_();
  auto it = w.key_to_pos_.find(k);
  if (it == w.key_to_pos_.end()) {
    const auto idx = static_cast<std::int64_t>(w.keys_.size());
    w.key_to_pos_.emplace(k, idx);
    w.keys_.push_back(std::move(k));
    w.streams_.push_back(value);
  } else {
    w.streams_[static_cast<std::size_t>(it->second)] = value;
  }
  return *this;
}

Catalog Catalog::set(std::string_view key, const Stream& value) const {
  return set_stream(key, value);
}

Catalog& Catalog::set_(std::string_view key, const Stream& value) {
  return set_stream_(key, value);
}

Catalog Catalog::set(std::initializer_list<std::pair<std::string_view, Stream>> items) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto out = *this;
  out.set_(items);
  return out;
}

Catalog& Catalog::set_(std::initializer_list<std::pair<std::string_view, Stream>> items) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  for (const auto& [k, s] : items) {
    set_stream_(k, s);
  }
  return *this;
}

Catalog Catalog::set(std::initializer_list<NamedStream> items) const {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  auto out = *this;
  out.set_(items);
  return out;
}

Catalog& Catalog::set_(std::initializer_list<NamedStream> items) {
  PLUSS_CHECK(defined(), "Catalog: undefined handle");
  for (const auto& kv : items) {
    set_stream_(kv.key, kv.value);
  }
  return *this;
}

Catalog Catalog::drop(std::initializer_list<std::string_view> keys_drop) const {
  return drop(std::span<const std::string_view>(keys_drop.begin(), keys_drop.size()));
}
