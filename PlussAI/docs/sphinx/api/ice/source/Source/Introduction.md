# `pluss::ai::ice::Source` Introduction

## What `Source` Is

`Source` decouples [`Coach`](../../coach/Coach.md) from where training/
validation/test/inference data actually comes from. `Coach` only ever
calls [`prepare()`](prepare.md) once, then reads whichever
`*_batches()` accessor the current stage needs and calls
[`transfer_batch`](transfer_batch.md) on each one. Both built-in
sources -- [`TensorSource`](../TensorSource.md) (raw `Series` pairs) and
[`DataSetSource`](../DataSetSource.md)
([`data::DataSet`](../../data/DataSet.md) parts) -- implement exactly
this surface; nothing about `Coach`'s own loop changes based on which
one (or a hand-written third option) is in use.

## Writing A Custom Source

A subclass implements five required methods: `prepare` and the four
`*_batches()` accessors (all pure virtual), plus `do_transfer_batch`
(also pure virtual, but `protected` -- called internally by the public,
non-virtual [`transfer_batch`](transfer_batch.md)). Two more protected
hooks, `before_transfer`/`after_transfer`, default to no-ops and exist
for a subclass that needs to act immediately around the conversion
without touching `do_transfer_batch` itself -- the same
override-only-what-you-need shape
[`Callback`](../../callback/Callback.md)'s hooks follow.

Implement a `Source` that serves one batch for both training and inference, then prepare it. The program prints the batch count for each stage.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

class EchoSource final : public pluss::ai::ice::Source {
 public:
  explicit EchoSource(pluss::Series x) : x_(std::move(x)) {}

  void prepare() override {
    if (prepared_) return;
    pluss::ai::ice::data::Batch batch(pluss::ai::ice::data::Layout::CrossSection);
    batch.set("features", x_);
    batches_ = {batch};
    prepared_ = true;
  }
  const std::vector<pluss::ai::ice::data::Batch>& learn_batches() const override { return batches_; }
  const std::vector<pluss::ai::ice::data::Batch>& valid_batches() const override { return empty_; }
  const std::vector<pluss::ai::ice::data::Batch>& test_batches() const override { return empty_; }
  const std::vector<pluss::ai::ice::data::Batch>& infer_batches() const override { return batches_; }

 protected:
  pluss::ai::ice::Batch do_transfer_batch(const pluss::ai::ice::data::Batch& batch,
                                          const pluss::ai::ice::SourceContext& ctx) const override {
    pluss::ai::ice::Batch out{};
    out.set_inputs(batch.field("features").to(ctx.target_platform, ctx.target_dtype));
    return out;
  }

 private:
  pluss::Series x_;
  bool prepared_ = false;
  std::vector<pluss::ai::ice::data::Batch> batches_{};
  std::vector<pluss::ai::ice::data::Batch> empty_{};
};

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  EchoSource source(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3, 1}, cfg));
  source.prepare();
  std::cout << "learn batches = " << source.learn_batches().size() << "\n";
  std::cout << "infer batches = " << source.infer_batches().size() << "\n";
  return 0;
}
```

Output

```text
learn batches = 1
infer batches = 1
```

See [`transfer_batch`'s own page](transfer_batch.md) for this same
class run end to end, and with `before_transfer`/`after_transfer`
added to show the fixed firing order.

## Related Pages

- [`TensorSource`](../TensorSource.md) / [`DataSetSource`](../DataSetSource.md)
  -- the two built-in implementations.
- [`SourceContext`](../SourceContext.md) -- what every `transfer_batch`
  call reads.
- [`Coach`](../../coach/Coach.md) -- the only caller of `prepare`/the
  `*_batches()` accessors in the ordinary flow.
- [`data::Batch`](../../data/Batch.md) -- the host-resident type every
  `*_batches()` accessor returns, distinct from the core
  [`Batch`](../../step/Batch.md) `transfer_batch` produces.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="infer_batches.html">
    <span class="label">Next</span>
    <span class="title">infer_batches</span>
  </a>
</div>
<!-- apiref:nav:end -->
