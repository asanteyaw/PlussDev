# DataFrame::unify

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unify(pluss::RangeView<std::string_view> left_on, const DataFrame& right, pluss::RangeView<std::string_view> right_on = {}, std::string_view how = "common", std::string right_suffix = "_right", std::string_view validate = "many_to_many") -> DataFrame

DataFrame::unify(std::initializer_list<std::string_view> left_on, const DataFrame& right, std::initializer_list<std::string_view> right_on = {}, std::string_view how = "common", std::string right_suffix = "_right", std::string_view validate = "many_to_many") -> DataFrame

DataFrame::unify(std::string_view left_on, const DataFrame& right, std::string_view right_on = "", std::string_view how = "common", std::string right_suffix = "_right", std::string_view validate = "many_to_many") -> DataFrame
```

## What It Does

Combines two frames by key values. Use `right_on=""` when both frames use the same key name, Pluss-native modes such as `"common"` or `"unite"` to choose retained keys, and `validate` for duplicate-key checks.

## Parameters

**left_on**
    Key feature name(s) on the left frame used to match rows during `unify(...)`.

**right**
    Right-hand frame whose rows are matched against the left frame by the requested key features.

**right_on**
    Key feature name(s) on the right frame used to match rows during `unify(...)`. Leave empty to reuse the same names as `left_on`.

**how**
    Join mode for `DataFrame::unify(...)`.

    Supported values:
    - `common`: Keep only rows whose key values match on both sides.
    - `keep_left`: Preserve left keys and attach right-hand matches where they exist.
    - `keep_right`: Preserve right keys and attach left-hand matches where they exist.
    - `unite`: Keep the stable union of keys from both frames.
    - `cross`: Create the cartesian product; key feature arguments must be empty.
    - `left_only`: Keep left rows whose keys do not occur on the right.
    - `right_only`: Keep right rows whose keys do not occur on the left.

**right_suffix**
    Suffix added to right-hand feature names only when a non-key right feature would otherwise collide with an existing left feature name.

**validate**
    Optional key-cardinality check for `DataFrame::unify(...)`.

    Supported values:
    - `many_to_many`: Allow duplicate key values on both sides. This is the default.
    - `one_to_one`: Require each key to appear at most once on both sides.
    - `one_to_many`: Require unique keys on the left and allow duplicates on the right.
    - `many_to_one`: Allow duplicates on the left and require unique keys on the right.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Join two frames on several key features supplied through a reusable string view.

```cpp
auto left = pluss::table::DataFrame({
    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},
    {"midterm", pluss::make_series({71, 82, 93})},
});
auto right = pluss::table::DataFrame({
    {"OrgDefinedId", pluss::make_note({"20", "30"})},
    {"Final Points Grade", pluss::make_series({88, 99})},
});

auto out = left.unify(
    "OrgDefinedId",
    right,
    "",
    "left",
    "_right",
    "many_to_one"
);
```

### Output

```text
OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99
```
Example 2. Join two frames on several key features listed inline.

```cpp
auto left = pluss::table::DataFrame({
    {"course", pluss::make_note({"STAT200", "STAT200", "STAT300"})},
    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},
    {"midterm", pluss::make_series({71, 82, 93})},
});
auto right = pluss::table::DataFrame({
    {"course_code", pluss::make_note({"STAT200", "STAT300"})},
    {"Username", pluss::make_note({"20", "30"})},
    {"Final Points Grade", pluss::make_series({88, 99})},
});

auto out = left.unify(
    {"course", "OrgDefinedId"},
    right,
    {"course_code", "Username"},
    "inner",
    "_right",
    "one_to_one"
);
```

### Output

```text
course   OrgDefinedId  midterm  course_code  Username  Final Points Grade
STAT200  20            82       STAT200      20        88
STAT300  30            93       STAT300      30        99
```
Example 3. Join two frames on one key feature name.

```cpp
auto left = pluss::table::DataFrame({
    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},
    {"midterm", pluss::make_series({71, 82, 93})},
});
auto right = pluss::table::DataFrame({
    {"OrgDefinedId", pluss::make_note({"20", "30"})},
    {"Final Points Grade", pluss::make_series({88, 99})},
});

auto out = left.unify(
    "OrgDefinedId",
    right,
    "",
    "left",
    "_right",
    "many_to_one"
);
```

### Output

```text
OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99
```

## Join Semantics

- `unify(...)` matches rows by key values, not by current row order.
- `common`, `keep_left`, `keep_right`, and `unite` retain the corresponding key set while preserving stable input order.
- `left_only` and `right_only` select unmatched keys; `cross` creates the cartesian case product without key features.
- Repeated keys expand to every matching pair, and `validate` can enforce one-to-one or one-sided uniqueness.
- When right-hand non-key names collide with left-hand names, `right_suffix` is applied only to the right-hand collisions.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ts.html">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
  <a class="next-link" href="unsqueeze.html">
    <span class="label">Next</span>
    <span class="title">unsqueeze</span>
  </a>
</div>
