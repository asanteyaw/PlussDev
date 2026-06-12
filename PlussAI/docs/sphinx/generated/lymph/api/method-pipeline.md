# `pipeline`

## API Signature

- `pipeline(spec: const ModelSpec&, stages: std::initializer_list<TransformSpec>) -> Workflow`
- `pipeline(model: WorkflowModel, stages: std::initializer_list<TransformSpec>) -> Workflow`

## Notes

Namespace-level API function.

## Inputs

- `model` (`WorkflowModel`)
  Method argument; see signature type for accepted form.
- `spec` (`const ModelSpec&`)
  Method argument; see signature type for accepted form.
- `stages` (`std::initializer_list<TransformSpec>`)
  List-like argument; pass values in the order expected by the signature.

## Output

- Returns `Workflow`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = pipeline(/* spec */, /* stages */);
out.info("pipeline", value);
```

**Expected output**

```text
pipeline returned Workflow
```

### Example 2 (Demo Pattern)

```cpp
  Series final_pred = final_pred_or.move_value();

  std::cout << "\n========== final holdout metrics ========\n";
  std::cout << "rmse=" << (final_metrics_vec.size() > 0 ? final_metrics_vec[0] : std::numeric_limits<double>::quiet_NaN()) << "\n";
  std::cout << "r2=" << (final_metrics_vec.size() > 1 ? final_metrics_vec[1] : std::numeric_limits<double>::quiet_NaN()) << "\n";
  std::cout << "mae=" << (final_metrics_vec.size() > 2 ? final_metrics_vec[2] : std::numeric_limits<double>::quiet_NaN()) << "\n";

  //========== flow persistence with best spec ========
  // This section uses core Flow + io::save/io::load_and_restore for exact pipeline persistence.
  StatusOr<Flow> final_flow_or = make_flow(best_spec);
  if (!final_flow_or.ok()) {
    std::cerr << "make_flow(best_spec) failed: " << final_flow_or.status().message << "\n";
    return 1;
  }
  Flow final_flow = final_flow_or.move_value();

  Status flow_learn_status = final_flow.learn(train_batch, train_options);
  if (!flow_learn_status.ok()) {
    std::cerr << "final_flow.learn failed: " << flow_learn_status.message << "\n";
    return 1;
  }
```

**Expected output**

```text
pipeline returned Workflow
```
