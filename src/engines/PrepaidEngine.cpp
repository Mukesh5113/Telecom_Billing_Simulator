#include "tbs/engines/PrepaidEngine.hpp"
#include "tbs/engines/RatingEngine.hpp"

#include <algorithm>

namespace tbs {

PrepaidEngine::PrepaidEngine(const Dataset& dataset) : ds_(dataset) {
    for (const Cdr& c : ds_.cdrs) {
        cdrsBySub_[c.subscriberNo].push_back(&c);
    }
    for (const Topup& t : ds_.topups) {
        topupsBySub_[t.subscriberNo].push_back(&t);
    }
}

namespace {
// A time-ordered event: either a top-up or a CDR.
struct TimedEvent {
    std::string   time;
    const Cdr*    cdr = nullptr;
    const Topup*  topup = nullptr;
};
} // namespace

PrepaidResult PrepaidEngine::simulateSubscriber(const std::string& ban,
                                                const Subscriber&  sub) const {
    PrepaidResult res;
    res.ban          = ban;
    res.subscriberNo = sub.subscriberNo;
    res.startBalance = sub.openingBalance;

    const RatePlan* plan = ds_.catalog.findPlan(sub.planId);

    std::vector<TimedEvent> timeline;
    auto cit = cdrsBySub_.find(sub.subscriberNo);
    if (cit != cdrsBySub_.end()) {
        for (const Cdr* c : cit->second) timeline.push_back({c->timestamp, c, nullptr});
    }
    auto tit = topupsBySub_.find(sub.subscriberNo);
    if (tit != topupsBySub_.end()) {
        for (const Topup* t : tit->second) timeline.push_back({t->timestamp, nullptr, t});
    }

    // Stable sort by ISO timestamp string (chronological for ISO-8601).
    std::stable_sort(timeline.begin(), timeline.end(),
                     [](const TimedEvent& a, const TimedEvent& b) {
                         return a.time < b.time;
                     });

    Money balance = sub.openingBalance;

    for (const TimedEvent& ev : timeline) {
        PrepaidEvent pe;
        pe.time = ev.time;

        if (ev.topup != nullptr) {
            balance += ev.topup->amount;
            pe.kind         = "TOPUP";
            pe.description  = "Balance top-up";
            pe.amount       = ev.topup->amount;
            pe.balanceAfter = balance;
        } else {
            const Cdr* c = ev.cdr;
            Money cost = (plan != nullptr) ? RatingEngine::costOfCdr(*plan, *c)
                                           : Money(0);
            pe.kind        = toString(c->type);
            pe.amount      = cost;

            if (balance >= cost) {
                balance -= cost;
                pe.description  = "Usage charged";
                pe.balanceAfter = balance;
            } else {
                pe.rejected     = true;
                pe.description  = "Insufficient balance - usage rejected";
                pe.balanceAfter = balance;  // unchanged
                ++res.rejectedCount;
            }
        }
        res.events.push_back(std::move(pe));
    }

    res.endBalance = balance;
    return res;
}

std::vector<PrepaidResult> PrepaidEngine::run() const {
    std::vector<PrepaidResult> results;
    for (const Account& acc : ds_.accounts) {
        if (acc.mode != AccountMode::Prepaid) continue;
        for (const Subscriber& sub : acc.subscribers) {
            results.push_back(simulateSubscriber(acc.ban, sub));
        }
    }
    return results;
}

} // namespace tbs
