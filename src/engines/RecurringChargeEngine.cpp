#include "tbs/engines/RecurringChargeEngine.hpp"

namespace tbs {

std::vector<Charge> RecurringChargeEngine::chargeSubscriber(
    const Subscriber& subscriber, const RatePlan& plan) {

    std::vector<Charge> out;
    const std::string& sub = subscriber.subscriberNo;

    if (!plan.monthlyRecurringCharge.isZero()) {
        out.emplace_back(sub, ChargeType::Recurring,
                         "Monthly plan charge (" + plan.name + ")",
                         plan.monthlyRecurringCharge, true, plan.taxCategory);
    }
    if (!plan.regulatoryFee.isZero()) {
        out.emplace_back(sub, ChargeType::Fee, "Regulatory fee",
                         plan.regulatoryFee, true, plan.taxCategory);
    }
    if (!plan.adminFee.isZero()) {
        out.emplace_back(sub, ChargeType::Fee, "Administrative fee",
                         plan.adminFee, true, plan.taxCategory);
    }
    return out;
}

} // namespace tbs
