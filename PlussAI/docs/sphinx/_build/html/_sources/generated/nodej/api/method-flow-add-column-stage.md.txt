# `Flow::add_column_stage`

## API Signature

- `Flow::add_column_stage(stage_key: std::string, kind: TransformKind, options: const TransformOptions&, feature_route: std::initializer_list<std::int64_t>, remainder: RouteRemainderPolicy = RouteRemainderPolicy::PassThrough) -> Status`

## Notes

Instance method on `self`.

## Inputs

- `feature_route` (`std::initializer_list<std::int64_t>`)
  Input feature data used by this call.
- `kind` (`TransformKind`)
  Algorithm-size/count argument used to shape fit or split behavior.
- `options` (`const TransformOptions&`)
  Configuration object controlling algorithm behavior and safety defaults.
- `remainder` (`RouteRemainderPolicy`), default: `RouteRemainderPolicy::PassThrough`
  Method argument; see signature type for accepted form.
- `stage_key` (`std::string`)
  Target values aligned to input observations.

## Output

- Returns a status object indicating success/failure and an error message on failure.

## Options

Option semantics used by this method:

- `RouteRemainderPolicy` (`enum`):
  - `PassThrough`: Accepted enum choice for this method.
  - `Drop`: Accepted enum choice for this method.

- `TransformKind` (`enum`):
  - `StandardScaler`: Accepted enum choice for this method.
  - `MinMaxScaler`: Accepted enum choice for this method.
  - `RobustScaler`: Accepted enum choice for this method.
  - `SimpleImputer`: Accepted enum choice for this method.
  - `NeighborImputer`: Accepted enum choice for this method.
  - `IterativeImputer`: Accepted enum choice for this method.
  - `OneHotEncoder`: Accepted enum choice for this method.
  - `FrequencyEncoder`: Accepted enum choice for this method.
  - `VectorNormalizer`: Accepted enum choice for this method.
  - `MaxAbsScaler`: Accepted enum choice for this method.
  - `QuantileMap`: Accepted enum choice for this method.
  - `PowerMap`: Accepted enum choice for this method.
  - `ThresholdBinarizer`: Accepted enum choice for this method.
  - `PolynomialLift`: Accepted enum choice for this method.
  - `OrdinalEncoder`: Accepted enum choice for this method.
  - `LabelEncoder`: Accepted enum choice for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto st = Flow::add_column_stage(/* stage_key */, /* kind */, /* options */, /* feature_route */, /* remainder */);
if (!st.ok()) {
  out.error("add_column_stage failed", st.message);
} else {
  out.info("add_column_stage ok");
}
```

**Expected output**

```text
add_column_stage: ok
status.ok() == true
```
