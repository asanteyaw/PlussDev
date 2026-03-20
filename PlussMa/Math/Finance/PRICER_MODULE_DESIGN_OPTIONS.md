# Pricer Module Layer: 3 Design Options

## Option A: Scenario-First Field (Recommended)

### Idea
Build a thin orchestration module that wires `instrument + market + model + engine + loss` and returns callable objectives and evaluation bundles. No optimizer ownership.

### User Interface
```cpp
#include <pluss/ma.h>
namespace fin = pluss::subsp::math::finance;

fin::module::Field pipe{
  .instrument = fin::instrument::Vanilla{
    .type = fin::pricing::OptionType::Call,
    .strike = strikes,
    .maturity = maturities
  },
  .market = fin::market::State{
    .spot = spots,
    .rate = rates,
    .dividend_yield = divs
  },
  .model = fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, cfg),
  .controls = fin::module::Controls{
    .engine = fin::module::Engine::Cos,
    .risk = fin::module::Risk::All,
    .loss = fin::module::Loss::Huber,
    .huber_delta = 0.05
  }
};

auto priced = pipe.price(cfg);
auto objective = pipe.bind_objective_typed<fin::model::Heston>(market_prices, cfg);
double val = objective(fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, cfg));

pipe.controls.engine = fin::module::Engine::MonteCarlo;
pipe.controls.mc.paths = 100000;
pipe.controls.mc.steps = 252;
pipe.simulate(cfg);
auto asset_paths = pipe.paths.asset;
auto vol_paths = pipe.paths.vol;
```

### Pros
- Keeps your “user owns optimizer” rule clean.
- Minimal bloat, reusable for calibration, stress, and benchmarking.
- Easy to add engines (`FFT`, `COS`, `MC`) without changing user workflow.

### Cons
- Slightly more verbose than fully stateful OO wrappers.

---

## Option B: Stateful Module Objects (Calibrator-Like, But Optimizer-Agnostic)

### Idea
Have model-specific module objects that cache scenario and market targets, expose objective evaluators, jacobian stubs, diagnostics, but **do not run optimization** internally.

### User Interface
```cpp
fin::module::HestonObjective obj{inst, mkt, market_prices, cfg};
obj.set_engine(fin::module::Engine::FFT);
obj.set_loss(fin::module::Loss::MSE);

double f = obj.value(fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, cfg));
auto g = obj.grad_fd(fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, cfg)); // optional
```

### Pros
- Best ergonomics for repeated evaluation and production calibration loops.
- Natural place for diagnostics and caching.

### Cons
- More API surface and maintenance than Option A.

---

## Option C: Pure Functional Builders

### Idea
Keep everything functional and stateless: helper functions return objective lambdas and pricing maps.

### User Interface
```cpp
auto objective = fin::module::make_heston_objective(
    inst, mkt, market_prices,
    fin::module::Engine::COS,
    fin::module::Loss::RelativeMSE,
    cfg);

double f = objective(fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, cfg));
```

### Pros
- Smallest implementation and easiest testing.
- Zero object lifecycle overhead.

### Cons
- Harder to attach rich diagnostics/caching without wrapper structs.

---

## Recommendation
Use **Option A** now:
- It is scalable and modular.
- It matches your architecture constraints (tools only, no forced optimizer).
- It can later evolve into Option B ergonomics by adding thin wrappers on top.
