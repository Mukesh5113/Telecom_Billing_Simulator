#ifndef TBS_INVOICE_HPP
#define TBS_INVOICE_HPP

#include <string>
#include <vector>
#include "tbs/Charge.hpp"
#include "tbs/Money.hpp"

namespace tbs {

// Aggregated bill for one account over one cycle.
struct Invoice {
    std::string         ban;
    std::string         name;
    std::vector<Charge> charges;   // every line item, in pipeline order

    Money usageTotal;
    Money recurringTotal;
    Money feeTotal;
    Money discountTotal;   // negative or zero
    Money taxTotal;
    Money grandTotal;

    void recomputeTotals() {
        usageTotal = recurringTotal = feeTotal = Money(0);
        discountTotal = taxTotal = Money(0);
        for (const Charge& c : charges) {
            switch (c.type) {
                case ChargeType::Usage:     usageTotal     += c.amount; break;
                case ChargeType::Recurring: recurringTotal += c.amount; break;
                case ChargeType::Fee:       feeTotal       += c.amount; break;
                case ChargeType::Discount:  discountTotal  += c.amount; break;
                case ChargeType::Tax:       taxTotal       += c.amount; break;
            }
        }
        grandTotal = usageTotal + recurringTotal + feeTotal
                   + discountTotal + taxTotal;
    }
};

} // namespace tbs

#endif // TBS_INVOICE_HPP
