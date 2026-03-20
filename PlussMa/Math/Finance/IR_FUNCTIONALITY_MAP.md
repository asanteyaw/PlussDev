# Interest-Rate Functionality Map (MATLAB `fininst` -> Pluss)

## MATLAB `fininst` IR usage buckets
- Tree/model construction: `hwtree`, `cirtree`, `hjmtree`, `bdttree`, `bktree`
- Core fixed-income pricing on trees: `bondby*`, `fixedby*`, `floatby*`, `swapby*`
- IR derivatives: `capby*`, `floorby*`, `swaptionby*`, `optbndby*`, `optfloatby*`
- Calibration/fit helpers: `hwcalbycap`, `hwcalbyfloor`, `hwcalbycapfloor`
- Curve/spread/risk utilities: `oasby*`, `*sens`, `findeffrates`

## Current Pluss coverage
- Path simulation:
  - `rates::vasicek_paths`, `rates::cir_paths`, `rates::hull_white_paths`, `rates::hjm_paths`
- Bond discounting:
  - `rates::vasicek_zcb`, `rates::cir_zcb`, `rates::hull_white_zcb`, `rates::hjm_zcb_mc`
- Object APIs:
  - `models::VasicekRate`, `models::CirRate`, `models::HullWhiteRate`, `models::HjmRate`
- Calibration:
  - rate calibration routes in `calibration::` and object `calibrate()/calibrate_inplace()`

## Corresponding additions to reach MATLAB-style IR breadth
- Fixed-income pricers on top of rate models:
  - coupon bond PV, fixed leg PV, floating leg PV, vanilla IRS PV
- IR option layer:
  - bond options, cap/floor (caplet/floorlet strip), swaptions
- Sensitivity and spread layer:
  - DV01/PV01, model greeks, OAS-style spread solve
- Calibration utilities:
  - direct cap/floor/swaption objective builders (tooling only, optimizer user-driven)

## Implementation direction
- Reuse existing `rates` simulators and ZCB kernels as the common engine.
- Keep pricing APIs Series-first and vectorized.
- Expose both functional and object facades, matching the current finance design.
