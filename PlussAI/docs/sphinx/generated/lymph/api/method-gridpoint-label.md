# `GridPoint::label`

## API Signature

- `GridPoint::label() -> std::string`

## Notes

Instance method on `self`.

## Inputs

None.

## Output

- Returns `std::string`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = GridPoint::label();
out.info("label", value);
```

**Expected output**

```text
label returned std::string
```

### Example 2 (Demo Pattern)

```cpp
      .overflow = CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
      .one_based = true,
  });
  auto ordinal_or = ordinal.learn_map(cat_batch);
  if (!require_ok(ordinal_or, "OrdinalEncoder.learn_map")) return 1;
  print_shape("ordinal-encoded", ordinal_or.value());

  LabelEncoder label(LabelEncoderOptions{
      .max_categories = 8,
      .overflow = CategoryOverflowPolicy::KeepMostFrequent,
      .include_unknown_bucket = true,
      .one_based = false,
  });
  auto label_or = label.learn_map(cat_batch);
  if (!require_ok(label_or, "LabelEncoder.learn_map")) return 1;
  print_shape("label-encoded", label_or.value());

  std::cout << "NodeJ split + prep demo completed.\n";
  return 0;
}
```

**Expected output**

```text
NodeJ split + prep demo
holdout train idx count=4 test idx count=2
strat holdout train idx count=4 test idx count=2
kfold folds=3
imputed shape=[6, 3]
standardized shape=[6, 3]
minmax shape=[6, 3]
robust shape=[6, 3]
neighbor-imputed shape=[6, 3]
iterative-imputed shape=[6, 3]
vector-normalized shape=[6, 3]
maxabs shape=[6, 3]
...
```
