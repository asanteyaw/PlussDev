# `FeatureTransformer::set_remainder_policy`

## API Signature

- `FeatureTransformer::set_remainder_policy(policy: FeatureRemainderPolicy) -> void`

## Notes

Sets or updates internal configuration/state used by later train/infer/evaluate calls.

## Inputs

- `policy` (`FeatureRemainderPolicy`)
  Target values aligned to input observations.

## Output

- No direct return value; operation applies side effects to object state.

## Options

Option semantics used by this method:

- `FeatureRemainderPolicy` (`enum`):
  - `Drop`: Accepted enum choice for this method.
  - `PassThrough`: Accepted enum choice for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
FeatureTransformer::set_remainder_policy(/* policy */);
out.info("set_remainder_policy completed");
```

**Expected output**

```text
set_remainder_policy completed
```
