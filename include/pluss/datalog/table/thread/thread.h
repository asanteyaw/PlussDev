#pragma once

#include <variant>
#include <string>
#include <pluss/Note.h>
#include <pluss/Timeline.h>
#include <pluss/Series.h>

namespace pluss::table {

// A Thread represents a DataFrame column. It holds the column name and a variant
// payload which may be a `pluss::Series`, `pluss::Note`, or `pluss::Timeline`.
class Thread {
public:
    using Value = std::variant<pluss::Series, pluss::Note, pluss::Timeline>;

    Thread() = default;
    explicit Thread(Value v, std::string name = "") : value_(std::move(v)), name_(std::move(name)) {}
    Thread(const pluss::Series& s, std::string name = "") : value_(s), name_(std::move(name)) {}
    Thread(pluss::Series&& s, std::string name = "") : value_(std::move(s)), name_(std::move(name)) {}
    Thread(const pluss::Note& n, std::string name = "") : value_(n), name_(std::move(name)) {}
    Thread(pluss::Note&& n, std::string name = "") : value_(std::move(n)), name_(std::move(name)) {}
    Thread(const pluss::Timeline& t, std::string name = "") : value_(t), name_(std::move(name)) {}
    Thread(pluss::Timeline&& t, std::string name = "") : value_(std::move(t)), name_(std::move(name)) {}

    Thread(const Thread&) = default;
    Thread(Thread&&) noexcept = default;
    Thread& operator=(const Thread&) = default;
    Thread& operator=(Thread&&) noexcept = default;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(const std::string& n) { name_ = n; }

    [[nodiscard]] bool is_series() const noexcept { return std::holds_alternative<pluss::Series>(value_); }
    [[nodiscard]] bool is_note() const noexcept { return std::holds_alternative<pluss::Note>(value_); }
    [[nodiscard]] bool is_timeline() const noexcept { return std::holds_alternative<pluss::Timeline>(value_); }

    [[nodiscard]] const pluss::Series& as_series() const { return std::get<pluss::Series>(value_); }
    [[nodiscard]] pluss::Series& as_series() { return std::get<pluss::Series>(value_); }

    [[nodiscard]] const pluss::Note& as_note() const { return std::get<pluss::Note>(value_); }
    [[nodiscard]] pluss::Note& as_note() { return std::get<pluss::Note>(value_); }

    [[nodiscard]] const pluss::Timeline& as_timeline() const { return std::get<pluss::Timeline>(value_); }
    [[nodiscard]] pluss::Timeline& as_timeline() { return std::get<pluss::Timeline>(value_); }

    // Attempt to convert the stored value to a Series. Timelines can be converted
    // via `to_Series()`. Notes are not generally convertible to numeric Series and
    // will throw.
    pluss::Series to_series() const {
        if (is_series()) return as_series();
        if (is_timeline()) return as_timeline().to_Series();
        throw std::runtime_error("Thread: cannot convert Note to Series");
    }

    // Size/shape helpers
    int64_t size() const {
        if (is_series()) return as_series().numel();
        if (is_timeline()) return as_timeline().to_Series().numel();
        // For Note, interpret the first shape dimension as size (vector-like)
        if (is_note()) {
            auto sh = as_note().shape();
            return sh.empty() ? 1 : sh[0];
        }
        return 0;
    }

    int64_t dim() const {
        if (is_series()) return as_series().dim();
        if (is_timeline()) return as_timeline().to_Series().dim();
        if (is_note()) return as_note().ndim();
        return 0;
    }

private:
    Value value_;
    std::string name_;
};

} // namespace pluss::table
// This is equivalent to an Archive on the entry side. It is a table of tables(DataFrames), thus we say a Store of DataFrames!
// The method that converts an Archive to a DataStore is storelize(). This is a 3D table.