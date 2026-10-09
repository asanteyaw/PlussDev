# Showing a DataFrame, Atlas, or plot

[Back to PlussView](view.md) · [PlussView API Reference](../api_reference.md)

PlussView does not have a call you make to "show a DataFrame" or "show a
plot." That call belongs to the object you have: `DataFrame::show()`,
`Atlas::show()`, and PlussPl's `Studio::show()`/`flash::show()` are where you
actually look. PlussView is the renderer those methods use underneath when
they decide to open a window instead of printing text -- not a separate API
you call yourself.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  using namespace pluss::table;

  auto orders = DataFrame({
      {"id", Slate(pluss::make_series({1, 2, 3}))},
      {"sales", Slate(pluss::make_series({100.0, 120.0, 90.0}))},
      {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
  });

  // The explicit-stream overload always prints plain text, regardless of
  // which window backend is available on this machine.
  display::DisplayOptions options;
  orders.show(std::cout, options);
  return 0;
}
```

Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

Call `show()` without a stream (`orders.show()`, with a GUI-capable
`options.backend`), and it opens a window instead of printing -- the window
that PlussView renders. `options.backend` picks which window implementation
renders it, the same choice `DataFrame::show`'s own reference describes.

For the full, verified behavior of these calls -- every overload, every
option, and a documented bug in one of them -- see each type's own reference,
in that library's own documentation (not this one):

- `DataFrame::show`, in PlussMic's reference (`dataframe/show.md`).
- `Atlas::show`, in PlussMic's reference (`atlas/show.md`) -- its no-stream
  overload is documented there as hanging outside an interactive terminal.
- `Studio::show` and `flash::show`, in PlussPl's reference
  (`studio/show.md`, `flash/show.md`).

PlussView itself has no page for the window layer those methods render
through: it is internal to this library, built entirely from the
[core rendering](view.md) pipeline above, and nothing outside PlussView's own
module constructs it directly.
