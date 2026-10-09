# `pluss::tils::OutputEngine::options`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputOptions options() const;
```

`options` returns a copy of the engine's current configuration. The copy is a snapshot: changing it does not change the engine, and a later change to the engine does not change the copy.

## Parameters

This member takes no parameters.


## Returns

An `OutputOptions` holding the current separator, prefixes, levels setting, minimum level, and flush behavior.

## Errors

None.

## Practical Examples

### Example 1

Example 1 reads the defaults of a new engine.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  OutputEngine out;
  const OutputOptions now = out.options();
  std::cout << "separator: [" << now.separator << "]\n";
  std::cout << "levels enabled: " << now.levels_enabled << '\n';
  std::cout << "min level is trace: " << (now.min_level == OutputLevel::Trace) << '\n';
  return 0;
}
```

Output

```text
separator: [ ]
levels enabled: true
min level is trace: true
```

A new engine uses a single space, has levels enabled, and has the minimum level set to trace.

### Example 2

Example 2 reads the configuration after two changes.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  OutputEngine out;
  out.sep(" ; ");
  out.enable_levels(false);
  std::cout << "separator now: [" << out.options().separator << "]\n";
  std::cout << "levels now enabled: " << out.options().levels_enabled << '\n';
  return 0;
}
```

Output

```text
separator now: [ ; ]
levels now enabled: false
```

The separator and the levels setting show the values set on the engine.

### Example 3

Example 3 edits a copy and checks the engine.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  OutputOptions snapshot = out.options();
  snapshot.separator = "::";
  out.writeln("a", "b");
  std::cout << "engine separator after editing the copy: [" << out.options().separator << "]\n";
  return 0;
}
```

Output

```text
a b
engine separator after editing the copy: [ ]
```

The engine keeps its separator, because the copy was a snapshot.

## See Also

- [`OutputEngine`](../core-output-engine.md)
