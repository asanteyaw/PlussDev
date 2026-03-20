### The following needs rigorous tests and reviews
1. SV (sv_paths, sv_price_mc, calibrate_sv) was done by us, not from a direct MATLAB one-to-one function.

In the finance lane, these were implemented primarily by us (no direct MATLAB function clone), so they should be treated as highest-risk and reviewed next:

2. Generic SV lane: stochastic_vol.cpp, heston_sv.cpp, heston_sv_calibration.cpp
3. Heston-Nandi lane: heston_nandi_calibration.cpp, heston_sv.cpp (heston_nandi_* parts), stochastic_vol.cpp (heston_nandi_paths)
4. SLV lane: stochastic_vol.cpp (slv_paths), heston_sv.cpp (slv_price_mc)
5. Calibration engine internals (search/validation architecture): search_core.h, validation_core.h
6. TS-to-finance NGARCH bridge: vol_bridge.cpp
7. Filtering stack (Kalman/EKF/UKF/particle APIs/impl): filtering.h, Math/Finance/filtering/*.cpp
8. Object wrappers/orchestration API: models.h, stochvol_models.cpp
9. MC pricers + variance reduction
The MC pricers in our code were implemented by me natively.
Variance reduction (antithetic, control variate, importance shift, quasi-random options wiring) was also implemented by me, with MATLAB as general inspiration for capability surface, not one-to-one implementation from a single MATLAB pricer file
