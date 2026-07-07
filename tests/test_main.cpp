// Self-contained test suite (no external framework).
// Returns non-zero exit code if any check fails (CTest-friendly).

#include <iostream>
#include <string>
#include <vector>

#include "tbs/Money.hpp"
#include "tbs/Dataset.hpp"
#include "tbs/engines/RatingEngine.hpp"
#include "tbs/engines/RecurringChargeEngine.hpp"
#include "tbs/engines/DiscountEngine.hpp"
#include "tbs/engines/TaxEngine.hpp"
#include "tbs/engines/BillingEngine.hpp"
#include "tbs/engines/PrepaidEngine.hpp"

using namespace tbs;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        ++g_checks;                                                      \
        if (!(cond)) {                                                   \
            ++g_failures;                                                \
            std::cerr << "FAIL [" << __LINE__ << "]: " #cond "\n";       \
        }                                                                \
    } while (0)

#define CHECK_MONEY(actual, expectMinor)                                            \
    do {                                                                            \
        ++g_checks;                                                                 \
        Money _a = (actual);                                                        \
        if (_a.minor() != (expectMinor)) {                                          \
            ++g_failures;                                                           \
            std::cerr << "FAIL [" << __LINE__ << "]: expected " << (expectMinor)    \
                      << " got " << _a.minor() << " (" << _a.toString() << ")\n";   \
        }                                                                           \
    } while (0)

static void testMoney() {
    CHECK_MONEY(Money::parse("12.34"), 1234);
    CHECK_MONEY(Money::parse("$1.50"), 150);
    CHECK_MONEY(Money::parse("-2"),   -200);
    CHECK_MONEY(Money::parse(".99"),    99);
    CHECK_MONEY(Money::parse("1,000.05"), 100005);

    CHECK(Money::parse("12.34").toString() == "$12.34");
    CHECK(Money(-150).toString() == "-$1.50");
    CHECK(Money(150).toString("") == "1.50");

    CHECK_MONEY(Money(1000) + Money(234), 1234);
    CHECK_MONEY(Money(1000) - Money(1234), -234);
    CHECK_MONEY(Money(100).timesInt(3), 300);

    // Rounding half away from zero.
    CHECK_MONEY(Money(3100).scaleBasisPoints(825), 256);  // 25.575 -> 25.58? (2557.5 -> 2558)
    CHECK_MONEY(Money(100).scaleBasisPoints(825), 8);     // 0.0825 -> 0.08
    CHECK_MONEY(Money(3100).scaleBasisPoints(300), 93);   // exact 0.93
    CHECK_MONEY(Money(2000).scaleBasisPoints(1000), 200); // 10% of 20.00 = 2.00
}

static RatePlan starterPlan() {
    RatePlan p;
    p.id = "PLAN_S"; p.name = "Starter";
    p.monthlyRecurringCharge = Money::parse("20.00");
    p.allowance = { 30000, 200, 2097152 };
    p.overage.perVoiceMinute = Money::parse("0.10");
    p.overage.perSms         = Money::parse("0.05");
    p.overage.perMb          = Money::parse("0.01");
    p.regulatoryFee = Money::parse("1.50");
    p.adminFee      = Money::parse("0.99");
    p.taxCategory   = "STD";
    return p;
}

static void testRating() {
    RatePlan plan = starterPlan();
    Subscriber sub; sub.subscriberNo = "5551000001"; sub.planId = "PLAN_S";

    std::vector<Cdr> cdrs = {
        {"5551000001", ServiceType::Voice, "t1", 18000},
        {"5551000001", ServiceType::Voice, "t2", 15000},   // total 33000s -> 50 min over
        {"5551000001", ServiceType::Sms,   "t3", 250},     // 50 over
        {"5551000001", ServiceType::Data,  "t4", 2200000}, // 102848 KB over -> 101 MB
    };
    std::vector<const Cdr*> ptrs;
    for (auto& c : cdrs) ptrs.push_back(&c);

    std::vector<Charge> usage = RatingEngine::rateSubscriber(sub, plan, ptrs);
    Money total(0);
    for (auto& c : usage) total += c.amount;
    // 5.00 + 2.50 + 1.01 = 8.51
    CHECK_MONEY(total, 851);
    CHECK(usage.size() == 3);

    // Pay-as-you-go single CDR (prepaid style): 10 min * 0.10 = 1.00
    Cdr voice{"x", ServiceType::Voice, "t", 600};
    CHECK_MONEY(RatingEngine::costOfCdr(plan, voice), 100);
}

static void testRecurring() {
    RatePlan plan = starterPlan();
    Subscriber sub; sub.subscriberNo = "5551000001";
    auto charges = RecurringChargeEngine::chargeSubscriber(sub, plan);
    Money total(0);
    for (auto& c : charges) total += c.amount;
    CHECK_MONEY(total, 2000 + 150 + 99);  // MRC + reg + admin
    CHECK(charges.size() == 3);
}

