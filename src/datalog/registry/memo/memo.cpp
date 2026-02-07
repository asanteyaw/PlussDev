#include <pluss/datalog/registry/memo/memo.h>
#include <pluss/SeriesBody.h>
#include <pluss/Note.h>
#include <pluss/NoteOps.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <cctype>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <regex>

namespace pluss::registry {

// ========== Constructors ==========

Memo::Memo() : value_(std::monostate{}), field_name_("") {}

Memo::Memo(bool val) : value_(val), field_name_("") {}

Memo::Memo(int val) : value_(static_cast<int64_t>(val)), field_name_("") {}

Memo::Memo(int64_t val) : value_(val), field_name_("") {}

Memo::Memo(double val) : value_(val), field_name_("") {}

Memo::Memo(const char* val) : value_(std::string(val)), field_name_("") {}

Memo::Memo(const std::string& val) : value_(val), field_name_("") {}

Memo::Memo(std::string&& val) : value_(std::move(val)), field_name_("") {}

Memo::Memo(const Chord& val) : value_(val), field_name_("") {}

Memo::Memo(const std::vector<bool>& val) : value_(val), field_name_("") {}

Memo::Memo(const std::vector<int64_t>& val) : value_(val), field_name_("") {}

Memo::Memo(const std::vector<double>& val) : value_(val), field_name_("") {}

Memo::Memo(const std::vector<std::string>& val) : value_(val), field_name_("") {}

Memo::Memo(const Series& val) : value_(std::make_shared<Series>(val)), field_name_("") {}

Memo::Memo(const Note& val) : value_(std::make_shared<Note>(val)), field_name_("") {}

Memo::Memo(const DocumentValue& val) 
    : value_(std::make_shared<DocumentValue>(val)), field_name_("") {}

Memo::Memo(const std::shared_ptr<DocumentValue>& val) 
    : value_(val), field_name_("") {}

// ========== Type Checking ==========

bool Memo::is_null() const noexcept {
    return std::holds_alternative<std::monostate>(value_);
}

bool Memo::is_bool() const noexcept {
    return std::holds_alternative<bool>(value_);
}

bool Memo::is_int() const noexcept {
    return std::holds_alternative<int64_t>(value_);
}

bool Memo::is_double() const noexcept {
    return std::holds_alternative<double>(value_);
}

bool Memo::is_string() const noexcept {
    return std::holds_alternative<std::string>(value_);
}

bool Memo::is_chord() const noexcept {
    return std::holds_alternative<Chord>(value_);
}

bool Memo::is_scalar() const noexcept {
    return is_null() || is_bool() || is_int() || is_double() || is_string() || is_chord();
}

bool Memo::is_array() const noexcept {
    return std::holds_alternative<std::vector<int64_t>>(value_) ||
           std::holds_alternative<std::vector<double>>(value_) ||
           std::holds_alternative<std::vector<std::string>>(value_) ||
           std::holds_alternative<std::vector<bool>>(value_) ||
           std::holds_alternative<std::shared_ptr<Series>>(value_) ||
           std::holds_alternative<std::shared_ptr<Note>>(value_);
}

bool Memo::is_document() const noexcept {
    return std::holds_alternative<std::shared_ptr<DocumentValue>>(value_);
}

bool Memo::is_series() const noexcept {
    return std::holds_alternative<std::shared_ptr<Series>>(value_);
}

bool Memo::is_note() const noexcept {
    return std::holds_alternative<std::shared_ptr<Note>>(value_);
}

bool Memo::is_numeric() const noexcept {
    return is_int() || is_double();
}

bool Memo::is_text() const noexcept {
    return is_string() || is_chord();
}

// ========== Type Extraction ==========

bool Memo::as_bool() const {
    if (is_bool()) return std::get<bool>(value_);
    if (is_int()) return std::get<int64_t>(value_) != 0;
    if (is_double()) return std::get<double>(value_) != 0.0;
    throw std::runtime_error("Memo: cannot convert to bool");
}

int64_t Memo::as_int() const {
    if (is_int()) return std::get<int64_t>(value_);
    if (is_double()) return static_cast<int64_t>(std::get<double>(value_));
    if (is_bool()) return std::get<bool>(value_) ? 1 : 0;
    throw std::runtime_error("Memo: cannot convert to int");
}

double Memo::as_double() const {
    if (is_double()) return std::get<double>(value_);
    if (is_int()) return static_cast<double>(std::get<int64_t>(value_));
    if (is_bool()) return std::get<bool>(value_) ? 1.0 : 0.0;
    throw std::runtime_error("Memo: cannot convert to double");
}

std::string Memo::as_string() const {
    if (is_string()) return std::get<std::string>(value_);
    if (is_chord()) return std::get<Chord>(value_).str();
    if (is_int()) return std::to_string(std::get<int64_t>(value_));
    if (is_double()) return std::to_string(std::get<double>(value_));
    if (is_bool()) return std::get<bool>(value_) ? "true" : "false";
    if (is_null()) return "null";
    throw std::runtime_error("Memo: cannot convert to string");
}

Series Memo::as_series() const {
    if (!is_series()) throw std::runtime_error("Memo: not a Series");
    return *std::get<std::shared_ptr<Series>>(value_);
}

Note Memo::as_note() const {
    if (!is_note()) throw std::runtime_error("Memo: not a Note");
    return *std::get<std::shared_ptr<Note>>(value_);
}

DocumentValue Memo::as_document() const {
    if (!is_document()) throw std::runtime_error("Memo: not a document");
    return *std::get<std::shared_ptr<DocumentValue>>(value_);
}

// Safe get methods with defaults
bool Memo::get_bool(bool default_val) const noexcept {
    try { return as_bool(); } catch (...) { return default_val; }
}

int64_t Memo::get_int(int64_t default_val) const noexcept {
    try { return as_int(); } catch (...) { return default_val; }
}

double Memo::get_double(double default_val) const noexcept {
    try { return as_double(); } catch (...) { return default_val; }
}

std::string Memo::get_string(const std::string& default_val) const noexcept {
    try { return as_string(); } catch (...) { return default_val; }
}

// ========== Document Operations ==========

Memo Memo::operator[](const std::string& key) const {
    return get_field(key);
}

Memo Memo::get_field(const std::string& key) const {
    if (!is_document()) return Memo::null();
    const auto& doc = as_document();
    auto it = doc.find(key);
    if (it == doc.end()) return Memo::null();
    return it->second;
}

Memo Memo::get_nested(const std::string& path) const {
    // Support dot notation: "user.address.city"
    size_t pos = path.find('.');
    if (pos == std::string::npos) {
        return get_field(path);
    }
    
    std::string first = path.substr(0, pos);
    std::string rest = path.substr(pos + 1);
    Memo child = get_field(first);
    if (child.is_null()) return Memo::null();
    return child.get_nested(rest);
}

Memo Memo::set_field(const std::string& key, const Memo& value) const {
    DocumentValue doc;
    if (is_document()) {
        doc = as_document();
    }
    doc[key] = value;
    return Memo(doc);
}

Memo Memo::set_nested(const std::string& path, const Memo& value) const {
    size_t pos = path.find('.');
    if (pos == std::string::npos) {
        return set_field(path, value);
    }
    
    std::string first = path.substr(0, pos);
    std::string rest = path.substr(pos + 1);
    
    Memo child = get_field(first);
    if (!child.is_document()) {
        child = Memo::document({});
    }
    
    Memo updated_child = child.set_nested(rest, value);
    return set_field(first, updated_child);
}

Memo Memo::unset_field(const std::string& key) const {
    if (!is_document()) return *this;
    DocumentValue doc = as_document();
    doc.erase(key);
    return Memo(doc);
}

Memo Memo::rename_field(const std::string& old_key, const std::string& new_key) const {
    if (!is_document()) return *this;
    DocumentValue doc = as_document();
    auto it = doc.find(old_key);
    if (it == doc.end()) return *this;
    
    Memo value = it->second;
    doc.erase(old_key);
    doc[new_key] = value;
}

// ========== MongoDB Update Operators ==========

Memo Memo::$set(const std::string& field, const Memo& value) const {
    return set_field(field, value);
}

Memo Memo::$unset(const std::string& field) const {
    return unset_field(field);
}

Memo Memo::$rename(const std::string& old_field, const std::string& new_field) const {
    return rename_field(old_field, new_field);
}

Memo Memo::$inc(const std::string& field, double amount) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    Memo updated = field_value.is_int() 
        ? Memo(field_value.as_int() + static_cast<int64_t>(amount))
        : Memo(field_value.as_double() + amount);
    return set_field(field, updated);
}

