#include "tbs/io/CsvReader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace tbs {

std::vector<std::string> CsvReader::parseLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur.push_back('"');
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(cur);
                cur.clear();
            } else if (c == '\r') {
                // ignore CR (Windows line endings)
            } else {
                cur.push_back(c);
            }
        }
    }
    fields.push_back(cur);

    // Trim surrounding whitespace on each field.
    for (std::string& f : fields) {
        std::size_t b = f.find_first_not_of(" \t");
        std::size_t e = f.find_last_not_of(" \t");
        if (b == std::string::npos) {
            f.clear();
        } else {
            f = f.substr(b, e - b + 1);
        }
    }
    return fields;
}

std::vector<std::vector<std::string>> CsvReader::read(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("csv: cannot open file '" + path + "'");
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(in, line)) {
        // skip fully blank lines
        if (line.find_first_not_of(" \t\r") == std::string::npos) continue;
        rows.push_back(parseLine(line));
    }
    return rows;
}

} // namespace tbs
