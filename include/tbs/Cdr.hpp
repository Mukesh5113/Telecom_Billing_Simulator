#ifndef TBS_CDR_HPP
#define TBS_CDR_HPP

#include <string>
#include <cstdint>
#include "tbs/Types.hpp"
#include "tbs/Money.hpp"

namespace tbs {

// Call Detail Record - one metered usage event.
// quantity units depend on service type:
//   Voice -> seconds, Sms -> message count, Data -> kilobytes (KB).
struct Cdr {
    std::string  subscriberNo;
    ServiceType  type = ServiceType::Unknown;
    std::string  timestamp;   // ISO-8601, sortable lexicographically
    std::int64_t quantity = 0;
};

// Prepaid balance top-up event.
struct Topup {
    std::string subscriberNo;
    std::string timestamp;
    Money       amount;
};

} // namespace tbs

#endif // TBS_CDR_HPP
