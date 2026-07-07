#ifndef TBS_ENGINES_DISCOUNTENGINE_HPP
#define TBS_ENGINES_DISCOUNTENGINE_HPP

#include <vector>
#include "tbs/Account.hpp"
#include "tbs/Dataset.hpp"
#include "tbs/Charge.hpp"

namespace tbs {

// Applies discount rules, producing negative DISCOUNT line items.
// Discounts are modeled as non-taxable (they do not reduce the tax base) -
// a documented simplification; see README.
class DiscountEngine {
public:
    // existingCharges are the usage/recurring/fee charges already accrued for
    // the account (used by percent-off-recurring rules).
    static std::vector<Charge> applyDiscounts(
        const Account&             account,
        const std::vector<DiscountRule>& discounts,
        const std::vector<Charge>& existingCharges);
};

} // namespace tbs

#endif // TBS_ENGINES_DISCOUNTENGINE_HPP