Memo Memo::$mul(const std::string& field, double factor) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    Memo updated = field_value.is_int() 
        ? Memo(field_value.as_int() * static_cast<int64_t>(factor))
        : Memo(field_value.as_double() * factor);
    return set_field(field, updated);
}

Memo Memo::$min(const std::string& field, double value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    double current = field_value.as_double();
    if (current <= value) return *this;
    return set_field(field, Memo(value));
}

Memo Memo::$max(const std::string& field, double value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_numeric()) return *this;
    double current = field_value.as_double();
    if (current >= value) return *this;
    return set_field(field, Memo(value));
}

Memo Memo::$push(const std::string& field, const Memo& value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    
    if (field_value.is_array()) {
        if (std::holds_alternative<std::vector<int64_t>>(field_value.value_) && value.is_int()) {
            auto vec = std::get<std::vector<int64_t>>(field_value.value_);
            vec.push_back(value.as_int());
            return set_field(field, Memo(vec));
        }
        if (std::holds_alternative<std::vector<double>>(field_value.value_) && value.is_numeric()) {
            auto vec = std::get<std::vector<double>>(field_value.value_);
            vec.push_back(value.as_double());
            return set_field(field, Memo(vec));
        }
        if (std::holds_alternative<std::vector<std::string>>(field_value.value_) && value.is_text()) {
            auto vec = std::get<std::vector<std::string>>(field_value.value_);
            vec.push_back(value.as_string());
            return set_field(field, Memo(vec));
        }
    }
    // If field doesn't exist or not array, create new array with value
    if (value.is_int()) return set_field(field, Memo(std::vector<int64_t>{value.as_int()}));
    if (value.is_double()) return set_field(field, Memo(std::vector<double>{value.as_double()}));
    if (value.is_text()) return set_field(field, Memo(std::vector<std::string>{value.as_string()}));
    return *this;
}

