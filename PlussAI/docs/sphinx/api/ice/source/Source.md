# `pluss::ai::ice::Source`

The pluggable train/valid/test/infer batch-feeding abstraction Model/Coach iterate over -- prepare() once, then the four *_batches() accessors and transfer_batch() to device-resident core Batch.

## Signature

```cpp
// the public factories / constructors that produce this object
// e.g. lymph::models::Model lymph::models::logistic_regression(...);
```

## Construction

<!-- how an instance is obtained; delete if the Signature block says it all -->

## State

<!-- the observable fields / accessors and what each means -->

## Methods

<!-- one line per method, linking its page; the toctree below carries the sidebar entries -->

## Changelog

- **0.1** — <!-- initial documented surface -->

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Source/Introduction
Source/infer_batches
Source/learn_batches
Source/prepare
Source/test_batches
Source/transfer_batch
Source/valid_batches
```
<!-- apiref:toctree:end -->
