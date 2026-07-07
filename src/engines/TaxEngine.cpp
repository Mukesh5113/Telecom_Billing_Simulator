#include "tbs/engines/TaxEngine.hpp"

namespace tbs {

std::vector<Charge> TaxEngine::computeTaxes(const std::vector<TaxRule>& rules,
                                            const std::vector<Charge>&  charges) {
    std::vector<Charge> out;

    for (const TaxRule& rule : rules) {
        Money base(0);
        for (const Charge& c : charges) {
            if (!c.taxable) continue;
            if (rule.category == "*" || rule.category == c.taxCategory) {
                base += c.amount;
            }
        }
        if (base.isZero()) continue;

        Money tax = base.scaleBasisPoints(rule.rateBasisPoints);
        if (tax.isZero()) continue;

        out.emplace_back(std::string(), ChargeType::Tax, rule.name,
                         tax, false, rule.category);
    }

    return out;
}

} // namespace tbs