Memo Memo::$pop(const std::string& field, int position) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_array()) return *this;
    
    bool from_front = (position < 0);
    
    if (std::holds_alternative<std::vector<int64_t>>(field_value.value_)) {
        auto vec = std::get<std::vector<int64_t>>(field_value.value_);
        if (vec.empty()) return *this;
        if (from_front) vec.erase(vec.begin());
        else vec.pop_back();
        return set_field(field, Memo(vec));
    }
    if (std::holds_alternative<std::vector<double>>(field_value.value_)) {
        auto vec = std::get<std::vector<double>>(field_value.value_);
        if (vec.empty()) return *this;
        if (from_front) vec.erase(vec.begin());
        else vec.pop_back();
        return set_field(field, Memo(vec));
    }
    if (std::holds_alternative<std::vector<std::string>>(field_value.value_)) {
        auto vec = std::get<std::vector<std::string>>(field_value.value_);
        if (vec.empty()) return *this;
        if (from_front) vec.erase(vec.begin());
        else vec.pop_back();
        return set_field(field, Memo(vec));
    }
    return *this;
}

Memo Memo::$pull(const std::string& field, const Memo& value) const {
    if (!is_document()) return *this;
    Memo field_value = get_field(field);
    if (!field_value.is_array()) return *this;
    
    if (std::holds_alternative<std::vector<int64_t>>(field_value.value_) && value.is_int()) {
        auto vec = std::get<std::vector<int64_t>>(field_value.value_);
        int64_t val = value.as_int();
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
        return set_field(field, Memo(vec));
    }
    if (std::holds_alternative<std::vector<double>>(field_value.value_) && value.is_numeric()) {
        auto vec = std::get<std::vector<double>>(field_value.value_);
        double val = value.as_double();
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
        return set_field(field, Memo(vec));
    }
    if (std::holds_alternative<std::vector<std::string>>(value_) && value.is_text()) {
        auto vec = std::get<std::vector<std::string>>(value_);
        std::string val = value.as_string();
        vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end());
        return Memo(vec);
    }
    return *this;
}

