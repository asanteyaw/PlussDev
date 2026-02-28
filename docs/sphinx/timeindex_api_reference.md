# TimeIndex API Reference

# Complete TimeIndex Method Reference: Signatures + Real Calls



## API Reference Style

This section follows a professional reference layout: signatures, parameters, returns, notes, examples, and overload links.

### `align_union`

#### Signature

```cpp
AlignUnion align_union(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`AlignUnion`
    Return value produced by this method call.

#### Notes

Applies `align_union` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.align_union(other_idx);
```

### `argsort`

#### Signature

```cpp
pluss::codem::Series argsort(self) const
```

#### Parameters

`None`

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.argsort();
```

### `asof`

#### Signature

```cpp
std::int64_t asof(self, const Timeline& key) const
```

#### Parameters

`key` : `const Timeline&`
    Required method argument.

#### Returns

`std::int64_t`
    Return value produced by this method call.

#### Notes

Applies `asof` to `self`. Arguments: `const Timeline& key`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto key_t = pluss::make_timeline("2024-01-01T00:20:00Z");

auto out = idx.asof(key_t);
```

### `ceil`

#### Signature

```cpp
TimeIndex ceil(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `ceil` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.ceil("D");
```

### `difference`

#### Signature

```cpp
TimeIndex difference(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `difference` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.difference(other_idx);
```

### `diffIndex`

#### Signature

```cpp
TimeIndex diffIndex(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `diffIndex` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.diffIndex(other_idx);
```

### `drop_duplicates`

#### Signature

```cpp
TimeIndex drop_duplicates(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `drop_duplicates` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.drop_duplicates();
```

### `floor`

#### Signature

```cpp
TimeIndex floor(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `floor` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.floor("D");
```

### `format`

#### Signature

```cpp
pluss::coden::Note format(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.format();
```

### `format_iso`

#### Signature

```cpp
pluss::coden::Note format_iso(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.format_iso();
```

### `freq`

#### Signature

```cpp
std::string freq(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `freq` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.freq();
```

### `frequency`

#### Signature

```cpp
std::string frequency(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `frequency` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.frequency();
```

### `has_duplicates`

#### Signature

```cpp
bool has_duplicates(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.has_duplicates();
```

### `head`

#### Signature

```cpp
TimeIndex head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `head` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.head(1);
```

### `infer_freq`

#### Signature

```cpp
std::string infer_freq(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Return value produced by this method call.

#### Notes

Applies `infer_freq` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.infer_freq();
```

### `intersect`

#### Signature

```cpp
TimeIndex intersect(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `intersect` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.intersect(other_idx);
```

### `intersection`

#### Signature

```cpp
TimeIndex intersection(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `intersection` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.intersection(other_idx);
```

### `is_monotonic_decreasing`

#### Signature

```cpp
bool is_monotonic_decreasing(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.is_monotonic_decreasing();
```

### `is_monotonic_increasing`

#### Signature

```cpp
bool is_monotonic_increasing(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.is_monotonic_increasing();
```

### `is_unique`

#### Signature

```cpp
bool is_unique(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Return value produced by this method call.

#### Notes

Evaluates a predicate on `self` and returns boolean/scalar mask output. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.is_unique();
```

### `max`

#### Signature

```cpp
Timeline max(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `max` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.max();
```

### `min`

#### Signature

```cpp
Timeline min(self) const
```

#### Parameters

`None`

#### Returns

`Timeline`
    Return value produced by this method call.

#### Notes

