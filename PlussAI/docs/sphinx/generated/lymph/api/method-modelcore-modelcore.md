# `ModelCore::ModelCore`

## API Signature

- `ModelCore(arg1: ::pluss::ai::nodej::Workflow workflow) : workflow_(std::move(workflow)) -> auto`
- `ModelCore(arg1: const ModelSpec& spec) : workflow_(resolve(::pluss::ai::nodej::make_flow(spec), "make_flow")) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`::pluss::ai::nodej::Workflow workflow) : workflow_(std::move(workflow) | const ModelSpec& spec) : workflow_(resolve(::pluss::ai::nodej::make_flow(spec), "make_flow")`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelCore(/* arg1 */);
out.info("ModelCore", value);
```

**Expected output**

```text
ModelCore returned result
```