// ========== MongoDB Query Operators ==========

bool Memo::$eq(const Memo& other) const {
    return *this == other;
}

bool Memo::$ne(const Memo& other) const {
    return !(*this == other);
}

bool Memo::$gt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return as_double() > other.as_double();
    }
    if (is_text() && other.is_text()) {
        return as_string() > other.as_string();
    }
    return false;
}

bool Memo::$gte(const Memo& other) const {
    return $gt(other) || $eq(other);
}

bool Memo::$lt(const Memo& other) const {
    if (is_numeric() && other.is_numeric()) {
        return as_double() < other.as_double();
    }
    if (is_text() && other.is_text()) {
        return as_string() < other.as_string();
    }
    return false;
}

bool Memo::$lte(const Memo& other) const {
    return $lt(other) || $eq(other);
}

bool Memo::$in(const std::vector<Memo>& values) const {
    for (const auto& val : values) {
        if ($eq(val)) return true;
    }
    return false;
}

bool Memo::$nin(const std::vector<Memo>& values) const {
    return !$in(values);
}

bool Memo::$exists(const std::string& field) const {
    if (!is_document()) return false;
    return has_field(field);
}

bool Memo::$regex(const std::string& pattern) const {
    if (!is_text()) return false;
    try {
        std::regex re(pattern);
        return std::regex_search(as_string(), re);
    } catch (...) {
        return false;
    }
}

// ========== Array Operations ==========

int64_t Memo::size() const {
    if (is_scalar()) return 1;
    if (std::holds_alternative<std::vector<int64_t>>(value_))
        return static_cast<int64_t>(std::get<std::vector<int64_t>>(value_).size());
    if (std::holds_alternative<std::vector<double>>(value_))
        return static_cast<int64_t>(std::get<std::vector<double>>(value_).size());
    if (std::holds_alternative<std::vector<std::string>>(value_))
        return static_cast<int64_t>(std::get<std::vector<std::string>>(value_).size());
    if (std::holds_alternative<std::vector<bool>>(value_))
        return static_cast<int64_t>(std::get<std::vector<bool>>(value_).size());
    if (is_series()) return std::get<std::shared_ptr<Series>>(value_)->numel();
    if (is_note()) return std::get<std::shared_ptr<Note>>(value_)->shape()[0];
    if (is_document()) return static_cast<int64_t>(as_document().size());
    return 0;
}

Memo Memo::at(int64_t index) const {
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        if (index < 0) index += vec.size();
        if (index < 0 || index >= static_cast<int64_t>(vec.size())) 
            throw std::out_of_range("Memo: index out of range");
        return Memo(vec[index]);
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        if (index < 0) index += vec.size();
        if (index < 0 || index >= static_cast<int64_t>(vec.size())) 
            throw std::out_of_range("Memo: index out of range");
        return Memo(vec[index]);
    }
    if (std::holds_alternative<std::vector<std::string>>(value_)) {
        const auto& vec = std::get<std::vector<std::string>>(value_);
        if (index < 0) index += vec.size();
        if (index < 0 || index >= static_cast<int64_t>(vec.size())) 
            throw std::out_of_range("Memo: index out of range");
        return Memo(vec[index]);
    }
    throw std::runtime_error("Memo: at() requires array type");
}

bool Memo::contains(const Memo& value) const {
    if (!is_array()) return false;
    
    if (std::holds_alternative<std::vector<int64_t>>(value_) && value.is_int()) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        return std::find(vec.begin(), vec.end(), value.as_int()) != vec.end();
    }
    if (std::holds_alternative<std::vector<double>>(value_) && value.is_numeric()) {
        const auto& vec = std::get<std::vector<double>>(value_);
        return std::find(vec.begin(), vec.end(), value.as_double()) != vec.end();
    }
    if (std::holds_alternative<std::vector<std::string>>(value_) && value.is_text()) {
        const auto& vec = std::get<std::vector<std::string>>(value_);
        return std::find(vec.begin(), vec.end(), value.as_string()) != vec.end();
    }
    return false;
}

