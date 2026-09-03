# Lymph `workflow`

`workflow` is the Lymph family for deployable orchestration.

This is the branch you enter when one fitted model is no longer the whole
story and the project needs a durable artifact that knows:

- what payload shape it consumes
- which policies govern execution
- how preprocessing and model state travel together

The `models` family is where an estimator becomes fit-ready. The `workflow`
family is where that fitted intent becomes a reusable runtime artifact.

This family is still being expanded to the same method-level depth as
`models` and `prep`. The landing page is present now so the public family map
stops drifting back to the old flat `model` bucket.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

model/Workflow
model/Payload
model/Policy
```
<!-- apiref:toctree:end -->
