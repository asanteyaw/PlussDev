# MATLAB Parity Cache2

Reference roots:
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph2d`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/graph3d`
- `/Applications/MATLAB_R2023b.app/toolbox/matlab/specgraph`

Audit date:
- `2026-03-03`

Scope rule:
- This cache measures callable plot-family parity, not every MATLAB axes utility.
- A family counts as `implemented` only when PlussPl exposes a direct plotting path for it.
- A family counts as `partial` when PlussPl has a close rendering mode but not the full MATLAB behavior.
- Coverage percentages are separate from visual-parity percentages.
- Visual parity is a renderer/defaults score, not a family-count score.

## 2D Core Plot Families

Audited families:
- `plot`
- `scatter`
- `bar`
- `histogram`
- `stairs`
- `area`
- `errorbar`
- `imagesc/heatmap`
- `boxplot`
- `violin`
- `stem`
- `semilogx`
- `semilogy`
- `loglog`
- `polar`

Status:
- `implemented`: `plot`, `scatter`, `bar`, `histogram`, `stairs`, `area`, `errorbar`, `imagesc/heatmap`, `boxplot`, `violin`, `stem`, `semilogx`, `semilogy`, `loglog`, `polar`
- `missing`: none in the audited family set

Coverage:
- `15 / 15 = 100.0%`

Visual parity:
- `79.0%`
- Gap to MATLAB visual quality: `21.0%`
- Main remaining deficits: richer statistical defaults and finer typography/ruler polish

## 3D Core Plot Families

Audited families:
- `plot3`
- `scatter3`
- `surf`
- `mesh`
- `surfc`
- `meshc`
- `surfl`
- `waterfall`
- `meshz`
- `contour3`
- `fill3`

Status:
- `implemented`: `plot3`, `scatter3`, `surf`, `mesh`, `surfc`, `meshc`, `surfl`, `waterfall`, `meshz`, `contour3`, `fill3`
- `missing`: none in the audited family set

Coverage:
- `11 / 11 = 100.0%`

Visual parity:
- `70.0%`
- Gap to MATLAB visual quality: `30.0%`
- Main remaining deficits: tick/ruler elegance, label placement polish, mesh/surface edge rendering, and PNG text/raster polish

## MATLAB References Used In This Patch

2D references:
- `graph2d/Contents.m`
- `graph2d/semilogx.m`
- `graph2d/semilogy.m`
- `graph2d/loglog.m`
- `specgraph/stem.m`
- `specgraph/polarplot.m`
- `stats/stats/boxplot.m`

3D references:
- `graph3d/Contents.m`
- `graph3d/viewmtx.m`
- `graph3d/view.m`
- `graph3d/daspect.m`
- `graph3d/pbaspect.m`
- `graph3d/surf.m`
- `graph3d/mesh.m`
- `graph3d/surfl.m`
- `specgraph/surfc.m`
- `specgraph/meshc.m`
- `specgraph/waterfall.m`
- `specgraph/meshz.m`
- `specgraph/contour3.m`

## Current Parity Notes

- 2D callable-family coverage is now complete on the audited family set, and `polarplot` now uses a dedicated polar subplot renderer instead of cartesian guide traces.
- `boxplot` and `violin` are now exposed as direct Series-first helpers, implemented in PlussPl as composed statistical traces rather than one-for-one MATLAB object ports.
- `surf`/`mesh`/`surfc`/`meshc`/`surfl` now map to distinct 3D rendering modes instead of one generic surface look.
- `waterfall`, `meshz`, `contour3`, and `fill3` now have direct callable PlussPl 3D entry points.
- `daspect` and `pbaspect` are represented by `Figure3D::data_aspect(...)` and `Figure3D::plot_box_aspect(...)`.
- The implied-volatility 3D demo now uses a plain MATLAB-style surface preset and no longer carries the old legend/colorbar/path composition that made it read like a 4D scene.
