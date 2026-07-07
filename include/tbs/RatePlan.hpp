#ifndef TBS_RATEPLAN_HPP
#define TBS_RATEPLAN_HPP

#include <string>
#include <cstdint>
#include "tbs/Money.hpp"

namespace tbs {

// Bundled monthly allowances (consumed before overage is charged).
struct Allowance {
    std::int64_t voiceSeconds = 0;
    std::int64_t sms          = 0;
    std::int64_t dataKb       = 0;
};

// Per-unit overage prices, applied after allowances are exhausted.
struct OverageRates {
    Money perVoiceMinute;  // charged per started minute
    Money perSms;          // charged per message
    Money perMb;           // charged per started megabyte
};

struct RatePlan {
    std::string  id;
    std::string  name;
    Money        monthlyRecurringCharge;
    Allowance    allowance;
    OverageRates overage;
    Money        regulatoryFee;   // REG-fee style line item
    Money        adminFee;        // MNT/admin-fee style line item
    std::string  taxCategory = "STD";
};

} // namespace tbs

#endif // TBS_RATEPLAN_HPP