// ========== Aggregations ==========

Memo Memo::sum() const {
    if (is_numeric()) return *this;
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        return Memo(std::accumulate(vec.begin(), vec.end(), 0.0));
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        return Memo(std::accumulate(vec.begin(), vec.end(), 0.0));
    }
    if (is_series()) {
        auto s = *std::get<std::shared_ptr<Series>>(value_);
        return Memo(s.sum().item<double>());
    }
    return Memo(0.0);
}

Memo Memo::mean() const {
    int64_t n = size();
    if (n == 0) return Memo(0.0);
    double sum_val = sum().as_double();
    return Memo(sum_val / static_cast<double>(n));
}

Memo Memo::min() const {
    if (is_numeric()) return *this;
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        if (vec.empty()) return Memo(0.0);
        return Memo(static_cast<double>(*std::min_element(vec.begin(), vec.end())));
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        if (vec.empty()) return Memo(0.0);
        return Memo(*std::min_element(vec.begin(), vec.end()));
    }
    if (is_series()) {
        auto s = *std::get<std::shared_ptr<Series>>(value_);
        return Memo(s.min().item<double>());
    }
    return Memo(0.0);
}

Memo Memo::max() const {
    if (is_numeric()) return *this;
    
    if (std::holds_alternative<std::vector<int64_t>>(value_)) {
        const auto& vec = std::get<std::vector<int64_t>>(value_);
        if (vec.empty()) return Memo(0.0);
        return Memo(static_cast<double>(*std::max_element(vec.begin(), vec.end())));
    }
    if (std::holds_alternative<std::vector<double>>(value_)) {
        const auto& vec = std::get<std::vector<double>>(value_);
        if (vec.empty()) return Memo(0.0);
        return Memo(*std::max_element(vec.begin(), vec.end()));
    }
    if (is_series()) {
        auto s = *std::get<std::shared_ptr<Series>>(value_);
        return Memo(s.max().item<double>());
    }
    return Memo(0.0);
}

Memo Memo::count() const {
    return Memo(static_cast<int64_t>(size()));
}

// ========== String Operations ==========

Memo Memo::to_upper() const {
    if (!is_text()) throw std::runtime_error("to_upper requires text type");
    std::string str = as_string();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return Memo(str);
}

Memo Memo::to_lower() const {
    if (!is_text()) throw std::runtime_error("to_lower requires text type");
    std::string str = as_string();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return Memo(str);
}

Memo Memo::trim() const {
    if (!is_text()) throw std::runtime_error("trim requires text type");
    std::string str = as_string();
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), str.end());
    return Memo(str);
}

Memo Memo::concat(const Memo& other) const {
    if (!is_text() || !other.is_text()) throw std::runtime_error("concat requires text types");
    return Memo(as_string() + other.as_string());
}

Memo Memo::substr(int64_t start, int64_t length) const {
    if (!is_text()) throw std::runtime_error("substr requires text type");
    std::string str = as_string();
    if (start < 0) start = 0;
    if (start >= static_cast<int64_t>(str.size())) return Memo("");
    if (length < 0) length = str.size() - start;
    return Memo(str.substr(start, length));
}

bool Memo::starts_with(const std::string& prefix) const {
    if (!is_text()) return false;
    std::string str = as_string();
    return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
}

bool Memo::ends_with(const std::string& suffix) const {
    if (!is_text()) return false;
    std::string str = as_string();
    return str.size() >= suffix.size() && 
           str.substr(str.size() - suffix.size()) == suffix;
}

bool Memo::str_contains(const std::string& substr) const {
    if (!is_text()) return false;
    return as_string().find(substr) != std::string::npos;
}

// ========== Conversions ==========

Memo Memo::to_int() const {
    return Memo(as_int());
}

Memo Memo::to_double() const {
    return Memo(as_double());
}

Memo Memo::to_string_memo() const {
    return Memo(as_string());
}

Memo Memo::to_bool_memo() const {
    return Memo(as_bool());
}

// ========== Serialization ==========

