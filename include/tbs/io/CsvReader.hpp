#ifndef TBS_IO_CSVREADER_HPP
#define TBS_IO_CSVREADER_HPP

#include <string>
#include <vector>

namespace tbs {

// Minimal CSV reader supporting quoted fields and escaped quotes ("").
// Row 0 is the header. Blank lines are skipped.
class CsvReader {
public:
    // Throws std::runtime_error if the file cannot be opened.
    static std::vector<std::vector<std::string>> read(const std::string& path);

    // Parse a single CSV line into fields (handles quotes/commas).
    static std::vector<std::string> parseLine(const std::string& line);
};

} // namespace tbs

#endif // TBS_IO_CSVREADER_HPP
