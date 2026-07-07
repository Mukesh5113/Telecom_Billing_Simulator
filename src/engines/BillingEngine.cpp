#include "tbs/engines/BillingEngine.hpp"
#include "tbs/engines/RatingEngine.hpp"
#include "tbs/engines/RecurringChargeEngine.hpp"
#include "tbs/engines/DiscountEngine.hpp"
#include "tbs/engines/TaxEngine.hpp"

namespace tbs {

BillingEngine::BillingEngine(const Dataset& dataset) : ds_(dataset) {
    for (const Cdr& c : ds_.cdrs) {
        cdrsBySub_[c.subscriberNo].push_back(&c);
    }
}

std::vector<const Cdr*> BillingEngine::cdrsFor(const std::string& subscriberNo) const {
    auto it = cdrsBySub_.find(subscriberNo);
    if (it == cdrsBySub_.end()) return {};
    return it->second;
}

Invoice BillingEngine::billAccount(const Account& account) const {
    Invoice inv;
    inv.ban  = account.ban;
    inv.name = account.name;

    // Stage 1+2: usage rating and recurring/fees per subscriber.
    std::vector<Charge> preTax;
    for (const Subscriber& sub : account.subscribers) {
        const RatePlan* plan = ds_.catalog.findPlan(sub.planId);
        if (plan == nullptr) {
            // Unknown plan: surface as a zero-value note-style fee so the
            // line is visible without breaking totals.
            preTax.emplace_back(sub.subscriberNo, ChargeType::Fee,
                                "Unknown plan '" + sub.planId + "' - not rated",
                                Money(0), false, "STD");
            continue;
        }

        std::vector<Charge> usage =
            RatingEngine::rateSubscriber(sub, *plan, cdrsFor(sub.subscriberNo));
        preTax.insert(preTax.end(), usage.begin(), usage.end());

        std::vector<Charge> recurring =
            RecurringChargeEngine::chargeSubscriber(sub, *plan);
        preTax.insert(preTax.end(), recurring.begin(), recurring.end());
    }

    // Stage 3: discounts (based on pre-tax charges).
    std::vector<Charge> discounts =
        DiscountEngine::applyDiscounts(account, ds_.catalog.discounts, preTax);

    // Stage 4: taxes (on the taxable pre-tax base).
    std::vector<Charge> taxes =
        TaxEngine::computeTaxes(ds_.catalog.taxes, preTax);

    // Assemble in pipeline order.
    inv.charges.insert(inv.charges.end(), preTax.begin(), preTax.end());
    inv.charges.insert(inv.charges.end(), discounts.begin(), discounts.end());
    inv.charges.insert(inv.charges.end(), taxes.begin(), taxes.end());

    inv.recomputeTotals();
    return inv;
}

std::vector<Invoice> BillingEngine::billAll() const {
    std::vector<Invoice> invoices;
    for (const Account& acc : ds_.accounts) {
        if (acc.mode != AccountMode::Postpaid) continue;
        invoices.push_back(billAccount(acc));
    }
    return invoices;
}

} // namespace tbs
