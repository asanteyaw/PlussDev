# `pluss::ai::nn::Schedule` Introduction

## Role

`Schedule` is the container every multi-layer network in this reference is
built from -- a named, ordered list of stages, run one after another.
Structurally it is a `Module` (a `ModuleHolder<ScheduleImpl>`, exactly like
`Fusion`/`Conv1d`/etc. wrap their own `Impl`), so it composes recursively:
a `Schedule` can itself be a stage inside another `Schedule`, and
`parameters()`/`train()`/`to()`/`snapshot()` all reach every nested layer
transparently, the same way they would through any other `Module`.

## Stages Do Not Have To Be Modules

Anything exposing `forward(In) -> Out` or `operator()(In) -> Out` can be a
stage -- a plain lambda, a free function object, or an `nn::Module`. A
non-`Module` stage contributes no parameters, is skipped by `train`/`to`,
and [`module_at`](module_at.md) throws if you ask for it, but it otherwise
participates in the chain exactly like a layer: its declared input/output
types are checked against its neighbors the same way. This is how you slot
plain data-reshaping or combining logic between real layers without writing
a throwaway `Module` subclass just to hold it.

## Type-Checked, Not Shape-Checked

Adding a stage (`then`/`then_as`/`insert`/`insert_as`) checks that its
declared **type** matches its neighbor's -- `Series` next to `Series`,
`std::tuple<Series, Series>` next to the same tuple type, and so on, compared
via `std::type_info`. This check is compile-time-declared but
runtime-enforced at the moment a stage is added. It says nothing about
tensor **shape** -- two adjacent `Fusion` layers with incompatible
`in_features`/`out_features` both declare type `Series -> Series`, so the
chain check passes; a shape mismatch only surfaces later, when
[`forward`](forward.md)/[`run`](run.md) actually executes the layers and the
underlying matrix multiply fails.

## Heterogeneous Chains

Most chains are `Series -> Series` throughout, which is what
[`then`](then.md) assumes by default. `Schedule` is not limited to that,
though: `RNN`/`GRU`/`LSTM`'s own `forward` returns `std::tuple<Series,
Series>` (or a nested tuple, for `LSTM`), and [`then_as`](then_as.md) lets a
stage declare exactly that as its output type, so a `Schedule` can chain
directly into one and back out of it with an ordinary stage that unpacks the
tuple. [`run`](run.md) extends the same idea to the chain's *input*: passing
more than one argument packs them into a `std::tuple<...>` for the first
stage to consume.

## Erase Does Not Re-Validate

[`erase`](erase.md) removes a stage and rebuilds the module-registration
tree, but it does **not** re-check that the stages now adjacent to each
other still have compatible types. Erasing a stage that "bridged" two
otherwise-incompatible types leaves a chain that will fail the next time it
runs, not at `erase` time.

## `clear()` Is Not `Module::clear()`

`ScheduleImpl` defines its own `clear()`, which **hides** the inherited
`Module::clear()` entirely (ordinary C++ name hiding -- there is no
`using Module::clear;` bringing the base version back). They do unrelated
things: [`Module::clear`](../Module.md) clears every parameter's
recorded gradient sensitivity and touches nothing else; `Schedule::clear`
erases every stage and resets the auto-naming counter. Calling `.clear()` on
a `Schedule` value always reaches the destructive `ScheduleImpl` version --
see [`clear`](clear.md) for a direct side-by-side comparison.

## Related Pages

- [`module` Introduction](../Introduction.md) -- the `ModuleHolder` wrapping
  mechanism `Schedule` itself is built on.
- [`Module`](../Module.md) -- the base contract every `Module`-like stage,
  and `Schedule` itself, implements.
- [`rnn`](../../rnn.md) -- `RNN`/`GRU`/`LSTM`, whose tuple-returning
  `forward` is the most common reason to reach for `then_as`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="clear.html">
    <span class="label">Next</span>
    <span class="title">clear</span>
  </a>
</div>
<!-- apiref:nav:end -->
