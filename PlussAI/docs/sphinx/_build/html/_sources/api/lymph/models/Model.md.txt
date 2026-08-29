# `lymph::models::Model`

`Model` is the trainable artifact at the center of the `models` family.

Use this branch when one estimator object is the thing you care about:

- shaping it with `prep`
- training it with `learn` or `learn_`
- calling `infer`
- assessing it into `reports::Metric`
- persisting it with `save` and `load`

## Pages

```{toctree}
:maxdepth: 2

Model/Introduction
Model/append
Model/assess
Model/infer
Model/learn
Model/load
Model/prepare
Model/save
Model/Notes
```
