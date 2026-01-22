// #include "item/archive.h"
// #include "item/io/loader.h"

// namespace pluss::item {

// void Archive::insert(const Record& record) {
//     records_.push_back(record);
// }

// Archive Archive::load(const std::string& format, const std::string& path) {
//     return io::load_from_file(format, path);
// }

// Archive Archive::parse(const std::string& format, const std::string& raw) {
//     return io::parse_from_string(format, raw);
// }

// void Archive::save(const std::string& format, const std::string& path) const {
//     io::save_to_file(*this, format, path);
// }

// std::string Archive::dump(const std::string& format) const {
//     return io::dump_to_string(*this, format);
// }

// std::vector<Record>::const_iterator Archive::begin() const {
//     return records_.begin();
// }

// std::vector<Record>::const_iterator Archive::end() const {
//     return records_.end();
// }

// }  // namespace pluss::entry