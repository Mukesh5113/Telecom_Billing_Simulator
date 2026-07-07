#ifndef TBS_IO_JSONLOADER_HPP
#define TBS_IO_JSONLOADER_HPP

#include <string>
#include "tbs/Dataset.hpp"

namespace tbs {

// Loads the catalog (rate plans, tax rules, discount rules) from JSON files
// using RapidJSON (via the tbs::json wrapper).
class JsonLoader {
public:
    static std::vector<RatePlan>     loadPlans(const std::string& path);
    static std::vector<TaxRule>      loadTaxes(const std::string& path);
    static std::vector<DiscountRule> loadDiscounts(const std::string& path);

    static Catalog loadCatalog(const std::string& plansPath,
                               const std::string& taxesPath,
                               const std::string& discountsPath);
};

} // namespace tbs

#endif // TBS_IO_JSONLOADER_HPP
