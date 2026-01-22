// #include "item/record.h"

// namespace pluss::item {

// // Constructor
// Record::Record(const std::unordered_map<std::string, Field>& fields)
//     : fields_(fields) {}

// // Set a field
// void Record::set(const std::string& key, const Field& value) {
//     fields_[key] = value;
// }

// // Get a field (const reference)
// const Field& Record::get(const std::string& key) const {
//     auto it = fields_.find(key);
//     if (it == fields_.end()) {
//         throw std::out_of_range("Field not found: " + key);
//     }
//     return it->second;
// }

// // Check if field exists
// bool Record::contains(const std::string& key) const {
//     return fields_.find(key) != fields_.end();
// }

// // Get all keys
// std::vector<std::string> Record::keys() const {
//     std::vector<std::string> result;
//     result.reserve(fields_.size());
//     for (const auto& [key, _] : fields_) {
//         result.push_back(key);
//     }
//     return result;
// }

// // Get the full map
// const std::unordered_map<std::string, Field>& Record::fields() const {
//     return fields_;
// }

// }  // namespace pluss::entry