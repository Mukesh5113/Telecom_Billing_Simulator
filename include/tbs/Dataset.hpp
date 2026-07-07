#ifndef TBS_DATASET_HPP
#define TBS_DATASET_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include "tbs/RatePlan.hpp"
#include "tbs/TaxRule.hpp"
#include "tbs/DiscountRule.hpp"
#include "tbs/Account.hpp"
#include "tbs/Cdr.hpp"

namespace tbs {

// Reference / configuration data (the "catalog").
struct Catalog {
    std::unordered_map<std::string, RatePlan> plans;
    std::vector<TaxRule>                       taxes;
    std::vector<DiscountRule>                  discounts;

    const RatePlan* findPlan(const std::string& id) const {
        auto it = plans.find(id);
        return it == plans.end() ? nullptr : &it->second;
    }
};

// Everything a run needs: catalog + transactional inputs.
struct Dataset {
    Catalog             catalog;
    std::vector<Account> accounts;
    std::vector<Cdr>     cdrs;
    std::vector<Topup>   topups;
};

} // namespace tbs

#endif // TBS_DATASET_HPP
