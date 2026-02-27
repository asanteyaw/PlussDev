# Complete TimeIndex Method Reference: Signatures + Real Calls

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});
auto key_t = pluss::moment("2024-01-01T00:20:00Z");
auto values = pluss::value(std::vector<double>{10.0, 20.0, 30.0}, {3});
auto indices = pluss::make_series(std::vector<std::int64_t>{0,2}, {2});
auto dur = pluss::duration_minutes(30);
auto tol = pluss::duration_minutes(20);
```

- Signature: `std::int64_t searchsorted(const Timeline& key, bool right) const`
  Example: `auto out = idx.searchsorted(key_t, true);`
- Signature: `pluss::codem::Series slice(const Timeline& start, const Timeline& end) const`
  Example: `auto out = idx.slice(key_t, key_t);`
- Signature: `pluss::codem::Series slice_time(std::string_view start_iso, std::string_view end_iso) const`
  Example: `auto out = idx.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");`
- Signature: `std::int64_t asof(const Timeline& key) const`
  Example: `auto out = idx.asof(key_t);`
- Signature: `ResampleBins resample_bins(std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_bins("H", 1);`
- Signature: `pluss::codem::Series resample_count(std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_count("H", 1);`
- Signature: `pluss::codem::Series resample_sum(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_sum(values, "H", 1);`
- Signature: `pluss::codem::Series resample_mean(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_mean(values, "H", 1);`
- Signature: `pluss::codem::Series resample_min(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_min(values, "H", 1);`
- Signature: `pluss::codem::Series resample_max(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_max(values, "H", 1);`
- Signature: `pluss::codem::Series resample_first(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_first(values, "H", 1);`
- Signature: `pluss::codem::Series resample_last(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_last(values, "H", 1);`
- Signature: `pluss::codem::Series resample_ohlc(const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_ohlc(values, "H", 1);`
- Signature: `pluss::codem::Series rolling_count(const Duration& window) const`
  Example: `auto out = idx.rolling_count(dur);`
- Signature: `pluss::codem::Series rolling_sum(const pluss::codem::Series& values, const Duration& window) const`
  Example: `auto out = idx.rolling_sum(values, dur);`
- Signature: `AlignUnion align_union(const TimeIndex& other) const`
  Example: `auto out = idx.align_union(other_idx);`
- Signature: `bool is_monotonic_increasing() const`
  Example: `auto out = idx.is_monotonic_increasing();`
- Signature: `bool is_monotonic_decreasing() const`
  Example: `auto out = idx.is_monotonic_decreasing();`
- Signature: `bool is_unique() const`
  Example: `auto out = idx.is_unique();`
- Signature: `bool has_duplicates() const`
  Example: `auto out = idx.has_duplicates();`
- Signature: `Timeline min() const`
  Example: `auto out = idx.min();`
- Signature: `Timeline max() const`
  Example: `auto out = idx.max();`
- Signature: `TimeIndex sort() const`
  Example: `auto out = idx.sort();`
- Signature: `TimeIndex unique() const`
  Example: `auto out = idx.unique();`
- Signature: `TimeIndex drop_duplicates() const`
  Example: `auto out = idx.drop_duplicates();`
- Signature: `pluss::codem::Series argsort() const`
  Example: `auto out = idx.argsort();`
- Signature: `TimeIndex take(const pluss::codem::Series& indices) const`
  Example: `auto out = idx.take(indices);`
- Signature: `TimeIndex slice_index(std::int64_t start, std::int64_t stop) const`
  Example: `auto out = idx.slice_index(1, 1);`
- Signature: `TimeIndex head(std::int64_t n) const`
  Example: `auto out = idx.head(1);`
- Signature: `TimeIndex tail(std::int64_t n) const`
  Example: `auto out = idx.tail(1);`
- Signature: `TimeIndex union_(const TimeIndex& other) const`
  Example: `auto out = idx.union_(other_idx);`