Applies `min` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.min();
```

### `normalize`

#### Signature

```cpp
TimeIndex normalize(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `normalize` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.normalize();
```

### `resample_agg`

#### Signature

```cpp
pluss::codem::Series resample_agg(self, const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`op` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::string_view op, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_agg(values, "H", "sum", 1);
```

### `resample_bins`

#### Signature

```cpp
ResampleBins resample_bins(self, std::string_view freq, std::int64_t step) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`ResampleBins`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.resample_bins("H", 1);
```

### `resample_count`

#### Signature

```cpp
pluss::codem::Series resample_count(self, std::string_view freq, std::int64_t step) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.resample_count("H", 1);
```

### `resample_first`

#### Signature

```cpp
pluss::codem::Series resample_first(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_first(values, "H", 1);
```

### `resample_last`

#### Signature

```cpp
pluss::codem::Series resample_last(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_last(values, "H", 1);
```

### `resample_max`

#### Signature

```cpp
pluss::codem::Series resample_max(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_max(values, "H", 1);
```

### `resample_mean`

#### Signature

```cpp
pluss::codem::Series resample_mean(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_mean(values, "H", 1);
```

### `resample_min`

#### Signature

```cpp
pluss::codem::Series resample_min(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_min(values, "H", 1);
```

### `resample_ohlc`

#### Signature

```cpp
pluss::codem::Series resample_ohlc(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_ohlc(values, "H", 1);
```

### `resample_size`

#### Signature

```cpp
pluss::codem::Series resample_size(self, std::string_view freq, std::int64_t step) const
```

#### Parameters

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.resample_size("H", 1);
```

### `resample_sum`

#### Signature

```cpp
pluss::codem::Series resample_sum(self, const pluss::codem::Series& values, std::string_view freq, std::int64_t step) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`freq` : `std::string_view`
    Required method argument.

`step` : `std::int64_t`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, std::string_view freq, std::int64_t step`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});

auto out = idx.resample_sum(values, "H", 1);
```

### `rolling_agg`

#### Signature

```cpp
pluss::codem::Series rolling_agg(self, const pluss::codem::Series& values, const Duration& window, std::string_view op) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

`op` : `std::string_view`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window, std::string_view op`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_agg(values, dur, "sum");
```

### `rolling_count`

#### Signature

```cpp
pluss::codem::Series rolling_count(self, const Duration& window) const
```

#### Parameters

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_count(dur);
```

### `rolling_first`

#### Signature

```cpp
pluss::codem::Series rolling_first(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_first(values, dur);
```

### `rolling_last`

#### Signature

```cpp
pluss::codem::Series rolling_last(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_last(values, dur);
```

### `rolling_max`

#### Signature

```cpp
pluss::codem::Series rolling_max(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_max(values, dur);
```

### `rolling_mean`

#### Signature

```cpp
pluss::codem::Series rolling_mean(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_mean(values, dur);
```

### `rolling_min`

#### Signature

```cpp
pluss::codem::Series rolling_min(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_min(values, dur);
```

### `rolling_sum`

#### Signature

```cpp
pluss::codem::Series rolling_sum(self, const pluss::codem::Series& values, const Duration& window) const
```

#### Parameters

`values` : `const pluss::codem::Series&`
    Required method argument.

`window` : `const Duration&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Performs time-window aggregation using the index/timeline context. Arguments: `const pluss::codem::Series& values, const Duration& window`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto values = pluss::make_series(std::vector<scalar>{10.0, 20.0, 30.0}, {3});
auto dur = pluss::duration_minutes(30);

auto out = idx.rolling_sum(values, dur);
```

### `round`

#### Signature

```cpp
TimeIndex round(self, std::string_view unit) const
```

#### Parameters

`unit` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `round` computation to `self`. Arguments: `std::string_view unit`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.round("D");
```

### `searchsorted`

#### Signature

```cpp
std::int64_t searchsorted(self, const Timeline& key, bool right) const
```

#### Parameters

`key` : `const Timeline&`
    Required method argument.

`right` : `bool`
    Required method argument.

#### Returns

`std::int64_t`
    Return value produced by this method call.

#### Notes

Applies `searchsorted` to `self`. Arguments: `const Timeline& key, bool right`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto key_t = pluss::make_timeline("2024-01-01T00:20:00Z");

auto out = idx.searchsorted(key_t, true);
```

### `slice`

#### Signature

```cpp
pluss::codem::Series slice(self, const Timeline& start, const Timeline& end) const
```

#### Parameters

`start` : `const Timeline&`
    Required method argument.

`end` : `const Timeline&`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `slice` to `self`. Arguments: `const Timeline& start, const Timeline& end`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto key_t = pluss::make_timeline("2024-01-01T00:20:00Z");

auto out = idx.slice(key_t, key_t);
```

### `slice_index`

#### Signature

```cpp
TimeIndex slice_index(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Required method argument.

`stop` : `std::int64_t`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `slice_index` to `self`. Arguments: `std::int64_t start, std::int64_t stop`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.slice_index(1, 1);
```

### `slice_time`

#### Signature

```cpp
pluss::codem::Series slice_time(self, std::string_view start_iso, std::string_view end_iso) const
```

#### Parameters

`start_iso` : `std::string_view`
    Required method argument.

`end_iso` : `std::string_view`
    Required method argument.

#### Returns

`pluss::codem::Series`
    Return value produced by this method call.

#### Notes

Applies `slice_time` to `self`. Arguments: `std::string_view start_iso, std::string_view end_iso`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.slice_time("2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z");
```

### `sort`

#### Signature

```cpp
TimeIndex sort(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.sort();
```

### `sort_index`

#### Signature

```cpp
TimeIndex sort_index(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `sort_index` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.sort_index();
```

### `sort_values`

#### Signature

```cpp
TimeIndex sort_values(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `sort_values` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.sort_values();
```

### `tail`

#### Signature

```cpp
TimeIndex tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `tail` to `self`. Arguments: `std::int64_t n`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.tail(1);
```

### `take`

#### Signature

```cpp
TimeIndex take(self, const pluss::codem::Series& indices) const
```

#### Parameters

`indices` : `const pluss::codem::Series&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Changes shape/indexing/view behavior for `self`. Arguments: `const pluss::codem::Series& indices`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto indices = pluss::make_series(std::vector<std::int64_t>{0,2}, {2});

auto out = idx.take(indices);
```

### `to_note_iso`

#### Signature

```cpp
pluss::coden::Note to_note_iso(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Formats/converts `self` to a textual representation. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.to_note_iso();
```

### `to_string`

#### Signature

```cpp
pluss::coden::Note to_string(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.to_string();
```

### `to_utc`

#### Signature

```cpp
TimeIndex to_utc(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Converts between related PlussQit data types. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.to_utc();
```

### `tostring`

#### Signature

```cpp
pluss::coden::Note tostring(self) const
```

#### Parameters

`None`

#### Returns

`pluss::coden::Note`
    Return value produced by this method call.

#### Notes

Applies `tostring` to `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.tostring();
```

### `tz_convert`

#### Signature

```cpp
TimeIndex tz_convert(self, std::string_view tz_name) const
```

#### Parameters

`tz_name` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `tz_convert` to `self`. Arguments: `std::string_view tz_name`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.tz_convert("UTC");
```

### `tz_localize`

#### Signature

```cpp
TimeIndex tz_localize(self, std::string_view tz_name, int fold, std::string_view nonexistent) const
```

#### Parameters

`tz_name` : `std::string_view`
    Required method argument.

`fold` : `int`
    Required method argument.

`nonexistent` : `std::string_view`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `tz_localize` to `self`. Arguments: `std::string_view tz_name, int fold, std::string_view nonexistent`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.tz_localize("UTC", 1, "raise");
```

### `union_`

#### Signature

```cpp
TimeIndex union_(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

In-place variant of `union`; it mutates `self` and returns `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.union_(other_idx);
```

### `unionIndex`

#### Signature

```cpp
TimeIndex unionIndex(self, const TimeIndex& other) const
```

#### Parameters

`other` : `const TimeIndex&`
    Required method argument.

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Applies `unionIndex` to `self`. Arguments: `const TimeIndex& other`.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});
auto other_idx = pluss::make_timeindex({"2024-01-01T00:05:00Z", "2024-01-01T00:35:00Z", "2024-01-01T01:05:00Z"});

auto out = idx.unionIndex(other_idx);
```

### `unique`

#### Signature

```cpp
TimeIndex unique(self) const
```

#### Parameters

`None`

#### Returns

`TimeIndex`
    Return value produced by this method call.

#### Notes

Computes a reduction or summary from `self`. No additional arguments.

#### Examples

```cpp
auto idx = pluss::make_timeindex({"2024-01-01T00:00:00Z", "2024-01-01T00:15:00Z", "2024-01-01T00:45:00Z"});

auto out = idx.unique();
```

## Demos to run

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`
- `PlussQit/examples/CodeH/timeline_printing_2d_3d_demo.cpp`