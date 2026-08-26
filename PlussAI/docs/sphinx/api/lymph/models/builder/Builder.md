# `lymph::models::Builder`

## Summary

`lymph::models::Builder` is the named candidate-model workspace in the
`models::builder` subfamily.

Use it when you want several candidate `Model` objects to live inside one
coordinated object before spotchecking, benchmarking, or search.

## Signature

```cpp
lymph::models::Builder lymph::models::build();
lymph::models::Builder lymph::models::build(
    std::initializer_list<std::pair<std::string, lymph::models::Model>> entries);

pluss::table::DataFrame lymph::models::Builder::report(std::int64_t n = -1) const;
lymph::models::Model lymph::models::Builder::best() const;
lymph::models::Model lymph::models::Builder::worst() const;
```

## Practical Example

This page uses the exact verifier program
`lymph_models_builder_docverify_demo.cpp`.

### Example 1

This example constructs a real candidate-model workspace, prints the scalar
state of the builder itself, and then prints the report table that summarizes
the registered candidates.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  lymph::models::Builder builder = lymph::models::build({
      {"logreg", lymph::models::logistic_regression({.fit_intercept = true})},
      {"tree", lymph::models::decision_tree_classifier({.max_depth = 3})},
      {"knn", lymph::models::knn_classifier({.k = 3})},
  });

  std::cout << "== builder ==\n";
  std::cout << "size=" << builder.size()
            << " empty=" << std::boolalpha << builder.empty() << "\n";
  std::cout << builder.report() << "\n";

  return 0;
}
```

Output

```text
== builder ==
size=3 empty=false
===================================================
     | name    task_kind       trained  stage_count
---------------------------------------------------
0    | logreg  classification  false    0          
1    | tree    classification  false    0          
2    | knn     classification  false    0          
---------------------------------------------------
[3 cs x 4 feat]
```

That is the minimal honest builder loop: register named candidates, confirm the
workspace state, and inspect the tabular summary before any benchmark,
spotcheck, or search phase begins.

## Related Pages

- [models family](../../models.md)
- [builder Introduction](Introduction.md)
