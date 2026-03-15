# Demo Catalog

## CodeM (Series)

- `PlussQit/examples/CodeM/series_usage_demo.cpp`: construction, dtype/platform transfer, host extraction, printing.
- `PlussQit/examples/CodeM/series_methods_demo.cpp`: broad method-family sweep with pass/fail output.
- `PlussQit/examples/CodeM/stats_demo.cpp`: distribution/stats/moving-window use.
- `PlussQit/examples/CodeM/signal_demo.cpp`: DCT/DST/Hilbert/shift workflows.
- `PlussQit/examples/CodeM/transmit_demo.cpp`: unary, zipped, and parallel `pluss::transmit(...)` usage.
- `PlussQit/examples/CodeM/aux_let_transmit_demo.cpp`: `suppose`, `let`, and `transmit` working together in one workflow.
- `PlussQit/examples/CodeM/suppose_demo.cpp`: focused `suppose` / `let_as(...)` patterns.
- `PlussQit/examples/translate_demo.cpp`: `transform_axis(...)` for `Series`, `Note`, `Timeline`, and `TimeIndex`.
- `PlussQit/examples/CodeM/autograd_optim_demo.cpp`: end-to-end autodiff, transforms, and optimizer integration.
- `PlussQit/examples/CodeM/autodiff_minimal_demo.cpp`: shortest reverse + `value_and_grad` usage.
- `PlussQit/examples/CodeM/autodiff_derivatives_demo.cpp`: scalar first/second derivative checks.
- `PlussQit/examples/CodeM/another_auto_diff_demo.cpp`: duplicate scalar derivative smoke for regression coverage.

## CodeN (Note)

- `PlussQit/examples/CodeN/note_factory_demo.cpp`: all major factory paths (UTF-8 + bytes).
- `PlussQit/examples/CodeN/note_base_methods_demo.cpp`: shape/index/iterators/base helpers.
- `PlussQit/examples/CodeN/note_method_families_demo.cpp`: NLP/TeX/token families.
- `PlussQit/examples/CodeN/note_printing_demo.cpp`: rendering options and bytes visualization.

## CodeH (Timeline / TimeIndex)

- `PlussQit/examples/CodeH/timeline_non_generated_and_factory_methods_demo.cpp`: factory and non-generated helpers.
- `PlussQit/examples/CodeH/timeline_printing_2d_3d_demo.cpp`: 2D/3D timeline rendering.
- `PlussQit/examples/CodeH/timeline_dataframe_timeindex_methods_demo.cpp`: frequency, resample, rolling.

## Dynamic / Aux

- `PlussQit/examples/queue_demo.cpp`: `pluss::Queue`, dynamic `invoke(...)`, and `Queue::where(...)`.
- `PlussQit/examples/dispense_demo.cpp`: method-pointer dispatch with `dispense(...)` and `dispense_as<T>(...)`.

## PlussAI (NN)

- `PlussAI/examples/adam_oop_demo.cpp`: explicit-gradient and autodiff/tape Adam workflows.
- `PlussAI/examples/adagrad_oop_demo.cpp`: explicit-gradient and autodiff/tape Adagrad workflows.
- `PlussAI/examples/nn_lstm_train_minimal_demo.cpp`: minimal LSTM training loop with closure-based optimizer step.
- `PlussAI/examples/nn_ngarch_train_minimal_demo.cpp`: NGARCH parameter training with closure-based optimizer step.
- `PlussAI/examples/nn_optimizer_closure_minimal_demo.cpp`: shortest closure-based `optimizer.step([&]{ ... })` training path.
