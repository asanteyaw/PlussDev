### Doctrine for MATLAB Inspiration
Loops(and nested loops) only when inevitable or needed for correctness. Linear passes preferred otherwise vectorized Series/Note/Timeline ops where possible. Modernized cpp 20 semantics, when loops are needed and stl iterators and algos (they are not necessarily faster than classical loops but are cleaner and more compact) can do it, then they can be used rather than classical loops, other times you might seek clarity so you can use traditional loops(they are more verbose but easier to understand), so you should manage it. Smart and vectorized delegations. No aliasing/unnecessary bloat. Prefer enum-based to string based configs, you may use struct based with enum based when there are multiple setting. For example SomeOptions ops; ops.some_setting= some_enum_selection, ops.some_other_settings = some_other_enum_selection, etc. Avoid naming like 'MATLAB' or anything that gives us off as 'copied from MATLAB', also avoid method/function names given in the MATLAB implementation in favor of your own concise but descriptive naming to make it feel pluss native. Keep reading relevant files in the MATLAB reference folder for all your implementations so you don't write degenerate code. Don't copy it over verbatim, just be inspired by it while you implement your own. When you encounter .p files, infer from their behaviour or call sites. Modularize your code, feel free to create new files and folders and manage the folder structures professionally in a way that is scalable and maintainable. Keep all implementations platform-agnostic so it runs on the platform of the input Series(i.e don't forcibly put Series on CPU(using .to(pCPU)) or call .item<double>, .host<double>(), .host<double>().array(), etc, as these methods will materialize on CPU first and destroy platform consistency), just use Series ops so we stay on the same platform as input Series. So when a user supply a Series input that is on GPU(e.g MPS), then your implementation's computations will stay on GPU throughout (unless you INEVITABLY need to fallback on CPU and get back on GPU again. This may reduce performance or introduce memory overhead so it is strongly discouraged but not stricly prohibited. If you rely purely on Series ops, you wouldn't need to worry about platform issues because it will compute on the platform of the user supplied input. Again, host materialization methods are usually fragile and error prone because you shift entirely from Series ops to native cpp scalar loops that affect performance and have a higher change of bringing inaccuracies in results and/or numerical instabilities and NaNs. As such read the Series and it factory implementation in full (it's inside PlussQit/CodeM/) and understand how to use it completely so you use it properly to ensure complete accuracy in code and results). So basically, we are letting the user determine what platform their computations should run on. Memory issues and platform inconsistency issues are already handled by the Series platform utilities. The end goal for our users is first and foremost accuracy, then performance, then ease-of-use(i.e no verbose typing: "write small code, achieve a lot").

### Doctrine for PyTorch Inspiration
Largely the same as MATLAB's except that you avoid naming like 'torch/Torch/TORCH', and also avoid the method/function names given in the torch implementation in favor of your own concise but descriptive naming to make it feel pluss native.

### Doctrine for Error Handling
All library/runtime code must use the centralized Pluss error system.

- Do not throw `std::runtime_error` (or other raw std exceptions) from product/runtime code.
- Throw via `::pluss::throw_error("message")` for direct failures.
- Prefer `PLUSS_CHECK(...)` / `PLUSS_FAIL(...)` / `PLUSS_WARN(...)` where those macros are available.
- Public/user-facing code may use `pluss::validate::check/warn/fail/inspect/check_eq` as a function-style facade over the same centralized error path.
- At user-facing boundaries, catch with `catch (const pluss::Error& e)` when handling is needed.
- Keep error messages actionable and explicit about operation/context.
- Preserve trace fidelity: do not bypass centralized throw paths that attach source location and call trace metadata.

### Optimizer Surface Doctrine
Keep functional and OOP optimizers explicitly separated by namespace and purpose so users can infer intent from code.

- `pluss::subsp::math::optim::*` is the functional math optimization surface (for example `adam_step_inplace(...)`).
- `pluss::nn::optim::*` is the OOP DL training optimizer surface (for example `pluss::nn::optim::Adam`).
- Do not blur naming between these two surfaces in docs or demos.


