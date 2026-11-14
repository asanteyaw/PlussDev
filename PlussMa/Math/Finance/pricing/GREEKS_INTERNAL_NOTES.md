# Field Greeks: internal notes (not for the public docs)

Status as of 2026-11-05. Every Greek in the Field engines comes from the engine's own structure
(closed form, tangent through the recursion/solve, pathwise differentiation, or a pricing-equation
identity). The only reprices left are listed under "Still finite differences".

## How each engine gets its Greeks

| Engine | Method |
|---|---|
| BS analytic, SABR analytic | closed form / expansion differentiated |
| COS (BS, Heston, Bates), Europe + Bermudan/American | CF-multiplier derivatives; forward-mode tangent through the exercise recursion |
| FFT (little-trap and original form) | multiplied transforms (original form borrows little-trap, same analytic function) |
| Spectral American (BS) | implicit differentiation of the boundary fixed point with duals |
| Lattice (steps >= 2) | nodes for delta/gamma/theta, forward-mode tangent for vega/rho |
| Heston PDE (ADI) | surface read-offs for delta/gamma/vega/theta, tangent through every ADI solve and the exercise projection for rho |
| MC European (BS, Heston, Bates, rough*) | pathwise from the shared paths; importance sampling handled by the likelihood ratio |
| MC European SABR | tracked forward + alpha0; gamma by payoff second difference along the pathwise Jacobian; IS reweighted, CV only changes the price |
| MC American/Bermudan (BS, Heston, Bates, rough*) | one LSM fit, Greeks along the frozen exercise rule (`pricing::detail::lsm_greeks`) |
| MC American/Bermudan SABR | same, non-homogeneous branch: tracked spot (per path), tracked drift, tracked alpha0, one reverse pass |

## Still finite differences / not exact (and why)

1. **LSM theta (all models except Black-Scholes).** Second-order backward difference of the
   LSM fit over the two earlier horizons of the same paths (a refit on truncated paths). Reason:
   a pathwise time derivative does not exist (the exercise grid and stopping rule move with the
   horizon), and the generator identity needs mixed second derivatives we do not have
   (Heston: V_v, V_Sv, V_vv; SABR: V_aa, V_Fa). Black-Scholes uses the pricing-equation identity
   `theta = rV - (r-q) S delta - 1/2 sigma^2 S^2 gamma`, within about 0.1 of the lattice on the test
   case. Heston stencil was about 0.17 off, SABR American theta is the stencil too and is
   unvalidated against a reference (no grid engine prices SABR American).
2. **LSM gamma (all models).** Second difference of the *stopped payoff estimator* with the
   exercise rule frozen (spot-scaled paths; for SABR, paths re-simulated from a scaled spot with
   the same normals). Not a reprice (no refit) but it is a bump of the paths. A pathwise second
   derivative does not exist (payoff'' is a delta function). Step is 5% so the variance of the
   difference is tolerable.
3. **MC European gamma (all models).** Same reason: payoff second difference along the pathwise
   Jacobian or spot scale, never a reprice.
4. **MC European theta (all models).** Backward difference over earlier dates of the same paths.
   For SABR it is combined with the exact drift/discount terms. Not a refit, but a stencil.
5. **Lattice with steps < 2.** A one-step tree has no second level, so gamma/theta from nodes are
   undefined; falls back to `numeric_transform_greeks_` (bump-and-reprice). Not worth fixing:
   a one-step tree is not a meaningful price.
6. **Bermudan COS theta (Black-Scholes) is the last-date derivative**; Heston/Bates COS theta is
   calendar theta (first interval shrinks). The two conventions differ by design of the recursion,
   so do not compare them.
7. **Pathwise vega ignores control variates.** The CV only reduces the price variance; the Greeks
   are those of the unbiased plain estimator, so they are consistent in expectation but do not
   enjoy the CV variance reduction.
8. `numeric_transform_greeks_` is still in `objects.cpp` solely for the lattice steps < 2 case.

## Bug-prone points

- SABR scaling: `(F0, alpha0) -> (lambda F0, lambda^(1-beta) alpha0)` scales the whole path by
  lambda, but `F0` alone does not. That is why SABR cannot use the spot-scaling shortcut and needs
  the tracked per-path spot. (Euler identity `F0 dV/dF0 + (1-beta) alpha0 dV/dalpha0 = ...` would also
  give delta from vega, unused.)
- A SABR American put with low vol and 5% rate is just exercised immediately (price == intrinsic,
  all Greeks degenerate): choose tests with alpha0 around 1.5 at spot 100.
- Envelope theorem holds for COS exercise points (continuous value) but NOT for the spectral
  boundary; it holds for LSM only with the rule frozen per path.