std::string Memo::to_json() const {
    std::ostringstream oss;
    
    if (is_null()) {
        oss << "null";
    } else if (is_bool()) {
        oss << (as_bool() ? "true" : "false");
    } else if (is_int()) {
        oss << as_int();
    } else if (is_double()) {
        oss << std::setprecision(15) << as_double();
    } else if (is_string()) {
        oss << "\"" << as_string() << "\"";  // TODO: escape special chars
    } else if (is_array()) {
        oss << "[";
        int64_t n = size();
        for (int64_t i = 0; i < n; ++i) {
            if (i > 0) oss << ",";
            oss << at(i).to_json();
        }
        oss << "]";
    } else if (is_document()) {
        oss << "{";
        const auto& doc = as_document();
        bool first = true;
        for (const auto& [key, val] : doc) {
            if (!first) oss << ",";
            oss << "\"" << key << "\":" << val.to_json();
            first = false;
        }
        oss << "}";
    } else {
        oss << "null";
    }
    
    return oss.str();
}

std::string Memo::to_string() const {
    if (is_text()) return as_string();
    return to_json();
}

void Memo::display() const {
    std::cout << to_json() << std::endl;
}

void Memo::info() const {
    std::cout << "Memo: " << (field_name_.empty() ? "<unnamed>" : field_name_) << "\n";
    std::cout << "Type: ";
    if (is_null()) std::cout << "null";
    else if (is_bool()) std::cout << "bool";
    else if (is_int()) std::cout << "int64";
    else if (is_double()) std::cout << "double";
    else if (is_string()) std::cout << "string";
    else if (is_chord()) std::cout << "Chord";
    else if (is_array()) std::cout << "array[" << size() << "]";
    else if (is_series()) std::cout << "Series[" << size() << "]";
    else if (is_note()) std::cout << "Note[" << size() << "]";
    else if (is_document()) std::cout << "document{" << size() << " fields}";
    std::cout << "\n";
    
    if (!is_null() && size() < 100) {
        std::cout << "Value: " << to_json() << "\n";
    }
}

// ========== Static Factories ==========

Memo Memo::null() {
    return Memo();
}

Memo Memo::array(const std::vector<Memo>& elements) {
    // For now, convert to vector<double> if all numeric
    std::vector<double> vec;
    for (const auto& elem : elements) {
        if (elem.is_numeric()) {
            vec.push_back(elem.as_double());
        }
    }
    return Memo(vec);
}

Memo Memo::document(const DocumentValue& doc) {
    return Memo(doc);
}

// ========== Operators ==========

bool Memo::operator==(const Memo& other) const {
    if (value_.index() != other.value_.index()) return false;
    
    if (is_null()) return true;
    if (is_bool()) return as_bool() == other.as_bool();
    if (is_int()) return as_int() == other.as_int();
    if (is_double()) return std::abs(as_double() - other.as_double()) < 1e-10;
    if (is_string()) return as_string() == other.as_string();
    if (is_chord()) return std::get<Chord>(value_).str() == std::get<Chord>(other.value_).str();
    
    // Arrays and documents require deeper comparison
    if (is_array()) {
        if (size() != other.size()) return false;
        for (int64_t i = 0; i < size(); ++i) {
            if (at(i) != other.at(i)) return false;
        }
        return true;
    }
    
    if (is_document()) {
        const auto& doc1 = as_document();
        const auto& doc2 = other.as_document();
        if (doc1.size() != doc2.size()) return false;
        for (const auto& [key, val] : doc1) {
            auto it = doc2.find(key);
            if (it == doc2.end() || it->second != val) return false;
        }
        return true;
    }
    
    return false;
}

bool Memo::operator!=(const Memo& other) const {
    return !(*this == other);
}

bool Memo::operator<(const Memo& other) const {
    return $lt(other);
}

bool Memo::operator<=(const Memo& other) const {
    return $lte(other);
}

bool Memo::operator>(const Memo& other) const {
    return $gt(other);
}

bool Memo::operator>=(const Memo& other) const {
    return $gte(other);
}

// ========== Additional Helper Methods ==========

bool Memo::has_field(const std::string& field) const {
    if (!is_document()) return false;
    const auto& doc = as_document();
    return doc.find(field) != doc.end();
}

} // namespace pluss::registry