static void testDiscountAndTax() {
    // One recurring charge of 20.00, taxable base 31.00.
    std::vector<Charge> pre = {
        {"s1", ChargeType::Usage,     "usage", Money(851), true, "STD"},
        {"s1", ChargeType::Recurring, "mrc",   Money(2000), true, "STD"},
        {"s1", ChargeType::Fee,       "fees",  Money(249), true, "STD"},
    };

    DiscountRule loyal;
    loyal.id = "LOYAL"; loyal.kind = DiscountKind::PercentOffRecurring;
    loyal.rateBasisPoints = 1000;
    std::vector<DiscountRule> discs = { loyal };

    Account acc; acc.ban = "1001";
    acc.subscribers.push_back(Subscriber{"s1", "PLAN_S", Money(0)});

    auto d = DiscountEngine::applyDiscounts(acc, discs, pre);
    Money dTotal(0);
    for (auto& c : d) dTotal += c.amount;
    CHECK_MONEY(dTotal, -200);  // 10% of 20.00

    TaxRule fed;   fed.id = "FED";   fed.category = "*";   fed.rateBasisPoints = 300;
    TaxRule state; state.id = "STATE"; state.category = "STD"; state.rateBasisPoints = 825;
    std::vector<TaxRule> taxes = { fed, state };

    auto t = TaxEngine::computeTaxes(taxes, pre);
    Money tTotal(0);
    for (auto& c : t) tTotal += c.amount;
    // FED 0.93 + STATE 2.56 = 3.49
    CHECK_MONEY(tTotal, 349);
}

static Dataset buildDataset() {
    Dataset ds;
    ds.catalog.plans.emplace("PLAN_S", starterPlan());

    TaxRule fed;   fed.id = "FED";   fed.category = "*";   fed.rateBasisPoints = 300;
    TaxRule state; state.id = "STATE"; state.category = "STD"; state.rateBasisPoints = 825;
    ds.catalog.taxes = { fed, state };

    DiscountRule loyal;
    loyal.id = "LOYAL"; loyal.kind = DiscountKind::PercentOffRecurring;
    loyal.rateBasisPoints = 1000;
    ds.catalog.discounts = { loyal };

    Account acc; acc.ban = "1001"; acc.name = "Test"; acc.mode = AccountMode::Postpaid;
    acc.subscribers.push_back(Subscriber{"5551000001", "PLAN_S", Money(0)});
    ds.accounts.push_back(acc);

    ds.cdrs = {
        {"5551000001", ServiceType::Voice, "t1", 18000},
        {"5551000001", ServiceType::Voice, "t2", 15000},
        {"5551000001", ServiceType::Sms,   "t3", 250},
        {"5551000001", ServiceType::Data,  "t4", 2200000},
    };
    return ds;
}

static void testBillingReconciliation() {
    Dataset ds = buildDataset();
    BillingEngine engine(ds);
    auto invoices = engine.billAll();
    CHECK(invoices.size() == 1);
    if (invoices.empty()) return;

    const Invoice& inv = invoices[0];
    CHECK_MONEY(inv.usageTotal,     851);
    CHECK_MONEY(inv.recurringTotal, 2000);
    CHECK_MONEY(inv.feeTotal,       249);
    CHECK_MONEY(inv.discountTotal, -200);
    CHECK_MONEY(inv.taxTotal,       349);
    CHECK_MONEY(inv.grandTotal,     3249);  // 8.51+20+2.49-2.00+3.49

    // Reconciliation: sum of line items equals grand total.
    Money sum(0);
    for (const Charge& c : inv.charges) sum += c.amount;
    CHECK_MONEY(sum, inv.grandTotal.minor());
}

static void testPrepaid() {
    Dataset ds;
    RatePlan pre;
    pre.id = "PLAN_PRE";
    pre.overage.perVoiceMinute = Money::parse("0.15");
    pre.overage.perSms         = Money::parse("0.10");
    pre.overage.perMb          = Money::parse("0.02");
    ds.catalog.plans.emplace("PLAN_PRE", pre);

    Account acc; acc.ban = "2001"; acc.mode = AccountMode::Prepaid;
    acc.subscribers.push_back(Subscriber{"5559000001", "PLAN_PRE", Money::parse("10.00")});
    ds.accounts.push_back(acc);

    ds.cdrs = {
        {"5559000001", ServiceType::Voice, "2026-07-01T08:00:00", 600},    // 1.50
        {"5559000001", ServiceType::Sms,   "2026-07-01T09:00:00", 5},      // 0.50
        {"5559000001", ServiceType::Data,  "2026-07-01T10:00:00", 51200},  // 1.00
        {"5559000001", ServiceType::Data,  "2026-07-02T10:00:00", 512000}, // 10.00
        {"5559000001", ServiceType::Voice, "2026-07-03T10:00:00", 12000},  // 30.00 -> reject
    };
    ds.topups = {
        {"5559000001", "2026-07-02T09:00:00", Money::parse("20.00")},
    };

    PrepaidEngine engine(ds);
    auto results = engine.run();
    CHECK(results.size() == 1);
    if (results.empty()) return;

    const PrepaidResult& r = results[0];
    CHECK(r.events.size() == 6);         // 5 cdrs + 1 topup
    CHECK(r.rejectedCount == 1);
    CHECK_MONEY(r.startBalance, 1000);
    CHECK_MONEY(r.endBalance,   1700);   // 10 -1.5 -0.5 -1 +20 -10 = 17.00
}

int main() {
    testMoney();
    testRating();
    testRecurring();
    testDiscountAndTax();
    testBillingReconciliation();
    testPrepaid();

    std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << g_failures << " check(s) FAILED\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}
