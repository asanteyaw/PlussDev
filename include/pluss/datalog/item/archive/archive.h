// // This is equivalent to a DataStore on the table side. It is a vector/list of Streams, thus we say an Archive of Streams!
// // The method that converts a DataStore to an Archive is archivize(). This is a 3D entry.

// #pragma once
// #include <string>
// #include <vector>
// #include "item/record.h"

// namespace pluss::item {

// class Archive {
// public:
//     Archive() = default;

//     void insert(const Record& record);

//     static Archive load(const std::string& format, const std::string& path);
//     static Archive parse(const std::string& format, const std::string& raw);

//     void save(const std::string& format, const std::string& path) const;
//     std::string dump(const std::string& format) const;

//     std::vector<Record>::const_iterator begin() const;
//     std::vector<Record>::const_iterator end() const;

// private:
//     std::vector<Record> records_;
// };

// }  // namespace pluss::entry