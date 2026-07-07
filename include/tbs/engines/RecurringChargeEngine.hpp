#ifndef TBS_ENGINES_RECURRINGCHARGEENGINE_HPP
#define TBS_ENGINES_RECURRINGCHARGEENGINE_HPP

#include <vector>
#include "tbs/Account.hpp"
#include "tbs/RatePlan.hpp"
#include "tbs/Charge.hpp"

namespace tbs {

// Emits the monthly recurring charge plus regulatory/admin fees for a
// subscriber (REG/MNT/TAX-fee style line items).
class RecurringChargeEngine {
public:
    static std::vector<Charge> chargeSubscriber(const Subscriber& subscriber,
                                                const RatePlan&   plan);
};

} // namespace tbs

#endif // TBS_ENGINES_RECURRINGCHARGEENGINE_HPP
