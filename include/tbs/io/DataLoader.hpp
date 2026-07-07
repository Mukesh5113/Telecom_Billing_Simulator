#ifndef TBS_IO_DATALOADER_HPP
#define TBS_IO_DATALOADER_HPP

#include <string>
#include <vector>
#include "tbs/Dataset.hpp"

namespace tbs {

// Loads the full Dataset from a data directory containing:
//   rate_plans.json, tax_rules.json, discount_rules.json,
//   subscribers.csv, cdrs.csv, and (optionally) topups.csv
class DataLoader {
public:
    static std::vector<Account> loadAccounts(const std::string& path);
    static std::vector<Cdr>     loadCdrs(const std::string& path);
    static std::vector<Topup>   loadTopups(const std::string& path);

    static Dataset loadFromDir(const std::string& dir);
};

} // namespace tbs

#endif // TBS_IO_DATALOADER_HPP
