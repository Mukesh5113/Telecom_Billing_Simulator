#ifndef TBS_ENGINES_BILLINGENGINE_HPP
#define TBS_ENGINES_BILLINGENGINE_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "tbs/Dataset.hpp"
#include "tbs/Invoice.hpp"

namespace tbs {

// Orchestrates the postpaid billing pipeline:
//   rating -> recurring/fees -> discounts -> taxes -> invoice aggregation.
class BillingEngine {
public:
    explicit BillingEngine(const Dataset& dataset);

    // Bill a single account (works for any mode; totals reflect the pipeline).
    Invoice billAccount(const Account& account) const;

    // Bill every POSTPAID account in the dataset.
    std::vector<Invoice> billAll() const;

private:
    const Dataset& ds_;
    std::unordered_map<std::string, std::vector<const Cdr*>> cdrsBySub_;

    std::vector<const Cdr*> cdrsFor(const std::string& subscriberNo) const;
};

} // namespace tbs

#endif // TBS_ENGINES_BILLINGENGINE_HPP
