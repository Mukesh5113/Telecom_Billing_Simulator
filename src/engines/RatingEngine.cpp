#include "tbs/engines/RatingEngine.hpp"

#include <string>

namespace tbs {

std::vector<Charge> RatingEngine::rateSubscriber(
    const Subscriber&              subscriber,
    const RatePlan&                plan,
    const std::vector<const Cdr*>& cdrs) {

    std::int64_t voiceSec = 0;
    std::int64_t smsCount = 0;
    std::int64_t dataKb   = 0;

    for (const Cdr* c : cdrs) {
        switch (c->type) {
            case ServiceType::Voice: voiceSec += c->quantity; break;
            case ServiceType::Sms:   smsCount += c->quantity; break;
            case ServiceType::Data:  dataKb   += c->quantity; break;
            default: break;
        }
    }

    std::vector<Charge> out;
    const std::string& sub = subscriber.subscriberNo;

    // --- Voice overage ---
    std::int64_t overVoiceSec = voiceSec - plan.allowance.voiceSeconds;
    if (overVoiceSec > 0) {
        std::int64_t overMin = secondsToMinutes(overVoiceSec);
        Money amt = plan.overage.perVoiceMinute.timesInt(overMin);
        if (!amt.isZero()) {
            out.emplace_back(sub, ChargeType::Usage,
                             "Voice overage: " + std::to_string(overMin) + " min",
                             amt, true, plan.taxCategory);
        }
    }

    // --- SMS overage ---
    std::int64_t overSms = smsCount - plan.allowance.sms;
    if (overSms > 0) {
        Money amt = plan.overage.perSms.timesInt(overSms);
        if (!amt.isZero()) {
            out.emplace_back(sub, ChargeType::Usage,
                             "SMS overage: " + std::to_string(overSms) + " msg",
                             amt, true, plan.taxCategory);
        }
    }

    // --- Data overage ---
    std::int64_t overKb = dataKb - plan.allowance.dataKb;
    if (overKb > 0) {
        std::int64_t overMb = kbToMb(overKb);
        Money amt = plan.overage.perMb.timesInt(overMb);
        if (!amt.isZero()) {
            out.emplace_back(sub, ChargeType::Usage,
                             "Data overage: " + std::to_string(overMb) + " MB",
                             amt, true, plan.taxCategory);
        }
    }

    return out;
}

Money RatingEngine::costOfCdr(const RatePlan& plan, const Cdr& cdr) {
    switch (cdr.type) {
        case ServiceType::Voice:
            return plan.overage.perVoiceMinute.timesInt(secondsToMinutes(cdr.quantity));
        case ServiceType::Sms:
            return plan.overage.perSms.timesInt(cdr.quantity);
        case ServiceType::Data:
            return plan.overage.perMb.timesInt(kbToMb(cdr.quantity));
        default:
            return Money(0);
    }
}

} // namespace tbs
