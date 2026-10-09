# PlussTils: Reading the environment

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

Most programs take some of their settings from the environment: the region a service runs in, the port it listens on, whether debug output is on. The environment is a set of named strings that the shell or the deployment provides before the program starts. This chapter shows how to read those strings safely, with a default when a name is missing, a typed value when a number or flag is needed, and a range when a value must stay within limits.

## The mental model

An environment variable is always a string, and it can be absent. The readers in this family ask one question each. `get` asks whether the variable is set and returns the string or nothing. `get_or` returns the string, or the default you give when the variable is not set. The typed readers convert the string to a number or a flag, and when the text is not a valid value they return the default instead of failing. The only call that refuses input is one that receives a bound which is not a finite number.

Because a bad value falls back and does not stop the program, your default is part of the contract. Pick a default that is safe to run with, and check the chapter's pitfalls for the cases where a default hides a mistake.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Settings with defaults for a service

A service must start on a laptop with nothing configured and in production with everything set. This program reads the region as text, the port as an integer, and the debug flag as a yes-or-no value. It sets two of the variables itself, so the output does not depend on the machine. The port is left unset.

```cpp
// A service reads its settings from the environment at startup. A variable that is not set gets a
// sensible default, so the service runs the same way on a laptop and in production.
// This program sets two of the variables itself, so the output is the same on every machine.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_REGION", "eu-west-2", 1);
  setenv("PLUSS_GUIDE_DEBUG", "on", 1);
  unsetenv("PLUSS_GUIDE_PORT");

  const std::string region = env::get_or("PLUSS_GUIDE_REGION", "us-east-1");
  const int port = env::get_int("PLUSS_GUIDE_PORT", 8080);
  const bool debug = env::get_bool("PLUSS_GUIDE_DEBUG", false);

  std::cout << "region: " << region << '\n';
  std::cout << "port: " << port << '\n';
  std::cout << "debug: " << std::boolalpha << debug << '\n';
  return 0;
}
```

Output

```text
region: eu-west-2
port: 8080
debug: true
```

The region comes from the variable, because it was set. The port takes its default of 8080, because the variable was not set. The debug flag reads on as true. In production the shell sets these variables before the program starts, and the program's code does not change.

## Example 2: Limits that keep a value safe

An operator can set a thread count of 64 on a machine that should run sixteen workers, and a negative timeout by mistake. The clamped readers keep each value inside a range the program can run with. This program reads a thread count and a timeout, and it shows the fallback path and the handling of bounds.

```cpp
// A worker pool reads its thread count and its request timeout. Operators can set absurd values by
// accident, so the readers clamp each value into a safe range instead of trusting it.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_THREADS", "64", 1);
  setenv("PLUSS_GUIDE_TIMEOUT", "-2.5", 1);

  const int threads = env::get_int_clamped("PLUSS_GUIDE_THREADS", 4, 1, 16);
  const double timeout = env::get_double_clamped("PLUSS_GUIDE_TIMEOUT", 30.0, 0.5, 120.0);
  std::cout << "threads: " << threads << '\n';
  std::cout << "timeout seconds: " << timeout << '\n';

  // A variable that is not set takes the fallback, and the clamp applies to the fallback as well.
  unsetenv("PLUSS_GUIDE_RETRIES");
  std::cout << "retries when unset: " << env::get_int_clamped("PLUSS_GUIDE_RETRIES", 5, 0, 3) << '\n';

  // Bounds given in the wrong order are swapped, so the range is still the one between them.
  std::cout << "timeout with reversed bounds: " << env::get_double_clamped("PLUSS_GUIDE_TIMEOUT", 30.0, 120.0, 0.5) << '\n';

  // A bound that is not finite is a programming error, and the reader refuses it at the call.
  try {
    (void)env::get_double_clamped("PLUSS_GUIDE_TIMEOUT", 30.0, 0.5, std::numeric_limits<double>::infinity());
  } catch (const std::invalid_argument&) {
    std::cout << "an infinite bound is refused at the call\n";
  }
  return 0;
}
```

Output

```text
threads: 16
timeout seconds: 0.5
retries when unset: 3
timeout with reversed bounds: 0.5
an infinite bound is refused at the call
```

The thread count of 64 is clamped to the top of its range, sixteen. The negative timeout is clamped to the bottom of its range, half a second. When the variable is not set, the fallback of five is clamped too, to three, so a fallback outside the range never leaks through. Bounds given in the wrong order are swapped, and a bound that is not finite is refused at the call.