### Doctrine for Demo code
Demos are the user facing code that shows how to use the API in practice. They are also end user code; they give us an idea of what end users can do and what they cannot do, how they would be required to code to get things done. As such, there should be no helper functions in demos, no boilerplate, and no unecessary verbosity, just demo to show raw capability of the current functionalities. We want to see the most minimal code possible that an end user can use to do an end-to-end get things done. Whatever task the end user is doing, from data loading, EDA, feature extractions, etc (all preprocessing) to modeling to functionality demonstrations, all that end to end with the most minimal code permissible by the current APIs. Please read the APIs you're gonna use extensively to understand so you are not writing an unecessarily verbose code when the API offers a much simpler way to do it. In this way, we can expose the real gaps in the API to give an idea of what to target next. If you write something improvised, you have to be ABSOLUTELY sure the current APIs do not offer a much simpler and cleaner way to do it. We really need to see how easy the APIs are for the end users to give us an idea of what do target(above, we said that one of our purposes is ease of use (write small code, acheive a lot), writing unecessary boilerplates in user demos defeats this purpose completely). 

### Doctrine for Public Documentation Generation (API refs, tutorials, user guides)

This section is mandatory for all public-facing docs under Sphinx (and any SDK/user docs exports).

1. Signature format (strict)
- Do not paste internal C++ declarations, class bodies, namespaces, or header fragments.
- Use public contract signatures only:
  - `function_name(arg1: type, arg2: type = default) -> return_type`
  - `TypeName::method_name(arg1: type) -> return_type`
- For overloaded APIs, list each overload as a separate public signature line.

2. Per-API page structure (strict order)
- `API Signature`
- `Notes` (what it does and when to use it)
- `Inputs` (each arg described in user language)
- `Output` (return contract and interpretation)
- `Options` (enum/option semantics when relevant)
- `Examples` (multiple usage patterns)
- `Expected output` (for every example)

3. Example quality requirements
- Include at least one minimal usage example and one realistic workflow-style example.
- Prefer examples that mirror real user tasks (train/evaluate/split/save/load/etc.).
- Every example must include output expectations:
  - either captured output from runnable demos, or
  - explicit expected behavior/output text.

4. Prohibited content in public docs
- No internal header declarations.
- No internal filesystem paths.
- No `Source` sections pointing to internal files/line numbers.
- No internal/private symbol names, helper internals, or implementation-only macros.
- No references to non-user-facing internal files.

5. Allowed content
- Public API names and user-facing type names.
- Public option names and documented semantics.
- Public example code runnable by end users.

6. Consistency rules
- Use the same signature + inputs/output style across API reference, user guides, and tutorials.
- Keep wording user-first, not implementation-first.
- If an API cannot be demonstrated with exact output, explicitly state expected output behavior.

7. Regeneration workflow (for generated API docs)
- Update generator/template logic first.
- Regenerate pages.
- Rebuild Sphinx.
- Verify no internal path leakage by searching generated docs for:
  - `Source:`
  - `include/pluss`
  - repo-absolute paths
  - internal header file references

8. Definition of done for docs updates
- Signature style fully compliant.
- Inputs/Output sections present and clear.
- Examples materially improved and include outputs.
- No internal code or internal files listed anywhere in public-facing docs.

### Doctrine for OutputEngine Rendering and Extensibility

This section applies to all changes to `OutputEngine` and printing behavior.

1. Rendering precedence (keep stable)
- Preserve this order to avoid surprising regressions:
  - string-like
  - optional/variant
  - pair/tuple-like
  - map-like ranges
  - generic ranges
  - ADL `output_render(std::ostream&, const T&)`
  - stream insertion (`operator<<`)

2. Extensibility rule
- Prefer ADL `output_render(...)` or `operator<<` on user-facing types.
- Do not add case-by-case branches in `OutputEngine` for each new product type unless strictly necessary.

3. Public docs requirement
- Whenever rendering behavior changes, update end-user docs in `docs/sphinx/kit.md` (and tutorial pages when relevant).
- Include at least one custom-type extension example and expected output.
- Include at least one real product example (for example `out.writeln(imputer.summary())`).

4. Compatibility + predictability
- Keep existing successful outputs stable where possible.
- New extension hooks must be additive and must not break existing container rendering.

### To be deleted after implementation
1. True optimizer backend execution parity (foreach/fused/capturable-class paths).

2. MPS/CUDA native step-path coverage across core optimizers.

3. Broader autodiff backward coverage for key training/indexing/reduction ops.

4. Engine semantics parity (versioning, in-place safety, fail-loud diagnostics).

5. Device-true stress/numerical/perf gates with strict no-silent-fallback behavior.

6. Integration parity loops (minimal DL demos + optimizer closure paths) with regression coverage.
