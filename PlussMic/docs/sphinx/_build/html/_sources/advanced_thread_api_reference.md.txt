# Advanced Thread, UDF, Text, and Delegation API Reference

Outputs on this page are exercised by `plussmic_table_statistics_window_tests`. Built-in numeric work stays in Series kernels; scalar and case callbacks are explicit UDF boundaries.

Full Unicode normalization and case conversion use the Unicode backend discovered while building the SDK. A build without that backend still handles ASCII exactly and fails explicitly for non-ASCII input.

## `Thread::map_elements`

### API Signature

```text
Thread::map_elements(fn: function<ThreadValue(const ThreadValue&)>) -> Thread
```

### Notes

Calls a scalar callback for every value and preserves one-dimensional ordering.

### Inputs

`fn: function<ThreadValue(const ThreadValue&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = numbers.map_elements(double_value);
```

#### Expected output

```text
[2.0, 4.0, 6.0]
```

Example 2.

```cpp
auto out = nullable_numbers.map_elements(double_value);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_numbers.map_elements(double_value);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = numbers.map_elements(double_value).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = numbers.map_elements(double_value);
auto second = numbers.map_elements(double_value);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::aggregate_with`

### API Signature

```text
Thread::aggregate_with(fn: function<Thread(const Thread&)>, shape: ResultShape = Reduce) -> Thread
```

### Notes

Runs an arbitrary Thread reduction or shape-controlled transformation.

### Inputs

`fn: function<Thread(const Thread&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

`shape: ResultShape = Reduce`
    Public value controlling `shape`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = numbers.aggregate_with(total_value);
```

#### Expected output

```text
[6.0]
```

Example 2.

```cpp
auto out = nullable_numbers.aggregate_with(total_value);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_numbers.aggregate_with(total_value);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = numbers.aggregate_with(total_value).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = numbers.aggregate_with(total_value);
auto second = numbers.aggregate_with(total_value);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::flow`

### API Signature

```text
Thread::flow(fn: function<Thread(const Thread&)>) -> Thread
```

### Notes

Passes the complete Thread through a chainable callback.

### Inputs

`fn: function<Thread(const Thread&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = numbers.flow(center_values);
```

#### Expected output

```text
[-1.0, 0.0, 1.0]
```

Example 2.

```cpp
auto out = nullable_numbers.flow(center_values);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_numbers.flow(center_values);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = numbers.flow(center_values).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = numbers.flow(center_values);
auto second = numbers.flow(center_values);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::map_cases`

### API Signature

```text
DataFrame::map_cases(fn: function<ThreadValue(const DataFrame&)>) -> Thread
```

### Notes

Calls a scalar callback once per case and returns one ordered Thread.

### Inputs

`fn: function<ThreadValue(const DataFrame&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.map_cases(score_case);
```

#### Expected output

```text
[10.0, 20.0, 30.0]
```

Example 2.

```cpp
auto out = nullable_df.map_cases(score_case);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.map_cases(score_case);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.map_cases(score_case).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.map_cases(score_case);
auto second = df.map_cases(score_case);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::apply_cases`

### API Signature

```text
DataFrame::apply_cases(fn: function<DataFrame(const DataFrame&)>, shape: ResultShape = Infer) -> DataFrame
```

### Notes

Applies a frame-returning callback per case with explicit inferred, reduced, preserved, or expanded shape checks.

### Inputs

`fn: function<DataFrame(const DataFrame&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

`shape: ResultShape = Infer`
    Public value controlling `shape`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.apply_cases(identity_case, ResultShape::Preserve);
```

#### Expected output

```text
the original three-case frame
```

Example 2.

```cpp
auto out = nullable_df.apply_cases(identity_case, ResultShape::Preserve);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.apply_cases(identity_case, ResultShape::Preserve);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.apply_cases(identity_case, ResultShape::Preserve).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.apply_cases(identity_case, ResultShape::Preserve);
auto second = df.apply_cases(identity_case, ResultShape::Preserve);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::map_elements`

### API Signature