## Example 3: Checking a secret without reading it

A deployment check needs to know whether a secret is configured, and it must never print the secret. The optional form of the string reader answers presence, and the value is used only where the program needs it. This program checks an API key that is set and an SMTP password that is not, and it checks an empty name.

```cpp
// A deployment check needs to know whether a secret is configured, without ever printing it. The
// optional form of get answers presence, and the value is read only where the program needs it.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_API_KEY", "not-printed-anywhere", 1);
  unsetenv("PLUSS_GUIDE_SMTP_PASSWORD");

  std::cout << std::boolalpha;
  const auto api_key = env::get("PLUSS_GUIDE_API_KEY");
  const auto smtp_password = env::get("PLUSS_GUIDE_SMTP_PASSWORD");
  std::cout << "api key configured: " << api_key.has_value() << '\n';
  std::cout << "smtp password configured: " << smtp_password.has_value() << '\n';

  // An empty name is never a variable, so the check reports it as absent.
  std::cout << "empty name configured: " << env::get("").has_value() << '\n';
  return 0;
}
```

Output

```text
api key configured: true
smtp password configured: false
empty name configured: false
```

The API key reads as configured and the password as not configured. The value of the key is never printed. An empty name is never a variable, so it reads as not configured.

## Example 4: Bad values fall back instead of stopping the program

Operators make small typing mistakes, such as a unit after a number or a word where a flag belongs. This program shows each mistake and what the typed readers return for it, along with a value that has surrounding spaces, which is accepted.

```cpp
// Operators make typing mistakes, such as a unit after a number or a word where a flag belongs. The
// typed readers never crash on those. They fall back to the value the program gave them.
#include <pluss/tils/env.h>

#include <cstdlib>
#include <iostream>

namespace env = pluss::tils::env;

int main() {
  setenv("PLUSS_GUIDE_CACHE_MB", "256", 1);
  std::cout << "valid integer: " << env::get_int("PLUSS_GUIDE_CACHE_MB", 64) << '\n';

  setenv("PLUSS_GUIDE_CACHE_MB", "256mb", 1);
  std::cout << "integer with a unit: " << env::get_int("PLUSS_GUIDE_CACHE_MB", 64) << '\n';

  setenv("PLUSS_GUIDE_VERBOSE", "perhaps", 1);
  std::cout << "unknown flag word: " << std::boolalpha << env::get_bool("PLUSS_GUIDE_VERBOSE", false) << '\n';

  setenv("PLUSS_GUIDE_RATE", "nan", 1);
  std::cout << "non-finite double: " << env::get_double("PLUSS_GUIDE_RATE", 0.25) << '\n';

  setenv("PLUSS_GUIDE_RATE", "  0.75  ", 1);
  std::cout << "padded double: " << env::get_double("PLUSS_GUIDE_RATE", 0.25) << '\n';
  return 0;
}
```

Output

```text
valid integer: 256
integer with a unit: 64
unknown flag word: false
non-finite double: 0.25
padded double: 0.75
```

A clean integer is read as 256. A value with a unit, 256mb, is not an integer, so the fallback of 64 is returned. The word perhaps is not a flag, so the flag falls back to false. A non-finite number falls back to 0.25. Surrounding spaces are removed, so the padded 0.75 is read as 0.75.

## Decisions

| You want | Use | Notes |
|---|---|---|
| A setting that may be absent, and has a sensible default | `get_or` | The default is returned when the variable is not set. |
| A value you need only to test for | `get` | The optional form tells you whether the variable is set, without a default to misuse. |
| A number, with the fallback when the text is bad | `get_int` or `get_double` | A bad token falls back and does not throw. |
| A flag, on or off | `get_bool` | The words yes, on, 1, and true are on; no, off, 0, and false are off. |
| A number that must stay in range | `get_int_clamped` or `get_double_clamped` | The fallback is clamped as well as the parsed value. |

## Pitfalls

**Expecting a bad value to stop the program.** e04 shows a unit after a number, a stray word, and a non-finite value each fall back to the default.

**Expecting a default to escape the clamp.** e02 shows the fallback of five clamped to three, so choose a fallback inside the range you want.

**Printing a secret to check that it is set.** e03 shows the presence check, which reports true or false without the value.

**Expecting reversed clamp bounds to be refused.** e02 shows reversed bounds are swapped, so the range between them is used.

## Where next

- [Environment reference](../api/core/environment.md) -- the reference pages for each reader.
- [Writing output](tils-output.md) -- print the settings you read, with print options.
