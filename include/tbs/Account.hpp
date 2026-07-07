#ifndef TBS_ACCOUNT_HPP
#define TBS_ACCOUNT_HPP

#include <string>
#include <vector>
#include "tbs/Types.hpp"
#include "tbs/Money.hpp"

namespace tbs {

struct Subscriber {
    std::string subscriberNo;
    std::string planId;
    Money       openingBalance;  // prepaid only; ignored for postpaid
};

struct Account {
    std::string             ban;   // Billing Account Number
    std::string             name;
    AccountMode             mode = AccountMode::Postpaid;
    std::vector<Subscriber> subscribers;
};

} // namespace tbs

#endif // TBS_ACCOUNT_HPP
