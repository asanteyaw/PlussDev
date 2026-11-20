// An append-only event log whose records carry text, timestamps and durations in one structure: append events one at a time,
// then ask which happened last, how long jobs ran and what failed.
#include <pluss/pluss.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Value = Slot::Value;

static Registry append(const Registry &log, const std::string &job, const std::string &status, const char *when, std::int64_t seconds) {
  // push adds a value to the end of one field: a record is the same position in every field.
  return log.push("job", Value(pluss::make_note({job}))).push("status", Value(pluss::make_note({status}))).push("when", Value(pluss::make_timeline({when}))).push("took", Value(pluss::duration::seconds(std::vector<std::int64_t>{seconds})));
}

int main() {
  Registry log({{"job", Slot(std::vector<Value>{})}, {"status", Slot(std::vector<Value>{})}, {"when", Slot(std::vector<Value>{})}, {"took", Slot(std::vector<Value>{})}});
  log = append(log, "backup", "ok", "2026-11-13T01:00:00Z", 340);
  log = append(log, "report", "ok", "2026-11-13T06:30:00Z", 95);
  log = append(log, "sync", "failed", "2026-11-13T09:15:00Z", 12);
  log = append(log, "backup", "ok", "2026-11-14T01:00:00Z", 362);
  log = append(log, "sync", "ok", "2026-11-14T09:15:00Z", 48);
  std::cout << "records " << log.sizes().at("job") << " in fields " << log.nfields() << "\n";

  // 1. The latest event is the last element of every field.
  const auto latest = log.tail(1);
  std::cout << "latest job: " << std::get<pluss::Note>(latest.get("job")->values().front()).at(0) << " (" << std::get<pluss::Note>(latest.get("status")->values().front()).at(0) << ")\n";

  // 2. What failed? Walk the status field; each cell is a one-element Note.
  const auto status = log.get("status")->values();
  const auto jobs = log.get("job")->values();
  for (std::size_t i = 0; i < status.size(); ++i)
    if (std::get<pluss::Note>(status[i]).at(0) == "failed") std::cout << "failed: record " << i << ", job " << std::get<pluss::Note>(jobs[i]).at(0) << "\n";

  // 3. How long did each job take? Durations are elapsed lengths: compare them and add them.
  const auto took = log.get("took")->values();
  double total_seconds = 0;
  for (const auto &cell : took) total_seconds += static_cast<double>(std::get<pluss::Duration>(cell).host()[0]) / 1e9;
  std::cout << "total run time " << total_seconds << " s over " << took.size() << " runs, mean " << total_seconds / took.size() << " s\n";

  // 4. Runs and failures per calendar day: the timestamps are Timeline cells, so format each one and count.
  std::map<std::string, std::pair<int, int>> per_day;   // day -> (runs, failures)
  const auto when = log.get("when")->values();
  for (std::size_t i = 0; i < when.size(); ++i) {
    auto &entry = per_day[std::string(std::get<pluss::Timeline>(when[i]).strftime("%Y-%m-%d").at(0))];
    ++entry.first;
    if (std::get<pluss::Note>(status[i]).at(0) == "failed") ++entry.second;
  }
  for (const auto &[day, counts] : per_day) std::cout << day << ": " << counts.first << " runs, " << counts.second << " failed\n";
}