```text
DataFrame::map_elements(fn: function<ThreadValue(const ThreadValue&)>, features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates element mapping to every selected feature Thread.

### Inputs

`fn: function<ThreadValue(const ThreadValue&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = numeric.map_elements(double_value, {"x", "y"});
```

#### Expected output

```text
every selected numeric cell is doubled
```

Example 2.

```cpp
auto out = nullable_numeric.map_elements(double_value, {"x", "y"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_numeric.map_elements(double_value, {"x", "y"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = numeric.map_elements(double_value, {"x", "y"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = numeric.map_elements(double_value, {"x", "y"});
auto second = numeric.map_elements(double_value, {"x", "y"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::aggregate_with`

### API Signature

```text
DataFrame::aggregate_with(fn: function<Thread(const DataFrame&)>, shape: ResultShape = Reduce) -> Thread
```

### Notes

Runs an arbitrary whole-frame callback and validates the requested result shape.

### Inputs

`fn: function<Thread(const DataFrame&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

`shape: ResultShape = Reduce`
    Public value controlling `shape`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.aggregate_with(total_x);
```

#### Expected output

```text
[6.0]
```

Example 2.

```cpp
auto out = nullable_df.aggregate_with(total_x);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.aggregate_with(total_x);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.aggregate_with(total_x).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.aggregate_with(total_x);
auto second = df.aggregate_with(total_x);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::flow`

### API Signature

```text
DataFrame::flow(fn: function<DataFrame(const DataFrame&)>) -> DataFrame
```

### Notes

Passes the complete frame through a chainable callback.

### Inputs

`fn: function<DataFrame(const DataFrame&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.flow(clean_labels);
```

#### Expected output

```text
label = [A, BB, CCC]
```

Example 2.

```cpp
auto out = nullable_df.flow(clean_labels);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.flow(clean_labels);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.flow(clean_labels).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.flow(clean_labels);
auto second = df.flow(clean_labels);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Atlas::map_elements`

### API Signature

```text
Atlas::map_elements(fn: function<ThreadValue(const ThreadValue&)>) -> Atlas
```

### Notes

Delegates element mapping through DataFrame and Thread for every depth frame.

### Inputs

`fn: function<ThreadValue(const ThreadValue&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

### Output

`Atlas` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = atlas.map_elements(double_value);
```

#### Expected output

```text
every compatible cell in both frames is doubled
```

Example 2.

```cpp
auto out = nullable_atlas.map_elements(double_value);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_atlas.map_elements(double_value);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = atlas.map_elements(double_value).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = atlas.map_elements(double_value);
auto second = atlas.map_elements(double_value);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `AtlasTsAccessor::rolling`

### API Signature

```text
AtlasTsAccessor::rolling(window: int64, statistic: WindowStatistic = Mean, options: WindowOptions = {}) -> Atlas
```

### Notes

Delegates the selected rolling statistic through `atlas.ts(...)` to every DataFrame at depth.

### Inputs

`window: int64`
    Public value controlling `window`. Invalid values fail through the Pluss error system.

`statistic: WindowStatistic = Mean`
    Public value controlling `statistic`. Invalid values fail through the Pluss error system.

`options: WindowOptions = {}`
    Public value controlling `options`. Invalid values fail through the Pluss error system.

### Output

`Atlas` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = atlas.ts({"x"}).rolling(2, WindowStatistic::Sum);
```

#### Expected output

```text
each frame x = [1.0, 3.0, 5.0]
```

Example 2.

```cpp
auto out = nullable_atlas.ts({"x"}).rolling(2, WindowStatistic::Sum);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_atlas.ts({"x"}).rolling(2, WindowStatistic::Sum);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = atlas.ts({"x"}).rolling(2, WindowStatistic::Sum).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = atlas.ts({"x"}).rolling(2, WindowStatistic::Sum);
auto second = atlas.ts({"x"}).rolling(2, WindowStatistic::Sum);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Atlas::flow`

### API Signature

```text
Atlas::flow(fn: function<Atlas(const Atlas&)>) -> Atlas
```

### Notes

Passes the complete Atlas through a chainable callback.

### Inputs

`fn: function<Atlas(const Atlas&)>`
    Public value controlling `fn`. Invalid values fail through the Pluss error system.

### Output

`Atlas` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = atlas.flow(clean_each_frame);
```

#### Expected output

```text
labels are uppercased in every frame
```

Example 2.

```cpp
auto out = nullable_atlas.flow(clean_each_frame);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_atlas.flow(clean_each_frame);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = atlas.flow(clean_each_frame).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = atlas.flow(clean_each_frame);
auto second = atlas.flow(clean_each_frame);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::upper`

### API Signature

```text
Thread::upper() -> Thread
```

### Notes

Converts every observed text value to Unicode uppercase.

### Inputs

`None`

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.upper();
```

#### Expected output

```text
[ALPHA-10, BETA-20, RED|BLUE]
```

Example 2.

```cpp
auto out = nullable_text.upper();
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.upper();
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.upper().flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.upper();
auto second = text.upper();
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::casefold`

### API Signature

```text
Thread::casefold() -> Thread
```

### Notes

Applies Unicode case folding for caseless matching.

### Inputs

`None`

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = unicode.casefold();
```

#### Expected output

```text
[café, strasse]
```

Example 2.

```cpp
auto out = nullable_unicode.casefold();
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_unicode.casefold();
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = unicode.casefold().flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = unicode.casefold();
auto second = unicode.casefold();
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::title_case`

### API Signature

```text
Thread::title_case() -> Thread
```

### Notes

Converts observed text to title case.

### Inputs

`None`

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = words.title_case();
```

#### Expected output

```text
[North America, West Africa]
```

Example 2.

```cpp
auto out = nullable_words.title_case();
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_words.title_case();
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = words.title_case().flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = words.title_case();
auto second = words.title_case();
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::text_length`

### API Signature

```text
Thread::text_length() -> Thread
```

### Notes

Counts Unicode code points in every observed text value.

### Inputs

`None`

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.text_length();
```

#### Expected output

```text
[8, 7, 8]
```

Example 2.

```cpp
auto out = nullable_text.text_length();
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.text_length();
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.text_length().flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.text_length();
auto second = text.text_length();
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::starts_with`

### API Signature

```text
Thread::starts_with(prefix: string_view) -> Thread
```

### Notes

Tests whether each observed text value begins with a prefix.

### Inputs

`prefix: string_view`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.starts_with("alpha");
```

#### Expected output

```text
[true, false, false]
```

Example 2.

```cpp
auto out = nullable_text.starts_with("alpha");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.starts_with("alpha");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.starts_with("alpha").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.starts_with("alpha");
auto second = text.starts_with("alpha");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::ends_with`

### API Signature

```text
Thread::ends_with(suffix: string_view) -> Thread
```

### Notes

Tests whether each observed text value ends with a suffix.

### Inputs

`suffix: string_view`
    Public value controlling `suffix`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.ends_with("20");
```

#### Expected output

```text
[false, true, false]
```

Example 2.

```cpp
auto out = nullable_text.ends_with("20");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.ends_with("20");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.ends_with("20").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.ends_with("20");
auto second = text.ends_with("20");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::regex_match`

### API Signature

```text
Thread::regex_match(pattern: string_view) -> Thread
```

### Notes

Tests an ECMAScript regular expression at the beginning of each text value.

### Inputs

`pattern: string_view`
    Public value controlling `pattern`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.regex_match("[a-z]+");
```

#### Expected output

```text
[true, false, true]
```

Example 2.

```cpp
auto out = nullable_text.regex_match("[a-z]+");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.regex_match("[a-z]+");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.regex_match("[a-z]+").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.regex_match("[a-z]+");
auto second = text.regex_match("[a-z]+");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::regex_fullmatch`

### API Signature

```text
Thread::regex_fullmatch(pattern: string_view) -> Thread
```

### Notes

Tests whether an ECMAScript regular expression consumes each complete text value.

### Inputs

`pattern: string_view`
    Public value controlling `pattern`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.regex_fullmatch("[A-Za-z]+-[0-9]+");
```

#### Expected output

```text
[true, true, false]
```

Example 2.

```cpp
auto out = nullable_text.regex_fullmatch("[A-Za-z]+-[0-9]+");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.regex_fullmatch("[A-Za-z]+-[0-9]+");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.regex_fullmatch("[A-Za-z]+-[0-9]+").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.regex_fullmatch("[A-Za-z]+-[0-9]+");
auto second = text.regex_fullmatch("[A-Za-z]+-[0-9]+");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::regex_extract`

### API Signature

```text
Thread::regex_extract(pattern: string_view, names: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Extracts every ECMAScript capture group into a named feature.

### Inputs

`pattern: string_view`
    Public value controlling `pattern`. Invalid values fail through the Pluss error system.

`names: RangeView<string_view> = {}`
    Public value controlling `names`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.regex_extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
```

#### Expected output

```text
word = [alpha, Beta, NA]; number = [10, 20, NA]
```

Example 2.

```cpp
auto out = nullable_text.regex_extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.regex_extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.regex_extract("([A-Za-z]+)-([0-9]+)", {"word", "number"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.regex_extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
auto second = text.regex_extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::split_text`

### API Signature

```text
Thread::split_text(delimiter: string_view, limit: int64 = -1, prefix: string = "part") -> DataFrame
```

### Notes

Splits each text value into a rectangular set of nullable output features.

### Inputs

`delimiter: string_view`
    Public value controlling `delimiter`. Invalid values fail through the Pluss error system.

`limit: int64 = -1`
    Public value controlling `limit`. Invalid values fail through the Pluss error system.

`prefix: string = "part"`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.split_text("-");
```

#### Expected output

```text
part_0 = [alpha, Beta, red|blue]; part_1 = [10, 20, NA]
```

Example 2.

```cpp
auto out = nullable_text.split_text("-");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.split_text("-");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.split_text("-").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.split_text("-");
auto second = text.split_text("-");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::partition_text`

### API Signature

```text
Thread::partition_text(separator: string_view, prefix: string = "part") -> DataFrame
```

### Notes

Partitions each value into before, separator, and after features.

### Inputs

`separator: string_view`
    Public value controlling `separator`. Invalid values fail through the Pluss error system.

`prefix: string = "part"`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.partition_text("-");
```

#### Expected output

```text
the first row is [alpha, -, 10]
```

Example 2.

```cpp
auto out = nullable_text.partition_text("-");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.partition_text("-");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.partition_text("-").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.partition_text("-");
auto second = text.partition_text("-");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::pad_text`

### API Signature

```text
Thread::pad_text(width: int64, align: TextAlign = Left, fill: char = ' ') -> Thread
```

### Notes

Pads text to a minimum character width with typed alignment.

### Inputs

`width: int64`
    Public value controlling `width`. Invalid values fail through the Pluss error system.

`align: TextAlign = Left`
    Public value controlling `align`. Invalid values fail through the Pluss error system.

`fill: char = ' '`
    Public value controlling `fill`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.pad_text(10, TextAlign::Right, '.');
```

#### Expected output

```text
[..alpha-10, ...Beta-20, ..red|blue]
```

Example 2.

```cpp
auto out = nullable_text.pad_text(10, TextAlign::Right, '.');
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.pad_text(10, TextAlign::Right, '.');
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.pad_text(10, TextAlign::Right, '.').flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.pad_text(10, TextAlign::Right, '.');
auto second = text.pad_text(10, TextAlign::Right, '.');
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::slice_text`

### API Signature

```text
Thread::slice_text(start: int64, stop: optional<int64> = nullopt, step: int64 = 1) -> Thread
```

### Notes

Slices each value by Unicode character position.

### Inputs

`start: int64`
    Public value controlling `start`. Invalid values fail through the Pluss error system.

`stop: optional<int64> = nullopt`
    Public value controlling `stop`. Invalid values fail through the Pluss error system.

`step: int64 = 1`
    Public value controlling `step`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.slice_text(0, 5);
```

#### Expected output

```text
[alpha, Beta-, red|b]
```

Example 2.

```cpp
auto out = nullable_text.slice_text(0, 5);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.slice_text(0, 5);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.slice_text(0, 5).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.slice_text(0, 5);
auto second = text.slice_text(0, 5);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::normalize_text`

### API Signature

```text
Thread::normalize_text(form: UnicodeForm = NFC) -> Thread
```

### Notes

Applies canonical or compatibility Unicode normalization.

### Inputs

`form: UnicodeForm = NFC`
    Public value controlling `form`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = unicode.normalize_text(UnicodeForm::NFC);
```

#### Expected output

```text
[Café, Straße]
```

Example 2.

```cpp
auto out = nullable_unicode.normalize_text(UnicodeForm::NFC);
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_unicode.normalize_text(UnicodeForm::NFC);
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = unicode.normalize_text(UnicodeForm::NFC).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = unicode.normalize_text(UnicodeForm::NFC);
auto second = unicode.normalize_text(UnicodeForm::NFC);
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::encode_text`

### API Signature

```text
Thread::encode_text(encoding: string_view = "base64") -> Thread
```

### Notes

Encodes each value as base64, hexadecimal, or URL text.

### Inputs

`encoding: string_view = "base64"`
    Public value controlling `encoding`. Invalid values fail through the Pluss error system.

### Output

`Thread` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.encode_text("hex");
```

#### Expected output

```text
[616c7068612d3130, 426574612d3230, 7265647c626c7565]
```

Example 2.

```cpp
auto out = nullable_text.encode_text("hex");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.encode_text("hex");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.encode_text("hex").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.encode_text("hex");
auto second = text.encode_text("hex");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `Thread::text_indicators`

### API Signature

```text
Thread::text_indicators(delimiter: string_view = "|", prefix: string = "") -> DataFrame
```

### Notes

Creates one boolean feature per distinct delimited token.

### Inputs

`delimiter: string_view = "|"`
    Public value controlling `delimiter`. Invalid values fail through the Pluss error system.

`prefix: string = ""`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = text.text_indicators("|");
```

#### Expected output

```text
red and blue are true for the third case
```

Example 2.

```cpp
auto out = nullable_text.text_indicators("|");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_text.text_indicators("|");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = text.text_indicators("|").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = text.text_indicators("|");
auto second = text.text_indicators("|");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_upper`

### API Signature

```text
DataFrame::text_upper(features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates Unicode uppercase conversion to selected text Threads.

### Inputs

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_upper({"label"});
```

#### Expected output

```text
label = [A, BB, CCC]
```

Example 2.

```cpp
auto out = nullable_df.text_upper({"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_upper({"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_upper({"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_upper({"label"});
auto second = df.text_upper({"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_casefold`

### API Signature

```text
DataFrame::text_casefold(features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates Unicode case folding to selected text Threads.

### Inputs

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_casefold({"label"});
```

#### Expected output

```text
label contains caseless normalized forms
```

Example 2.

```cpp
auto out = nullable_df.text_casefold({"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_casefold({"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_casefold({"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_casefold({"label"});
auto second = df.text_casefold({"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_title`

### API Signature

```text
DataFrame::text_title(features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates title casing to selected text Threads.

### Inputs

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_title({"label"});
```

#### Expected output

```text
selected labels use title case
```

Example 2.

```cpp
auto out = nullable_df.text_title({"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_title({"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_title({"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_title({"label"});
auto second = df.text_title({"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_lengths`

### API Signature

```text
DataFrame::text_lengths(features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Replaces selected text features with Unicode character counts.

### Inputs

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_lengths({"label"});
```

#### Expected output

```text
label = [1, 2, 3]
```

Example 2.

```cpp
auto out = nullable_df.text_lengths({"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_lengths({"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_lengths({"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_lengths({"label"});
auto second = df.text_lengths({"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_starts_with`

### API Signature

```text
DataFrame::text_starts_with(prefix: string_view, features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates prefix testing to selected text Threads.

### Inputs

`prefix: string_view`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_starts_with("a", {"label"});
```

#### Expected output

```text
label = [true, false, false]
```

Example 2.

```cpp
auto out = nullable_df.text_starts_with("a", {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_starts_with("a", {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_starts_with("a", {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_starts_with("a", {"label"});
auto second = df.text_starts_with("a", {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_ends_with`

### API Signature

```text
DataFrame::text_ends_with(suffix: string_view, features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates suffix testing to selected text Threads.

### Inputs

`suffix: string_view`
    Public value controlling `suffix`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_ends_with("c", {"label"});
```

#### Expected output

```text
label = [false, false, true]
```

Example 2.

```cpp
auto out = nullable_df.text_ends_with("c", {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_ends_with("c", {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_ends_with("c", {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_ends_with("c", {"label"});
auto second = df.text_ends_with("c", {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_matches`

### API Signature

```text
DataFrame::text_matches(pattern: string_view, full: bool = false, features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates prefix or full regular-expression matching to selected text Threads.

### Inputs

`pattern: string_view`
    Public value controlling `pattern`. Invalid values fail through the Pluss error system.

`full: bool = false`
    Public value controlling `full`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_matches("[a-z]+", true, {"label"});
```

#### Expected output

```text
each selected cell reports a full-match boolean
```

Example 2.

```cpp
auto out = nullable_df.text_matches("[a-z]+", true, {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_matches("[a-z]+", true, {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_matches("[a-z]+", true, {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_matches("[a-z]+", true, {"label"});
auto second = df.text_matches("[a-z]+", true, {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_pad`

### API Signature

```text
DataFrame::text_pad(width: int64, align: TextAlign = Left, fill: char = ' ', features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates text padding to selected text Threads.

### Inputs

`width: int64`
    Public value controlling `width`. Invalid values fail through the Pluss error system.

`align: TextAlign = Left`
    Public value controlling `align`. Invalid values fail through the Pluss error system.

`fill: char = ' '`
    Public value controlling `fill`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_pad(4, TextAlign::Right, '.', {"label"});
```

#### Expected output

```text
label = [...a, ..bb, .ccc]
```

Example 2.

```cpp
auto out = nullable_df.text_pad(4, TextAlign::Right, '.', {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_pad(4, TextAlign::Right, '.', {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_pad(4, TextAlign::Right, '.', {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_pad(4, TextAlign::Right, '.', {"label"});
auto second = df.text_pad(4, TextAlign::Right, '.', {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_slice`

### API Signature

```text
DataFrame::text_slice(start: int64, stop: optional<int64> = nullopt, step: int64 = 1, features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates Unicode character slicing to selected text Threads.

### Inputs

`start: int64`
    Public value controlling `start`. Invalid values fail through the Pluss error system.

`stop: optional<int64> = nullopt`
    Public value controlling `stop`. Invalid values fail through the Pluss error system.

`step: int64 = 1`
    Public value controlling `step`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_slice(0, 1, 1, {"label"});
```

#### Expected output

```text
label = [a, b, c]
```

Example 2.

```cpp
auto out = nullable_df.text_slice(0, 1, 1, {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_slice(0, 1, 1, {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_slice(0, 1, 1, {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_slice(0, 1, 1, {"label"});
auto second = df.text_slice(0, 1, 1, {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_normalize`

### API Signature

```text
DataFrame::text_normalize(form: UnicodeForm = NFC, features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates Unicode normalization to selected text Threads.

### Inputs

`form: UnicodeForm = NFC`
    Public value controlling `form`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_normalize(UnicodeForm::NFC, {"label"});
```

#### Expected output

```text
canonically equivalent labels use NFC encoding
```

Example 2.

```cpp
auto out = nullable_df.text_normalize(UnicodeForm::NFC, {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_normalize(UnicodeForm::NFC, {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_normalize(UnicodeForm::NFC, {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_normalize(UnicodeForm::NFC, {"label"});
auto second = df.text_normalize(UnicodeForm::NFC, {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_encode`

### API Signature

```text
DataFrame::text_encode(encoding: string_view = "base64", features: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Delegates text encoding to selected text Threads.

### Inputs

`encoding: string_view = "base64"`
    Public value controlling `encoding`. Invalid values fail through the Pluss error system.

`features: RangeView<string_view> = {}`
    Public value controlling `features`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = df.text_encode("hex", {"label"});
```

#### Expected output

```text
label = [61, 6262, 636363]
```

Example 2.

```cpp
auto out = nullable_df.text_encode("hex", {"label"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_df.text_encode("hex", {"label"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = df.text_encode("hex", {"label"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = df.text_encode("hex", {"label"});
auto second = df.text_encode("hex", {"label"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::regex_groups`

### API Signature

```text
DataFrame::regex_groups(feature: string_view, pattern: string_view, names: RangeView<string_view> = {}) -> DataFrame
```

### Notes

Expands capture groups from one text feature while preserving cases.

### Inputs

`feature: string_view`
    Public value controlling `feature`. Invalid values fail through the Pluss error system.

`pattern: string_view`
    Public value controlling `pattern`. Invalid values fail through the Pluss error system.

`names: RangeView<string_view> = {}`
    Public value controlling `names`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = people.regex_groups("code", "([A-Z]+)-([0-9]+)", {"site", "id"});
```

#### Expected output

```text
site and id contain the two capture groups
```

Example 2.

```cpp
auto out = nullable_people.regex_groups("code", "([A-Z]+)-([0-9]+)", {"site", "id"});
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_people.regex_groups("code", "([A-Z]+)-([0-9]+)", {"site", "id"});
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = people.regex_groups("code", "([A-Z]+)-([0-9]+)", {"site", "id"}).flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = people.regex_groups("code", "([A-Z]+)-([0-9]+)", {"site", "id"});
auto second = people.regex_groups("code", "([A-Z]+)-([0-9]+)", {"site", "id"});
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::split_feature`

### API Signature

```text
DataFrame::split_feature(feature: string_view, delimiter: string_view, limit: int64 = -1, prefix: string = "part") -> DataFrame
```

### Notes

Expands one delimited text feature while preserving cases.

### Inputs

`feature: string_view`
    Public value controlling `feature`. Invalid values fail through the Pluss error system.

`delimiter: string_view`
    Public value controlling `delimiter`. Invalid values fail through the Pluss error system.

`limit: int64 = -1`
    Public value controlling `limit`. Invalid values fail through the Pluss error system.

`prefix: string = "part"`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = people.split_feature("code", "-");
```

#### Expected output

```text
part_0 and part_1 contain the split code
```

Example 2.

```cpp
auto out = nullable_people.split_feature("code", "-");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_people.split_feature("code", "-");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = people.split_feature("code", "-").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = people.split_feature("code", "-");
auto second = people.split_feature("code", "-");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::partition_feature`

### API Signature

```text
DataFrame::partition_feature(feature: string_view, separator: string_view, prefix: string = "part") -> DataFrame
```

### Notes

Expands one text feature into before, separator, and after features.

### Inputs

`feature: string_view`
    Public value controlling `feature`. Invalid values fail through the Pluss error system.

`separator: string_view`
    Public value controlling `separator`. Invalid values fail through the Pluss error system.

`prefix: string = "part"`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = people.partition_feature("code", "-");
```

#### Expected output

```text
three partition features preserve the source cases
```

Example 2.

```cpp
auto out = nullable_people.partition_feature("code", "-");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_people.partition_feature("code", "-");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = people.partition_feature("code", "-").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = people.partition_feature("code", "-");
auto second = people.partition_feature("code", "-");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```

## `DataFrame::text_indicators`

### API Signature

```text
DataFrame::text_indicators(feature: string_view, delimiter: string_view = "|", prefix: string = "") -> DataFrame
```

### Notes

Expands delimited categories into boolean indicator features.

### Inputs

`feature: string_view`
    Public value controlling `feature`. Invalid values fail through the Pluss error system.

`delimiter: string_view = "|"`
    Public value controlling `delimiter`. Invalid values fail through the Pluss error system.

`prefix: string = ""`
    Public value controlling `prefix`. Invalid values fail through the Pluss error system.

### Output

`DataFrame` with the shape and validity described above.

### Options

`ResultShape` supports `Infer`, `Reduce`, `Preserve`, and `Expand`; `TextAlign` supports `Left`, `Right`, and `Center`; `UnicodeForm` supports `NFC`, `NFD`, `NFKC`, and `NFKD`. Options not present in this signature do not apply.

### Examples

Example 1.

```cpp
auto out = people.text_indicators("tags", "|");
```

#### Expected output

```text
one boolean feature is returned for every observed tag
```

Example 2.

```cpp
auto out = nullable_people.text_indicators("tags", "|");
```

#### Expected output

```text
null input positions remain null; observed positions follow the same contract
```

Example 3.

```cpp
auto out = one_people.text_indicators("tags", "|");
```

#### Expected output

```text
one input value or case produces one contract-compatible result
```

Example 4.

```cpp
auto out = people.text_indicators("tags", "|").flow([](const auto& value) { return value; });
```

#### Expected output

```text
identity flow preserves the preceding result
```

Example 5.

```cpp
auto first = people.text_indicators("tags", "|");
auto second = people.text_indicators("tags", "|");
```

#### Expected output

```text
repeated calls are deterministic and produce identical values and validity
```
