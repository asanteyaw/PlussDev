# `SearchResult::top`

## API Signature

- `SearchResult::top(k: std::int64_t) -> std::vector<std::pair<std::string, Model>>`

## Notes

Instance method on `self`.

## Inputs

- `k` (`std::int64_t`)
  Algorithm-size/count argument used to shape fit or split behavior.

## Output

- Returns `std::vector<std::pair<std::string, Model>>`.

## Options

Option semantics used by this method:

- `SearchResult` (`struct`):
  - `selection::CompareReport report{}`
    Option field used by this method.
  - `std::optional<Model> best_model{}`
    Option field used by this method.
  - `std::vector<Model> fitted{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = SearchResult::top(/* k */);
out.info("top", value);
```

**Expected output**

```text
top returned std::vector<std::pair<std::string, Model>>
```

### Example 2 (Demo Pattern)

```cpp
  SplitOptions cv_split{};
  cv_split.plan = CVPlanKind::CrossVal;
  cv_split.folds = 5;
  cv_split.shuffle = true;
  cv_split.random.seed = 73;
  cv_split.random.deterministic = true;
  // Force sequential fold evaluation: the shared ParallelRuntime singleton used by
  // pluss::msharp::parallel_for is not safe for concurrent top-level callers, and both
  // cross_assess (fold-level) and sweep (trial-level) can otherwise dispatch concurrently.
  cv_split.max_parallel = 1;

  //========== modelling: broad candidate sweep ========
  // This section runs broad ModelSpec sweep with CV over the preprocessed train batch.
  std::vector<ModelSpec> broad_specs{};

  broad_specs.push_back(ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
```

**Expected output**

```text
top returned std::vector<std::pair<std::string, Model>>
```
