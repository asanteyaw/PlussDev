# Lymph `prep`

`prep` is the Lymph family for conditioning features before they reach a
learner.

This is the family you enter when the central question is not yet "which model
family wins?" but rather:

- which features belong together
- which columns should be scaled, encoded, imputed, or passed through
- which transformation state must be learned once and reused later
- how to keep training-time and inference-time feature handling identical

That concern deserves its own family because preprocessing failures often look
like modeling failures until you inspect the feature contract closely.

## What This Family Owns

`prep` owns five connected concerns:

- `route`: declarative feature selection through `prep::Route`
- `stage`: attachable stage descriptors such as
  `prep::stage::standard_scaler(...)`
- `scaling`: standalone numeric transformers such as
  `prep::StandardScaler`
- `encoding`: standalone categorical transformers such as
  `prep::OneHotEncoder`
- `feature`: multi-branch transformation under one reusable
  `prep::FeatureTransformer`

The family boundary is deliberate:

- `models` owns the learner artifact
- `prep` owns the feature-conditioning contract
- `selection` owns the evidence policy that judges the learner
- `workflow` owns the broader runtime and deployment-shaped orchestration

## Mental Model

Read `prep` in two layers.

The first layer is reusable transformer objects. Those are the classes you can
fit, inspect, and reuse directly:

- `prep::StandardScaler`
- `prep::OneHotEncoder`
- `prep::FeatureTransformer`

The second layer is attachable stage planning. That is the route-plus-stage
surface used by `Model::prepare()` or `Model::append(...)`:

- `prep::Route`
- `prep::Stage`
- `prep::stage::*` factories

If you want a model to carry preprocessing as part of its own contract, the
second layer is usually the right entry. If you want a standalone transformer
object in its own right, start with the first layer.

## Practical Example

This executed example shows the highest-level everyday path: attach prep to a
model, learn once, then infer and assess through the same feature contract.

```{literalinclude} ../../../../examples/NodeJ/lymph/lymph_prep_docverify_demo.cpp
:language: cpp
```

Output

```text
== prep ==
stage_count = 2, trained = true, task_kind = 0

================
     | predicted
----------------
0    | 0
1    | 0
2    | 0
3    | 0
4    | 1
5    | 1
6    | 1
7    | 1
----------------
[8 cs x 1 feat]
accuracy = 1, f1 = 1
```

The point of that example is not the perfect toy accuracy. It is the ownership
boundary:

- the prep contract was attached before fitting
- the fitted model reused it for inference
- the same contract also governed assessment

```{toctree}
:maxdepth: 3
:hidden:

route <prep/route>
stage <prep/stage_family>
scaling <prep/scaling>
encoding <prep/encoding>
feature <prep/feature>
```
