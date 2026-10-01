# `pluss::ai::ice::RunState`

Mutable run-wide state a Callback's hooks read and write: model pointer, stage, platform, epoch/step counters, a named metrics map, checkpoint paths, and stop-request/exception bookkeeping.

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

RunState/Introduction
RunState/metric
RunState/put_metric
```
<!-- apiref:toctree:end -->
