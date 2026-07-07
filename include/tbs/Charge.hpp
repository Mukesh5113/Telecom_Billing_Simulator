#ifndef TBS_CHARGE_HPP
#define TBS_CHARGE_HPP

#include <string>
#include "tbs/Types.hpp"
#include "tbs/Money.hpp"

namespace tbs {

// A single billable line item. Every engine emits Charges; the invoice is
// simply an aggregation of them. Mirrors the spirit of the SAMSON t_chg_t
// record (type + amount + taxable + tax category).
struct Charge {
    std::string subscriberNo;   // empty => account-level charge
    ChargeType  type = ChargeType::Usage;
    std::string description;
    Money       amount;
    bool        taxable = false;
    std::string taxCategory = "STD";

    Charge() = default;
    Charge(std::string sub, ChargeType t, std::string desc,
           Money amt, bool tax, std::string cat)
        : subscriberNo(std::move(sub)), type(t), description(std::move(desc)),
          amount(amt), taxable(tax), taxCategory(std::move(cat)) {}
};

} // namespace tbs

#endif // TBS_CHARGE_HPP
