#include "tbs/engines/DiscountEngine.hpp"

namespace tbs {

std::vector<Charge> DiscountEngine::applyDiscounts(
    const Account&                   account,
    const std::vector<DiscountRule>& discounts,
    const std::vector<Charge>&       existingCharges) {

    std::vector<Charge> out;

    for (const DiscountRule& rule : discounts) {
        switch (rule.kind) {

            case DiscountKind::PercentOffRecurring: {
                // One credit per subscriber, based on that subscriber's
                // recurring charge(s).
                for (const Charge& c : existingCharges) {
                    if (c.type != ChargeType::Recurring) continue;
                    Money credit = c.amount.scaleBasisPoints(rule.rateBasisPoints);
                    if (credit.isZero()) continue;
                    out.emplace_back(c.subscriberNo, ChargeType::Discount,
                                     rule.name, -credit, false, "");
                }
                break;
            }

            case DiscountKind::FixedPromo: {
                if (!rule.amount.isZero()) {
                    out.emplace_back(std::string(), ChargeType::Discount,
                                     rule.name, -rule.amount, false, "");
                }
                break;
            }

            case DiscountKind::MultiLineBundle: {
                if (static_cast<int>(account.subscribers.size()) >= rule.minLines
                    && !rule.amount.isZero()) {
                    out.emplace_back(std::string(), ChargeType::Discount,
                                     rule.name + " (" +
                                         std::to_string(account.subscribers.size()) +
                                         " lines)",
                                     -rule.amount, false, "");
                }
                break;
            }
        }
    }

    return out;
}

} // namespace tbs