- Signature: `TimeIndex intersection(const TimeIndex& other) const`
  Example: `auto out = idx.intersection(other_idx);`
- Signature: `TimeIndex difference(const TimeIndex& other) const`
  Example: `auto out = idx.difference(other_idx);`
- Signature: `TimeIndex tz_convert(std::string_view tz_name) const`
  Example: `auto out = idx.tz_convert("UTC");`
- Signature: `TimeIndex tz_localize(std::string_view tz_name, int fold, std::string_view nonexistent) const`
  Example: `auto out = idx.tz_localize("UTC", 1, "raise");`
- Signature: `TimeIndex to_utc() const`
  Example: `auto out = idx.to_utc();`
- Signature: `TimeIndex floor(std::string_view unit) const`
  Example: `auto out = idx.floor("D");`
- Signature: `TimeIndex ceil(std::string_view unit) const`
  Example: `auto out = idx.ceil("D");`
- Signature: `TimeIndex round(std::string_view unit) const`
  Example: `auto out = idx.round("D");`
- Signature: `TimeIndex normalize() const`
  Example: `auto out = idx.normalize();`
- Signature: `pluss::codem::Series resample_agg(const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step) const`
  Example: `auto out = idx.resample_agg(values, "H", "sum", 1);`
- Signature: `pluss::codem::Series resample_size(std::string_view freq, std::int64_t step) const`
  Example: `auto out = idx.resample_size("H", 1);`
- Signature: `pluss::codem::Series rolling_agg(const pluss::codem::Series& values, const Duration& window, std::string_view op) const`
  Example: `auto out = idx.rolling_agg(values, dur, "sum");`
- Signature: `pluss::codem::Series rolling_mean(const pluss::codem::Series& values, const Duration& window) const`
  Example: `auto out = idx.rolling_mean(values, dur);`
- Signature: `pluss::codem::Series rolling_min(const pluss::codem::Series& values, const Duration& window) const`
  Example: `auto out = idx.rolling_min(values, dur);`
- Signature: `pluss::codem::Series rolling_max(const pluss::codem::Series& values, const Duration& window) const`
  Example: `auto out = idx.rolling_max(values, dur);`
- Signature: `pluss::codem::Series rolling_first(const pluss::codem::Series& values, const Duration& window) const`
  Example: `auto out = idx.rolling_first(values, dur);`
- Signature: `pluss::codem::Series rolling_last(const pluss::codem::Series& values, const Duration& window) const`
  Example: `auto out = idx.rolling_last(values, dur);`
- Signature: `std::string infer_freq() const`
  Example: `auto out = idx.infer_freq();`
- Signature: `std::string freq() const`
  Example: `auto out = idx.freq();`
- Signature: `std::string frequency() const`
  Example: `auto out = idx.frequency();`
- Signature: `pluss::coden::Note to_note_iso() const`
  Example: `auto out = idx.to_note_iso();`
- Signature: `pluss::coden::Note tostring() const`
  Example: `auto out = idx.tostring();`
- Signature: `pluss::coden::Note to_string() const`
  Example: `auto out = idx.to_string();`
- Signature: `pluss::coden::Note format_iso() const`
  Example: `auto out = idx.format_iso();`
- Signature: `pluss::coden::Note format() const`
  Example: `auto out = idx.format();`
- Signature: `TimeIndex sort_values() const`
  Example: `auto out = idx.sort_values();`
- Signature: `TimeIndex sort_index() const`
  Example: `auto out = idx.sort_index();`
- Signature: `TimeIndex unionIndex(const TimeIndex& other) const`
  Example: `auto out = idx.unionIndex(other_idx);`
- Signature: `TimeIndex intersect(const TimeIndex& other) const`
  Example: `auto out = idx.intersect(other_idx);`
- Signature: `TimeIndex diffIndex(const TimeIndex& other) const`
  Example: `auto out = idx.diffIndex(other_idx);`
