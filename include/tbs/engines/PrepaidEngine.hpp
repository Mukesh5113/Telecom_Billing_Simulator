#ifndef TBS_ENGINES_PREPAIDENGINE_HPP
#define TBS_ENGINES_PREPAIDENGINE_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "tbs/Dataset.hpp"
#include "tbs/Money.hpp"

namespace tbs {

// One entry in a prepaid subscriber's balance history.
struct PrepaidEvent {
    std::string time;
    std::string kind;         // TOPUP / VOICE / SMS / DATA
    std::string description;
    Money       amount;       // credit (topup) or debit cost (usage)
    Money       balanceAfter;
    bool        rejected = false;  // usage rejected for insufficient balance
};

// Per-subscriber prepaid simulation result.
struct PrepaidResult {
    std::string               ban;
    std::string               subscriberNo;
    Money                     startBalance;
    Money                     endBalance;
    std::vector<PrepaidEvent> events;
    int                       rejectedCount = 0;
};

// Real-time prepaid charging: processes CDRs and top-ups in time order,
// depleting each subscriber's wallet and flagging insufficient balance.
class PrepaidEngine {
public:
    explicit PrepaidEngine(const Dataset& dataset);

    std::vector<PrepaidResult> run() const;

private:
    const Dataset& ds_;
    std::unordered_map<std::string, std::vector<const Cdr*>>   cdrsBySub_;
    std::unordered_map<std::string, std::vector<const Topup*>> topupsBySub_;

    PrepaidResult simulateSubscriber(const std::string& ban,
                                     const Subscriber&  sub) const;
};

} // namespace tbs

#endif // TBS_ENGINES_PREPAIDENGINE_HPP
