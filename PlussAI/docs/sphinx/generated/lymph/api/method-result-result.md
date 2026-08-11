# `Result::Result`

## API Signature

- `model_(arg1: std::move(model)), arg2: coordinates_(std::move(coordinates)) -> Result(Model model, Series coordinates) :`
- `model_(arg1: std::move(model)), arg2: labels_(std::move(labels)) -> Result(Model model, Series labels) :`
- `model_(arg1: std::move(model)), arg2: labels_(std::move(labels)), arg3: labeled_round_(std::move(labeled_round)), arg4: rounds_(rounds), stop: stop_() -> Result(Model model, Series labels, Series labeled_round, std::int64_t rounds, StopReason stop) :`
- `method_(arg1: std::move(method)), arg2: labels_(std::move(labels)), arg3: scores_(std::move(scores)), arg4: threshold_(threshold), arg5: model_(std::move(model)) -> Result(std::string method, Series labels, Series scores, double threshold, std::optional<Model> model = std::nullopt) :`
- `feature_names_(arg1: std::move(feature_names)), arg2: sample_count_(sample_count), arg3: mean_(std::move(mean)), arg4: stddev_(std::move(stddev)), arg5: minimum_(std::move(minimum)), arg6: maximum_(std::move(maximum)), arg7: skewness_(std::move(skewness)), arg8: excess_kurtosis_(std::move(excess_kurtosis)), arg9: missing_rate_(std::move(missing_rate)), arg10: negative_rate_(std::move(negative_rate)), arg11: zero_rate_(std::move(zero_rate)), arg12: gaussianize_candidate_(std::move(gaussianize_candidate)) -> Result(std::vector<std::string> feature_names, std::int64_t sample_count, Series mean, Series stddev, Series minimum, Series maximum, Series skewness, Series excess_kurtosis, Series missing_rate, Series negative_rate, Series zero_rate, Series gaussianize_candidate) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`std::move(feature_names)) | std::move(method)) | std::move(model))`)
  Method argument; see signature type for accepted form.
- `arg10` (`negative_rate_(std::move(negative_rate))`)
  Method argument; see signature type for accepted form.
- `arg11` (`zero_rate_(std::move(zero_rate))`)
  Method argument; see signature type for accepted form.
- `arg12` (`gaussianize_candidate_(std::move(gaussianize_candidate)`)
  Method argument; see signature type for accepted form.
- `arg2` (`coordinates_(std::move(coordinates) | labels_(std::move(labels) | labels_(std::move(labels)) | sample_count_(sample_count)`)
  Method argument; see signature type for accepted form.
- `arg3` (`labeled_round_(std::move(labeled_round)) | mean_(std::move(mean)) | scores_(std::move(scores))`)
  Method argument; see signature type for accepted form.
- `arg4` (`rounds_(rounds) | stddev_(std::move(stddev)) | threshold_(threshold)`)
  Method argument; see signature type for accepted form.
- `arg5` (`minimum_(std::move(minimum)) | model_(std::move(model)`)
  Method argument; see signature type for accepted form.
- `arg6` (`maximum_(std::move(maximum))`)
  Method argument; see signature type for accepted form.
- `arg7` (`skewness_(std::move(skewness))`)
  Method argument; see signature type for accepted form.
- `arg8` (`excess_kurtosis_(std::move(excess_kurtosis))`)
  Method argument; see signature type for accepted form.
- `arg9` (`missing_rate_(std::move(missing_rate))`)
  Method argument; see signature type for accepted form.
- `stop` (`stop_(`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Result(Model model, Series coordinates) :`.
- Returns `Result(Model model, Series labels) :`.
- Returns `Result(Model model, Series labels, Series labeled_round, std::int64_t rounds, StopReason stop) :`.
- Returns `Result(std::string method, Series labels, Series scores, double threshold, std::optional<Model> model = std::nullopt) :`.
- Returns `Result(std::vector<std::string> feature_names, std::int64_t sample_count, Series mean, Series stddev, Series minimum, Series maximum, Series skewness, Series excess_kurtosis, Series missing_rate, Series negative_rate, Series zero_rate, Series gaussianize_candidate) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = model_(/* arg1 */, /* arg2 */);
out.info("Result", value);
```

**Expected output**

```text
Result returned Result(Model model, Series coordinates) :
```
