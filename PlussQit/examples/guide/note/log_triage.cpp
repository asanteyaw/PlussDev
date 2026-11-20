// Triage a server log: pick out the failing requests, pull the status code and path out of each line and count what is going wrong.
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto lines = pluss::make_note({"2026-11-13T09:00:01Z GET /api/users 200 12ms",
                                 "2026-11-13T09:00:02Z GET /api/orders 500 340ms",
                                 "2026-11-13T09:00:02Z POST /api/orders 201 45ms",
                                 "2026-11-13T09:00:04Z GET /api/users/42 404 8ms",
                                 "2026-11-13T09:00:05Z GET /api/orders 503 1200ms",
                                 "2026-11-13T09:00:07Z GET /health 200 1ms"});
  std::cout << lines.numel() << " log lines\n";

  // 1. The status code is the three-digit number between spaces; replace everything around it to isolate it.
  auto status = lines.regex().replace(R"(^.* (\d{3}) \d+ms$)", "$1");
  show("status", status);

  // 2. Server errors are the codes starting with 5. starts_with is a vectorised test over the whole column.
  auto server_error = status.starts_with("5");
  std::vector<std::int64_t> failing_rows;   // a Bool Note exposes its lanes as a span; collect the rows that are true
  std::int64_t row = 0;
  for (const auto flag : server_error.bools()) {
    if (flag) failing_rows.push_back(row);
    ++row;
  }
  show("lines with a 5xx status", lines.take(failing_rows));

  // 3. Which paths are involved? Extract the token that starts with a slash.
  auto path = lines.regex().replace(R"(^\S+ \S+ (/\S*) .*$)", "$1");
  show("paths", path);
  show("distinct paths", path.unique());

  // 4. Latency: keep the digits before "ms" and convert the column to numbers.
  auto millis = lines.regex().replace(R"(^.* (\d+)ms$)", "$1");
  show("latency text", millis);
  double slowest = 0.0;
  const auto cells = millis.host();   // keep the owning copy alive while you read from it
  for (const auto &text : cells.vector()) slowest = std::max(slowest, std::stod(text));
  std::cout << "slowest request: " << slowest << " ms\n";
}
