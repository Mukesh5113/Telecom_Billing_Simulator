#ifndef TBS_ENGINES_RATINGENGINE_HPP
#define TBS_ENGINES_RATINGENGINE_HPP

#include <vector>
#include "tbs/Account.hpp"
#include "tbs/RatePlan.hpp"
#include "tbs/Cdr.hpp"
#include "tbs/Charge.hpp"

namespace tbs {

// Prices metered usage. For postpaid it pools a subscriber's CDRs over the
// cycle, deducts plan allowances, and charges overage. For prepaid it prices
// a single CDR pay-as-you-go (no allowance).
class RatingEngine {
public:
    // Postpaid: one USAGE charge per service type that incurs overage.
    static std::vector<Charge> rateSubscriber(
        const Subscriber&              subscriber,
        const RatePlan&                plan,
        const std::vector<const Cdr*>& cdrs);

    // Prepaid: full pay-as-you-go cost of a single CDR.
    static Money costOfCdr(const RatePlan& plan, const Cdr& cdr);

    // Unit conversions (rounded up to the next whole unit).
    static std::int64_t secondsToMinutes(std::int64_t seconds) {
        return (seconds + 59) / 60;
    }
    static std::int64_t kbToMb(std::int64_t kb) {
        return (kb + 1023) / 1024;
    }
};

} // namespace tbs

#endif // TBS_ENGINES_RATINGENGINE_HPP
