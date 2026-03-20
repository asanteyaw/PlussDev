# `Candidate`

## API Signature

- `Candidate`

## Inputs

- This is a public data type. Populate its fields before passing into APIs.

## Output

- A `Candidate` object used by NodeJ/Lymph workflows.

## Notes

Public user-facing type used directly in NodeJ/Lymph workflows.

## Fields

Public fields on this type:

- `std::string name{}`
- `Workflow workflow{}`
- `Candidate& operator=(Candidate&&) noexcept = default`
- `Candidate& operator=(const Candidate&) = delete`

## Examples

```cpp
Candidate cfg{};
```
