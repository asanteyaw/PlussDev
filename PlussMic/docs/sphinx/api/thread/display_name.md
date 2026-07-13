# Thread::display_name

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::display_name() -> std::string
```

Return the display name attached to the current thread.

The display name is presentation metadata, useful for rendering and labeling, not for changing the underlying values.

Use it when you need the current human-facing label without reaching into renderer-specific code.

## Parameters

`None`

## Returns

`std::string`
    Current display name, if one has been attached.

## See Also

- <a href="with_display_name.html"><code>Thread::with_display_name</code></a>, <a href="clear_display_name.html"><code>Thread::clear_display_name</code></a>: attach or remove the metadata.

## Notes

Like `display_labels()`, this is metadata-only. Reading it does not mutate the thread or affect later numeric, text, or temporal operations.

## Examples

Example 1. Inspect the display name attached to a thread.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}))
               .with_display_name("scores")
               .display_name();
```

### Output

```text
scores
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describe.html">
    <span class="label">Previous</span>
    <span class="title">describe</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
