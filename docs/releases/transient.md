# PlussII 0.0.6a — Transient

The latest validated development build. It carries fixes and features that are not yet in a Steady release, so expect it to change from one update to the next. The next Steady release, 0.0.6, will collect what proves stable here.

## Changes since 0.0.5

- The LibTorch bridge (`pluss::series::from_torch` and `Series::tensor()`) now compiles with any include order. `<pluss/pluss.h>` no longer has to come after `<torch/torch.h>`.
- Apple Silicon is published twice: a CPU-only archive (`Darwin-arm64`) and one with the Apple GPU (MPS) backend (`Darwin-arm64-mps`). Intel Mac, Linux and Windows archives are CPU-only.
- Training errors from the tagger and entity-recognition stages no longer name an internal function.
- The PlussView test hook `PLUSSTILS_VIEW_NSEVENTS` posts real mouse events so the hover, drag and double-click paths can be tested without a person at the mouse.

## Compatibility

Unchanged from 0.0.5. The distribution requires CMake 3.28 or newer and a C++20 compiler. See the download page for the archive for your system.
