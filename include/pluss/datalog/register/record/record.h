// #pragma once
// #include <string>
// #include <unordered_map>
// #include <variant>
// #include <vector>
// #include <torch/torch.h>

// namespace pluss::item {

// // The Value type supported in a Record field
// using Field = std::variant<
//     int,
//     int64_t,
//     float,
//     double,
//     bool,
//     std::string,
//     torch::Tensor
// >;

// class Record {
// public:
//     Record() = default;
//     explicit Record(const std::unordered_map<std::string, Field>& fields);

//     // Set a field
//     void set(const std::string& key, const Field& value);

//     // Get a field
//     const Field& get(const std::string& key) const;

//     // Check if a field exists
//     bool contains(const std::string& key) const;

//     // Get all keys
//     std::vector<std::string> keys() const;

//     // Get underlying field map
//     const std::unordered_map<std::string, Field>& fields() const;

// private:
//     std::unordered_map<std::string, Field> fields_;
// };

// }  // namespace pluss::entry