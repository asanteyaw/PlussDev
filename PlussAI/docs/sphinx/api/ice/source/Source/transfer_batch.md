<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5b0cc00ca89c -->
# `pluss::ai::ice::Source::transfer_batch`

[Back to `pluss::ai::ice::Source`](../Source.md)

## Signature

```cpp
[[nodiscard]] pluss::ai::ice::Batch pluss::ai::ice::Source::transfer_batch(
    const pluss::ai::ice::data::Batch& batch, const pluss::ai::ice::SourceContext& ctx) const;
```

## What It Does

Converts one host-resident [`data::Batch`](../../data/Batch.md) (from
[`learn_batches`](learn_batches.md)/etc.) into the device-resident,
`inputs`/`targets`-shaped core [`Batch`](../../step/Batch.md) a
[`Model`](../../model/Model.md)'s step methods actually consume --
exactly the method `Coach` calls, once per batch, right before each
step method. Not itself virtual, but a fixed template-method sequence
around three protected extension points a subclass overrides:
`before_transfer` (optional hook, no-op by default) ->
`do_transfer_batch` (required, pure virtual -- the real conversion) ->
`after_transfer` (optional hook, no-op by default). See
[`Source`'s own Introduction](Introduction.md#writing-a-custom-source)
for a complete hand-written subclass exercising all three.

## Parameters

`const data::Batch& batch`
: One already-materialized batch, from any of the four `*_batches()`
  accessors.

`const SourceContext& ctx`
: Which stage this is, and the target platform/dtype -- see
  [`SourceContext`](../SourceContext.md).

## Returns

[`Batch`](../../step/Batch.md) -- the core, device-resident batch a
step method consumes.

## Errors

Whatever `do_transfer_batch` throws -- both built-in implementations
throw if `batch` is missing the field they expect (see
[`TensorSource`](../TensorSource.md)/[`DataSetSource`](../DataSetSource.md)).

## Practical Examples

Three examples: a hand-written `Source` subclass's minimal
`do_transfer_batch`, `SourceContext::target_dtype` actually casting a
field, and all three extension points firing in their fixed order.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

namespace {

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

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});
  EchoSource source(x);
  source.prepare();

  ice::SourceContext ctx{};
  const ice::Batch transferred = source.transfer_batch(source.learn_batches().front(), ctx);
  std::cout << "transferred has_inputs() = " << transferred.has_inputs() << "\n";

  return 0;
}
```

Output

```text
transferred has_inputs() = 1
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  using pluss::ScalarType;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 1.0f, 0.0f}, {2, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f}, {2, 1});

  ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{});
  source.prepare();

  ice::SourceContext ctx{};
  ctx.target_platform = pluss::pCPU;
  ctx.target_dtype = ScalarType::Float64;

  const ice::Batch transferred = source.transfer_batch(source.learn_batches().front(), ctx);
  std::cout << "has_inputs() = " << transferred.has_inputs() << "\n";
  std::cout << "has_targets() = " << transferred.has_targets() << "\n";
  std::cout << "inputs dtype == Float64: " << (transferred.inputs().scalar_type() == ScalarType::Float64) << "\n";

  return 0;
}
```

Output

```text
has_inputs() = 1
has_targets() = 1
inputs dtype == Float64: 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>

#include <iostream>
#include <vector>

namespace {

class LoggingSource final : public pluss::ai::ice::Source {
 public:
  explicit LoggingSource(pluss::Series x) : x_(std::move(x)) {}

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
  void before_transfer(const pluss::ai::ice::data::Batch&, const pluss::ai::ice::SourceContext&) const override {
    std::cout << "before_transfer\n";
  }
  pluss::ai::ice::Batch do_transfer_batch(const pluss::ai::ice::data::Batch& batch,
                                          const pluss::ai::ice::SourceContext& ctx) const override {
    std::cout << "do_transfer_batch\n";
    pluss::ai::ice::Batch out{};
    out.set_inputs(batch.field("features").to(ctx.target_platform, ctx.target_dtype));
    return out;
  }
  void after_transfer(pluss::ai::ice::Batch&, const pluss::ai::ice::SourceContext&) const override {
    std::cout << "after_transfer\n";
  }

 private:
  pluss::Series x_;
  bool prepared_ = false;
  std::vector<pluss::ai::ice::data::Batch> batches_{};
  std::vector<pluss::ai::ice::data::Batch> empty_{};
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;

  const pluss::Series x = pluss::make_series({1.0f, 2.0f}, {1, 2});
  LoggingSource source(x);
  source.prepare();

  ice::SourceContext ctx{};
  const ice::Batch transferred = source.transfer_batch(source.learn_batches().front(), ctx);
  std::cout << "has_inputs() = " << transferred.has_inputs() << "\n";

  return 0;
}
```

Output

```text
before_transfer
do_transfer_batch
after_transfer
has_inputs() = 1
```

## See Also

- [`SourceContext`](../SourceContext.md) -- the context every call
  reads.
- [`Source`'s own Introduction](Introduction.md#writing-a-custom-source) --
  the full custom-subclass pattern.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="test_batches.html">
    <span class="label">Previous</span>
    <span class="title">test_batches</span>
  </a>
  <a class="next-link" href="valid_batches.html">
    <span class="label">Next</span>
    <span class="title">valid_batches</span>
  </a>
</div>
<!-- apiref:nav:end -->
