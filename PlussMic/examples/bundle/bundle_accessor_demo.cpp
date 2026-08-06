#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pluss.h>

int main() {
  using pluss::bundle::Axel;
  using pluss::bundle::Registry;
  using pluss::bundle::Slot;
  using pluss::bundle::SlotKind;
  using pluss::codem::ScalarType;
  using pluss::table::ParseInvalid;
  using pluss::table::TextAlign;
  using pluss::table::TextOptions;
  using pluss::table::ThreadCastOptions;
  using pluss::table::TimeUnit;
  using pluss::table::TimelineLocalizeOptions;

  try {
    const Slot numeric_text({
        pluss::make_note("10.5"),
        pluss::make_note("20.0"),
        pluss::make_note("30.25"),
        std::monostate{},
    });
    const Slot codes({
        pluss::make_note("  alpha-01  "),
        pluss::make_note("BETA-22"),
        pluss::make_note("Gamma-333"),
        pluss::make_note("delta|44"),
    });
    const Slot tags({
        pluss::make_note("red|blue"),
        pluss::make_note("blue|green"),
        pluss::make_note("red|green"),
        std::monostate{},
    });
    const Slot date_text({
        pluss::make_note("2024-01-31"),
        pluss::make_note("2024-02-29"),
        pluss::make_note("2024-03-15"),
        std::monostate{},
    });
    const Slot dt_text({
        pluss::make_note("2024-01-31 08:15:00"),
        pluss::make_note("2024-02-29 16:45:30"),
        pluss::make_note("2024-03-15 10:05:45"),
        pluss::make_note("2024-04-20 21:30:15"),
    });
    const Slot timeline_slot({
        pluss::make_timeline({"2024-01-31T08:15:00Z"}),
        pluss::make_timeline({"2024-02-29T16:45:30Z"}),
        pluss::make_timeline({"2024-03-15T10:05:45Z"}),
        std::monostate{},
    });
    const Slot timeindex_slot({
        pluss::make_timeindex({"2024-01-31T08:15:00Z"}),
        pluss::make_timeindex({"2024-02-29T16:45:30Z"}),
        pluss::make_timeindex({"2024-03-15T10:05:45Z"}),
        std::monostate{},
    });
    const Slot amount({
        pluss::make_series(10.5, ScalarType::Float64),
        pluss::make_series(20.0, ScalarType::Float64),
        std::monostate{},
        pluss::make_series(40.25, ScalarType::Float64),
    });
    const Slot quantity({
        pluss::make_series(std::int64_t{1}, ScalarType::Int64),
        pluss::make_series(std::int64_t{2}, ScalarType::Int64),
        pluss::make_series(std::int64_t{2}, ScalarType::Int64),
        pluss::make_series(std::int64_t{4}, ScalarType::Int64),
    });
    const Slot null_slot({
        std::monostate{},
        std::monostate{},
        std::monostate{},
        std::monostate{},
    });
    const Slot empty_slot(std::vector<Slot::Value>{});
    const Slot mixed_slot({
        pluss::make_series(std::int64_t{7}, ScalarType::Int64),
        pluss::make_note("mixed"),
        pluss::make_timeline({"2024-05-01T00:00:00Z"}),
    });

    const Slot naive_timeline = dt_text.cast({
        .format = "%Y-%m-%d %H:%M:%S",
        .invalid = ParseInvalid::Null,
    }).timeline();

    std::cout << "\n=== Slot Cast Accessor ===\n";
    std::cout << "cast().series()\n" << numeric_text.cast({.series_dtype = ScalarType::Float64}).series() << "\n";
    std::cout << "cast().note()\n" << amount.cast().note() << "\n";
    std::cout << "cast().timeline()\n" << date_text.cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeline() << "\n";
    std::cout << "cast().timeindex()\n" << date_text.cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeindex() << "\n";

    std::cout << "\n=== Slot Note Accessor ===\n";
    std::cout << "nt().strip()\n" << codes.nt().strip() << "\n";
    std::cout << "nt().strip(\" -\")\n" << codes.nt().strip(" -") << "\n";
    std::cout << "nt().lstrip()\n" << codes.nt().lstrip() << "\n";
    std::cout << "nt().lstrip(\" AB\")\n" << codes.nt().lstrip(" AB") << "\n";
    std::cout << "nt().rstrip()\n" << codes.nt().rstrip() << "\n";
    std::cout << "nt().rstrip(\" 34\")\n" << codes.nt().rstrip(" 34") << "\n";
    std::cout << "nt().lower()\n" << codes.nt().lower() << "\n";
    std::cout << "nt().upper()\n" << codes.nt().upper() << "\n";
    std::cout << "nt().casefold()\n" << codes.nt().casefold() << "\n";
    std::cout << "nt().titlecase()\n" << codes.nt().titlecase() << "\n";
    std::cout << "nt().length()\n" << codes.nt().length() << "\n";
    std::cout << "nt().starts(\"  a\")\n" << codes.nt().starts("  a") << "\n";
    std::cout << "nt().ends(\"22\")\n" << codes.nt().ends("22") << "\n";
    std::cout << "nt().has(\"gamma\", {.case_sensitive = false})\n"
              << codes.nt().has("gamma", {.case_sensitive = false}) << "\n";
    std::cout << "nt().count(\"[0-9]\", {.regex = true})\n"
              << codes.nt().count("[0-9]", {.regex = true}) << "\n";
    std::cout << "nt().matches(\"[A-Za-z -]+\")\n" << codes.nt().matches("[A-Za-z -]+") << "\n";
    std::cout << "nt().fullmatch(\"[A-Za-z| -]+\")\n" << codes.nt().fullmatch("[A-Za-z| -]+") << "\n";
    std::cout << "nt().findall(\"[0-9]+\", {.regex = true})\n"
              << codes.nt().findall("[0-9]+", {.regex = true}) << "\n";
    std::cout << "nt().findall(\"-\")\n" << codes.nt().findall("-") << "\n";
    std::cout << "nt().split(\"-\", 1, \"piece\")\n" << codes.nt().split("-", 1, "piece") << "\n";
    std::cout << "nt().split(\"[|-]\", {.regex = true, .prefix = \"token\"})\n"
              << codes.nt().split("[|-]", {.regex = true, .prefix = "token"}) << "\n";
    std::cout << "nt().partition(\"-\", \"part\")\n" << codes.nt().partition("-", "part") << "\n";
    std::cout << "nt().pad(14, TextAlign::Right, '.')\n" << codes.nt().pad(14, TextAlign::Right, '.') << "\n";
    std::cout << "nt().chars(0, 5)\n" << codes.nt().chars(0, 5) << "\n";
    std::cout << "nt().normalize()\n" << codes.nt().normalize() << "\n";
    std::cout << "nt().encode(\"hex\")\n" << codes.nt().encode("hex") << "\n";
    std::cout << "nt().indicate(\"|\", \"tag_\")\n" << tags.nt().indicate("|", "tag_") << "\n";
    std::cout << "nt().replace(\"-\", \"_\")\n" << codes.nt().replace("-", "_") << "\n";
    std::cout << "nt().replace(\"[0-9]+\", \"NUM\", {.regex = true})\n"
              << codes.nt().replace("[0-9]+", "NUM", {.regex = true}) << "\n";

    std::cout << "\n=== Slot Timeline Accessor ===\n";
    std::cout << "tl().isnat()\n" << timeline_slot.tl().isnat() << "\n";
    std::cout << "tl().valid()\n" << timeline_slot.tl().valid() << "\n";
    std::cout << "tl().year()\n" << timeline_slot.tl().year() << "\n";
    std::cout << "tl().month()\n" << timeline_slot.tl().month() << "\n";
    std::cout << "tl().day()\n" << timeline_slot.tl().day() << "\n";
    std::cout << "tl().hour()\n" << timeline_slot.tl().hour() << "\n";
    std::cout << "tl().minute()\n" << timeline_slot.tl().minute() << "\n";
    std::cout << "tl().second()\n" << timeline_slot.tl().second() << "\n";
    std::cout << "tl().weekday()\n" << timeline_slot.tl().weekday() << "\n";
    std::cout << "tl().yearday()\n" << timeline_slot.tl().yearday() << "\n";
    std::cout << "tl().quarter()\n" << timeline_slot.tl().quarter() << "\n";
    std::cout << "tl().week()\n" << timeline_slot.tl().week() << "\n";
    std::cout << "tl().weekend()\n" << timeline_slot.tl().weekend() << "\n";
    std::cout << "tl().month_start()\n" << timeline_slot.tl().month_start() << "\n";
    std::cout << "tl().month_end()\n" << timeline_slot.tl().month_end() << "\n";
    std::cout << "tl().leap()\n" << timeline_slot.tl().leap() << "\n";
    std::cout << "tl().weekday_name()\n" << timeline_slot.tl().weekday_name() << "\n";
    std::cout << "tl().month_name()\n" << timeline_slot.tl().month_name() << "\n";
    std::cout << "tl().fmt(\"%Y/%m/%d %H:%M\")\n" << timeline_slot.tl().fmt("%Y/%m/%d %H:%M") << "\n";
    std::cout << "tl().iso()\n" << timeline_slot.tl().iso() << "\n";
    std::cout << "tl().epoch(TimeUnit::Seconds)\n" << timeline_slot.tl().epoch(TimeUnit::Seconds) << "\n";
    std::cout << "tl().floor(\"D\")\n" << timeline_slot.tl().floor("D") << "\n";
    std::cout << "tl().ceil(\"H\")\n" << timeline_slot.tl().ceil("H") << "\n";
    std::cout << "tl().round(\"H\")\n" << timeline_slot.tl().round("H") << "\n";
    std::cout << "tl().date()\n" << timeline_slot.tl().date() << "\n";
    std::cout << "tl().time()\n" << timeline_slot.tl().time() << "\n";
    std::cout << "tl().shift(2, \"days\")\n" << timeline_slot.tl().shift(2, "days") << "\n";
    std::cout << "tl().period(\"months\")\n" << timeline_slot.tl().period("months") << "\n";
    std::cout << "tl().tz(\"America/New_York\")\n" << timeline_slot.tl().tz("America/New_York") << "\n";
    std::cout << "tl().localize({.tz = \"UTC\"})\n"
              << naive_timeline.tl().localize(TimelineLocalizeOptions{.tz = "UTC"}) << "\n";
    std::cout << "tl().utc()\n" << timeline_slot.tl().tz("America/New_York").tl().utc() << "\n";
    std::cout << "tl().index(false)\n" << timeline_slot.tl().index(false) << "\n";

    const Registry registry({
        {"amount", amount},
        {"quantity", quantity},
        {"numeric_text", numeric_text},
        {"code", codes},
        {"tag", tags},
        {"date_text", date_text},
        {"opened_at", timeline_slot},
        {"stamp", timeindex_slot},
        {"all_null", null_slot},
        {"empty_slot", empty_slot},
        {"mixed", mixed_slot},
    });

    const Registry registry_b({
        {"amount", Slot({
            pluss::make_series(11.0, ScalarType::Float64),
            std::monostate{},
            pluss::make_series(31.5, ScalarType::Float64),
            pluss::make_series(45.0, ScalarType::Float64),
        })},
        {"quantity", Slot({
            pluss::make_series(std::int64_t{10}, ScalarType::Int64),
            pluss::make_series(std::int64_t{20}, ScalarType::Int64),
            pluss::make_series(std::int64_t{20}, ScalarType::Int64),
            pluss::make_series(std::int64_t{40}, ScalarType::Int64),
        })},
        {"numeric_text", Slot({
            pluss::make_note("100.5"),
            pluss::make_note("200.0"),
            pluss::make_note("300.25"),
            std::monostate{},
        })},
        {"code", Slot({
            pluss::make_note("  omega-10  "),
            pluss::make_note("SIGMA-77"),
            pluss::make_note("theta-303"),
            pluss::make_note("lambda|55"),
        })},
        {"tag", Slot({
            pluss::make_note("gold|blue"),
            pluss::make_note("green|black"),
            pluss::make_note("gold|white"),
            std::monostate{},
        })},
        {"date_text", Slot({
            pluss::make_note("2025-01-31"),
            pluss::make_note("2025-02-28"),
            pluss::make_note("2025-03-31"),
            std::monostate{},
        })},
        {"opened_at", Slot({
            pluss::make_timeline({"2025-01-31T08:15:00Z"}),
            pluss::make_timeline({"2025-02-28T16:45:30Z"}),
            pluss::make_timeline({"2025-03-31T10:05:45Z"}),
            std::monostate{},
        })},
        {"stamp", Slot({
            pluss::make_timeindex({"2025-01-31T08:15:00Z"}),
            pluss::make_timeindex({"2025-02-28T16:45:30Z"}),
            pluss::make_timeindex({"2025-03-31T10:05:45Z"}),
            std::monostate{},
        })},
        {"all_null", null_slot},
        {"empty_slot", empty_slot},
        {"mixed", Slot({
            pluss::make_note("mixed"),
            pluss::make_series(std::int64_t{9}, ScalarType::Int64),
            pluss::make_timeindex({"2025-04-01T00:00:00Z"}),
        })},
    });

    std::cout << "\n=== Registry Fields ===\n";
    std::cout << "fields(...).cast().series()\n"
              << registry.fields({"numeric_text"}).cast({.series_dtype = ScalarType::Float64}).series() << "\n";
    std::cout << "fields(...).cast().note()\n"
              << registry.fields({"amount", "quantity"}).cast().note() << "\n";
    std::cout << "fields(...).cast().timeline()\n"
              << registry.fields({"date_text"}).cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeline() << "\n";
    std::cout << "fields(...).cast().timeindex()\n"
              << registry.fields({"date_text"}).cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeindex() << "\n";
    std::cout << "fields(...).nt()\n"
              << registry.fields({"code", "tag"}).nt().strip().fields({"code", "tag"}).nt().upper() << "\n";
    std::cout << "fields(...).tl()\n" << registry.fields({"opened_at"}).tl().year() << "\n";
    std::cout << "fields(...).fillnull(0.0)\n" << registry.fields({"amount"}).fillnull(0.0) << "\n";
    std::cout << "fields(...).fillnull(\"missing\")\n" << registry.fields({"code"}).fillnull("missing") << "\n";
    std::cout << "fields(...).fillnull(Timeline)\n"
              << registry.fields({"opened_at"}).fillnull(pluss::make_timeline({"2024-12-31T00:00:00Z"})) << "\n";
    std::cout << "fields(...).fillnull(TimeIndex)\n"
              << registry.fields({"stamp"}).fillnull(pluss::make_timeindex({"2024-12-31T00:00:00Z"})) << "\n";
    std::cout << "fields(...).dropnull()\n" << registry.fields({"amount", "code"}).dropnull() << "\n";
    std::cout << "fields(...).isnull()\n" << registry.fields({"amount"}).isnull() << "\n";
    std::cout << "fields(...).notnull()\n" << registry.fields({"amount"}).notnull() << "\n";
    std::cout << "fields(...).invalid()\n" << registry.fields({"date_text"}).invalid() << "\n";
    std::cout << "fields(...).distinct()\n" << registry.fields({"quantity"}).distinct({.dropnull = false}) << "\n";
    std::cout << "fields(...).freq()\n" << registry.fields({"quantity"}).freq({.top = 3, .dropnull = false}) << "\n";
    std::cout << "fields(...).ndistinct()\n" << registry.fields({"quantity"}).ndistinct({.dropnull = false}) << "\n";
    std::cout << "fields(...).cast(Float64).series()\n"
              << registry.fields({"numeric_text"}).cast({.series_dtype = ScalarType::Float64}).series() << "\n";

    std::cout << "\n=== Registry Field-Axis Accessor ===\n";
    std::cout << "fieldaxis().like(\"date\")\n" << registry.fieldaxis().like("date") << "\n";
    std::cout << "fieldaxis().match(\"^a\")\n" << registry.fieldaxis().match("^a") << "\n";
    std::cout << "fieldaxis().prefix(\"st\")\n" << registry.fieldaxis().prefix("st") << "\n";
    std::cout << "fieldaxis().suffix(\"text\")\n" << registry.fieldaxis().suffix("text") << "\n";
    std::cout << "fieldaxis().kind(SlotKind::Mixed)\n" << registry.fieldaxis().kind(SlotKind::Mixed) << "\n";
    std::cout << "fieldaxis().series()\n" << registry.fieldaxis().series() << "\n";
    std::cout << "fieldaxis().note()\n" << registry.fieldaxis().note() << "\n";
    std::cout << "fieldaxis().timeline()\n" << registry.fieldaxis().timeline() << "\n";
    std::cout << "fieldaxis().timeindex()\n" << registry.fieldaxis().timeindex() << "\n";
    std::cout << "fieldaxis().mixed()\n" << registry.fieldaxis().mixed() << "\n";
    std::cout << "fieldaxis().nulls()\n" << registry.fieldaxis().nulls() << "\n";
    std::cout << "fieldaxis().empty()\n" << registry.fieldaxis().empty() << "\n";

    std::cout << "\n=== Registry Note Accessor ===\n";
    std::cout << "nt().strip()\n" << registry.fields({"code"}).nt().strip() << "\n";
    std::cout << "nt().strip(\" -\")\n" << registry.fields({"code"}).nt().strip(" -") << "\n";
    std::cout << "nt().lstrip()\n" << registry.fields({"code"}).nt().lstrip() << "\n";
    std::cout << "nt().lstrip(\" O\")\n" << registry.fields({"code"}).nt().lstrip(" O") << "\n";
    std::cout << "nt().rstrip()\n" << registry.fields({"code"}).nt().rstrip() << "\n";
    std::cout << "nt().rstrip(\" 34\")\n" << registry.fields({"code"}).nt().rstrip(" 34") << "\n";
    std::cout << "nt().lower()\n" << registry.fields({"code"}).nt().lower() << "\n";
    std::cout << "nt().upper()\n" << registry.fields({"code"}).nt().upper() << "\n";
    std::cout << "nt().casefold()\n" << registry.fields({"code"}).nt().casefold() << "\n";
    std::cout << "nt().titlecase()\n" << registry.fields({"code"}).nt().titlecase() << "\n";
    std::cout << "nt().length()\n" << registry.fields({"code"}).nt().length() << "\n";
    std::cout << "nt().starts(\"  \")\n" << registry.fields({"code"}).nt().starts("  ") << "\n";
    std::cout << "nt().ends(\"55\")\n" << registry_b.fields({"code"}).nt().ends("55") << "\n";
    std::cout << "nt().has(\"theta\", {.case_sensitive = false})\n"
              << registry_b.fields({"code"}).nt().has("theta", {.case_sensitive = false}) << "\n";
    std::cout << "nt().count(\"[0-9]\", {.regex = true})\n"
              << registry.fields({"code"}).nt().count("[0-9]", {.regex = true}) << "\n";
    std::cout << "nt().matches(\"[A-Za-z -]+\")\n"
              << registry.fields({"code"}).nt().matches("[A-Za-z -]+") << "\n";
    std::cout << "nt().fullmatch(\"[A-Za-z| -]+\")\n"
              << registry.fields({"code"}).nt().fullmatch("[A-Za-z| -]+") << "\n";
    std::cout << "nt().pad(16, TextAlign::Center, '.')\n"
              << registry.fields({"code"}).nt().pad(16, TextAlign::Center, '.') << "\n";
    std::cout << "nt().chars(0, 5)\n" << registry.fields({"code"}).nt().chars(0, 5) << "\n";
    std::cout << "nt().normalize()\n" << registry.fields({"code"}).nt().normalize() << "\n";
    std::cout << "nt().encode(\"url\")\n" << registry.fields({"code"}).nt().encode("url") << "\n";
    std::cout << "nt().replace(\"-\", \"_\")\n" << registry.fields({"code"}).nt().replace("-", "_") << "\n";
    std::cout << "nt().replace(\"[0-9]+\", \"NUM\", {.regex = true})\n"
              << registry.fields({"code"}).nt().replace("[0-9]+", "NUM", {.regex = true}) << "\n";

    std::cout << "\n=== Registry Timeline Accessor ===\n";
    std::cout << "tl().isnat()\n" << registry.fields({"opened_at"}).tl().isnat() << "\n";
    std::cout << "tl().valid()\n" << registry.fields({"opened_at"}).tl().valid() << "\n";
    std::cout << "tl().year()\n" << registry.fields({"opened_at"}).tl().year() << "\n";
    std::cout << "tl().month()\n" << registry.fields({"opened_at"}).tl().month() << "\n";
    std::cout << "tl().day()\n" << registry.fields({"opened_at"}).tl().day() << "\n";
    std::cout << "tl().hour()\n" << registry.fields({"opened_at"}).tl().hour() << "\n";
    std::cout << "tl().minute()\n" << registry.fields({"opened_at"}).tl().minute() << "\n";
    std::cout << "tl().second()\n" << registry.fields({"opened_at"}).tl().second() << "\n";
    std::cout << "tl().weekday()\n" << registry.fields({"opened_at"}).tl().weekday() << "\n";
    std::cout << "tl().yearday()\n" << registry.fields({"opened_at"}).tl().yearday() << "\n";
    std::cout << "tl().quarter()\n" << registry.fields({"opened_at"}).tl().quarter() << "\n";
    std::cout << "tl().week()\n" << registry.fields({"opened_at"}).tl().week() << "\n";
    std::cout << "tl().weekend()\n" << registry.fields({"opened_at"}).tl().weekend() << "\n";
    std::cout << "tl().month_start()\n" << registry.fields({"opened_at"}).tl().month_start() << "\n";
    std::cout << "tl().month_end()\n" << registry.fields({"opened_at"}).tl().month_end() << "\n";
    std::cout << "tl().leap()\n" << registry.fields({"opened_at"}).tl().leap() << "\n";
    std::cout << "tl().weekday_name()\n" << registry.fields({"opened_at"}).tl().weekday_name() << "\n";
    std::cout << "tl().month_name()\n" << registry.fields({"opened_at"}).tl().month_name() << "\n";
    std::cout << "tl().fmt(\"%d %b %Y\")\n" << registry.fields({"opened_at"}).tl().fmt("%d %b %Y") << "\n";
    std::cout << "tl().iso()\n" << registry.fields({"opened_at"}).tl().iso() << "\n";
    std::cout << "tl().epoch(TimeUnit::Days)\n" << registry.fields({"opened_at"}).tl().epoch(TimeUnit::Days) << "\n";
    std::cout << "tl().floor(\"D\")\n" << registry.fields({"opened_at"}).tl().floor("D") << "\n";
    std::cout << "tl().ceil(\"H\")\n" << registry.fields({"opened_at"}).tl().ceil("H") << "\n";
    std::cout << "tl().round(\"H\")\n" << registry.fields({"opened_at"}).tl().round("H") << "\n";
    std::cout << "tl().date()\n" << registry.fields({"opened_at"}).tl().date() << "\n";
    std::cout << "tl().time()\n" << registry.fields({"opened_at"}).tl().time() << "\n";
    std::cout << "tl().shift(1, \"months\")\n" << registry.fields({"opened_at"}).tl().shift(1, "months") << "\n";
    std::cout << "tl().period(\"days\")\n" << registry.fields({"opened_at"}).tl().period("days") << "\n";
    std::cout << "tl().tz(\"America/Edmonton\")\n" << registry.fields({"opened_at"}).tl().tz("America/Edmonton") << "\n";
    std::cout << "tl().localize({.tz = \"UTC\"})\n"
              << registry.fields({"date_text"}).cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeline()
                     .fields({"date_text"}).tl().localize(TimelineLocalizeOptions{.tz = "UTC"}) << "\n";
    std::cout << "tl().utc()\n"
              << registry.fields({"opened_at"}).tl().tz("America/New_York").fields({"opened_at"}).tl().utc() << "\n";
    std::cout << "tl().index(false)\n" << registry.fields({"opened_at"}).tl().index(false) << "\n";

    const Axel axel({
        registry,
        registry_b,
    }, Slot({
        pluss::make_note("train"),
        pluss::make_note("test"),
    }));

    std::cout << "\n=== Axel Fields ===\n";
    std::cout << "fields(...).cast().series()\n"
              << axel.fields({"numeric_text"}).cast({.series_dtype = ScalarType::Float64}).series().registry(0).fields("numeric_text") << "\n";
    std::cout << "fields(...).cast().note()\n"
              << axel.fields({"amount"}).cast().note().registry(0).fields("amount") << "\n";
    std::cout << "fields(...).cast().timeline()\n"
              << axel.fields({"date_text"}).cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeline().registry(0).fields("date_text") << "\n";
    std::cout << "fields(...).cast().timeindex()\n"
              << axel.fields({"date_text"}).cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeindex().registry(0).fields("date_text") << "\n";
    std::cout << "fields(...).nt()\n"
              << axel.fields({"code"}).nt().strip().fields({"code"}).nt().upper().registry(0).fields("code") << "\n";
    std::cout << "fields(...).tl()\n" << axel.fields({"opened_at"}).tl().year().registry(0).fields("opened_at") << "\n";
    std::cout << "fields(...).fillnull(0.0)\n" << axel.fields({"amount"}).fillnull(0.0).registry(0).fields("amount") << "\n";
    std::cout << "fields(...).fillnull(\"missing\")\n" << axel.fields({"code"}).fillnull("missing").registry(0).fields("code") << "\n";
    std::cout << "fields(...).fillnull(Timeline)\n"
              << axel.fields({"opened_at"}).fillnull(pluss::make_timeline({"2026-01-01T00:00:00Z"})).registry(0).fields("opened_at") << "\n";
    std::cout << "fields(...).fillnull(TimeIndex)\n"
              << axel.fields({"stamp"}).fillnull(pluss::make_timeindex({"2026-01-01T00:00:00Z"})).registry(0).fields("stamp") << "\n";
    std::cout << "fields(...).dropnull()\n" << axel.fields({"amount"}).dropnull().registry(0).fields("amount") << "\n";
    std::cout << "fields(...).isnull()\n" << axel.fields({"amount"}).isnull().registry(0).fields("amount") << "\n";
    std::cout << "fields(...).notnull()\n" << axel.fields({"amount"}).notnull().registry(0).fields("amount") << "\n";
    std::cout << "fields(...).invalid()\n" << axel.fields({"amount"}).invalid().registry(0).fields("amount") << "\n";
    std::cout << "fields(...).distinct()\n" << axel.fields({"quantity"}).distinct({.dropnull = false}).registry(0).fields("quantity") << "\n";
    std::cout << "fields(...).freq()\n" << axel.fields({"quantity"}).freq({.top = 2, .dropnull = false}).registry(0).fields("quantity") << "\n";
    std::cout << "fields(...).ndistinct()\n" << axel.fields({"quantity"}).ndistinct({.dropnull = false}).registry(0).fields("quantity") << "\n";
    std::cout << "fields(...).cast(Float64).series()\n"
              << axel.fields({"numeric_text"}).cast({.series_dtype = ScalarType::Float64}).series().registry(0).fields("numeric_text")
              << "\n";

    std::cout << "\n=== Axel Field-Axis Accessor ===\n";
    std::cout << "fieldaxis().like(\"date\") => " << axel.fieldaxis().like("date").registry(0).nfields() << "\n";
    std::cout << "fieldaxis().match(\"^a\") => " << axel.fieldaxis().match("^a").registry(0).nfields() << "\n";
    std::cout << "fieldaxis().prefix(\"st\") => " << axel.fieldaxis().prefix("st").registry(0).nfields() << "\n";
    std::cout << "fieldaxis().suffix(\"text\") => " << axel.fieldaxis().suffix("text").registry(0).nfields() << "\n";
    std::cout << "fieldaxis().kind(SlotKind::Mixed) => " << axel.fieldaxis().kind(SlotKind::Mixed).registry(0).nfields() << "\n";
    std::cout << "fieldaxis().series() => " << axel.fieldaxis().series().registry(0).nfields() << "\n";
    std::cout << "fieldaxis().note() => " << axel.fieldaxis().note().registry(0).nfields() << "\n";
    std::cout << "fieldaxis().timeline() => " << axel.fieldaxis().timeline().registry(0).nfields() << "\n";
    std::cout << "fieldaxis().timeindex() => " << axel.fieldaxis().timeindex().registry(0).nfields() << "\n";
    std::cout << "fieldaxis().mixed() => " << axel.fieldaxis().mixed().registry(0).nfields() << "\n";
    std::cout << "fieldaxis().nulls() => " << axel.fieldaxis().nulls().registry(0).nfields() << "\n";
    std::cout << "fieldaxis().empty() => " << axel.fieldaxis().empty().registry(0).nfields() << "\n";

    std::cout << "\n=== Axel Note Accessor ===\n";
    std::cout << "nt().strip()\n" << axel.fields({"code"}).nt().strip().registry(0).fields("code") << "\n";
    std::cout << "nt().strip(\" -\")\n" << axel.fields({"code"}).nt().strip(" -").registry(0).fields("code") << "\n";
    std::cout << "nt().lstrip()\n" << axel.fields({"code"}).nt().lstrip().registry(0).fields("code") << "\n";
    std::cout << "nt().lstrip(\" O\")\n" << axel.fields({"code"}).nt().lstrip(" O").registry(0).fields("code") << "\n";
    std::cout << "nt().rstrip()\n" << axel.fields({"code"}).nt().rstrip().registry(0).fields("code") << "\n";
    std::cout << "nt().rstrip(\" 34\")\n" << axel.fields({"code"}).nt().rstrip(" 34").registry(0).fields("code") << "\n";
    std::cout << "nt().lower()\n" << axel.fields({"code"}).nt().lower().registry(0).fields("code") << "\n";
    std::cout << "nt().upper()\n" << axel.fields({"code"}).nt().upper().registry(0).fields("code") << "\n";
    std::cout << "nt().casefold()\n" << axel.fields({"code"}).nt().casefold().registry(0).fields("code") << "\n";
    std::cout << "nt().titlecase()\n" << axel.fields({"code"}).nt().titlecase().registry(0).fields("code") << "\n";
    std::cout << "nt().length()\n" << axel.fields({"code"}).nt().length().registry(0).fields("code") << "\n";
    std::cout << "nt().starts(\"  \")\n" << axel.fields({"code"}).nt().starts("  ").registry(0).fields("code") << "\n";
    std::cout << "nt().ends(\"55\")\n" << axel.fields({"code"}).nt().ends("55").registry(0).fields("code") << "\n";
    std::cout << "nt().has(\"theta\", {.case_sensitive = false})\n"
              << axel.fields({"code"}).nt().has("theta", {.case_sensitive = false}).registry(0).fields("code") << "\n";
    std::cout << "nt().count(\"[0-9]\", {.regex = true})\n"
              << axel.fields({"code"}).nt().count("[0-9]", {.regex = true}).registry(0).fields("code") << "\n";
    std::cout << "nt().matches(\"[A-Za-z -]+\")\n" << axel.fields({"code"}).nt().matches("[A-Za-z -]+").registry(0).fields("code") << "\n";
    std::cout << "nt().fullmatch(\"[A-Za-z| -]+\")\n" << axel.fields({"code"}).nt().fullmatch("[A-Za-z| -]+").registry(0).fields("code") << "\n";
    std::cout << "nt().pad(16, TextAlign::Center, '.')\n"
              << axel.fields({"code"}).nt().pad(16, TextAlign::Center, '.').registry(0).fields("code") << "\n";
    std::cout << "nt().chars(0, 5)\n" << axel.fields({"code"}).nt().chars(0, 5).registry(0).fields("code") << "\n";
    std::cout << "nt().normalize()\n" << axel.fields({"code"}).nt().normalize().registry(0).fields("code") << "\n";
    std::cout << "nt().encode(\"base64\")\n" << axel.fields({"code"}).nt().encode("base64").registry(0).fields("code") << "\n";
    std::cout << "nt().replace(\"-\", \"_\")\n" << axel.fields({"code"}).nt().replace("-", "_").registry(0).fields("code") << "\n";
    std::cout << "nt().replace(\"[0-9]+\", \"NUM\", {.regex = true})\n"
              << axel.fields({"code"}).nt().replace("[0-9]+", "NUM", {.regex = true}).registry(0).fields("code") << "\n";

    std::cout << "\n=== Axel Timeline Accessor ===\n";
    std::cout << "tl().isnat()\n" << axel.fields({"opened_at"}).tl().isnat().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().valid()\n" << axel.fields({"opened_at"}).tl().valid().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().year()\n" << axel.fields({"opened_at"}).tl().year().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().month()\n" << axel.fields({"opened_at"}).tl().month().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().day()\n" << axel.fields({"opened_at"}).tl().day().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().hour()\n" << axel.fields({"opened_at"}).tl().hour().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().minute()\n" << axel.fields({"opened_at"}).tl().minute().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().second()\n" << axel.fields({"opened_at"}).tl().second().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().weekday()\n" << axel.fields({"opened_at"}).tl().weekday().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().yearday()\n" << axel.fields({"opened_at"}).tl().yearday().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().quarter()\n" << axel.fields({"opened_at"}).tl().quarter().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().week()\n" << axel.fields({"opened_at"}).tl().week().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().weekend()\n" << axel.fields({"opened_at"}).tl().weekend().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().month_start()\n" << axel.fields({"opened_at"}).tl().month_start().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().month_end()\n" << axel.fields({"opened_at"}).tl().month_end().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().leap()\n" << axel.fields({"opened_at"}).tl().leap().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().weekday_name()\n" << axel.fields({"opened_at"}).tl().weekday_name().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().month_name()\n" << axel.fields({"opened_at"}).tl().month_name().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().fmt(\"%Y-%m-%d\")\n" << axel.fields({"opened_at"}).tl().fmt("%Y-%m-%d").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().iso()\n" << axel.fields({"opened_at"}).tl().iso().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().epoch(TimeUnit::Millis)\n" << axel.fields({"opened_at"}).tl().epoch(TimeUnit::Millis).registry(0).fields("opened_at") << "\n";
    std::cout << "tl().floor(\"D\")\n" << axel.fields({"opened_at"}).tl().floor("D").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().ceil(\"H\")\n" << axel.fields({"opened_at"}).tl().ceil("H").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().round(\"H\")\n" << axel.fields({"opened_at"}).tl().round("H").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().date()\n" << axel.fields({"opened_at"}).tl().date().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().time()\n" << axel.fields({"opened_at"}).tl().time().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().shift(-90, \"minutes\")\n" << axel.fields({"opened_at"}).tl().shift(-90, "minutes").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().period(\"months\")\n" << axel.fields({"opened_at"}).tl().period("months").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().tz(\"America/New_York\")\n" << axel.fields({"opened_at"}).tl().tz("America/New_York").registry(0).fields("opened_at") << "\n";
    std::cout << "tl().localize({.tz = \"UTC\"})\n"
              << axel.fields({"date_text"}).cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null}).timeline()
                     .fields({"date_text"}).tl().localize(TimelineLocalizeOptions{.tz = "UTC"}).registry(0).fields("date_text") << "\n";
    std::cout << "tl().utc()\n"
              << axel.fields({"opened_at"}).tl().tz("America/New_York").fields({"opened_at"}).tl().utc().registry(0).fields("opened_at") << "\n";
    std::cout << "tl().index(false)\n" << axel.fields({"opened_at"}).tl().index(false).registry(0).fields("opened_at") << "\n";

    return 0;
  } catch (const std::exception& e) {
    std::cerr << "bundle_accessor_demo failed: " << e.what() << "\n";
    return 1;
  }
}
