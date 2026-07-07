#include "tbs/io/JsonLoader.hpp"
#include "tbs/io/Json.hpp"

namespace tbs {

std::vector<RatePlan> JsonLoader::loadPlans(const std::string& path) {
    rapidjson::Document doc = json::parseFile(path);
    const rapidjson::Value& arr = json::requireArray(doc, "plans");

    std::vector<RatePlan> plans;
    plans.reserve(arr.Size());
    for (auto& p : arr.GetArray()) {
        RatePlan plan;
        plan.id                    = json::requireString(p, "id");
        plan.name                  = json::getString(p, "name", plan.id);
        plan.monthlyRecurringCharge = json::getMoney(p, "monthly_recurring_charge");
        plan.regulatoryFee         = json::getMoney(p, "regulatory_fee");
        plan.adminFee              = json::getMoney(p, "admin_fee");
        plan.taxCategory           = json::getString(p, "tax_category", "STD");

        if (p.HasMember("allowance") && p["allowance"].IsObject()) {
            const rapidjson::Value& a = p["allowance"];
            plan.allowance.voiceSeconds = json::getInt64(a, "voice_seconds");
            plan.allowance.sms          = json::getInt64(a, "sms");
            plan.allowance.dataKb       = json::getInt64(a, "data_kb");
        }
        if (p.HasMember("overage") && p["overage"].IsObject()) {
            const rapidjson::Value& o = p["overage"];
            plan.overage.perVoiceMinute = json::getMoney(o, "per_voice_minute");
            plan.overage.perSms         = json::getMoney(o, "per_sms");
            plan.overage.perMb          = json::getMoney(o, "per_mb");
        }
        plans.push_back(std::move(plan));
    }
    return plans;
}

std::vector<TaxRule> JsonLoader::loadTaxes(const std::string& path) {
    rapidjson::Document doc = json::parseFile(path);
    const rapidjson::Value& arr = json::requireArray(doc, "taxes");

    std::vector<TaxRule> taxes;
    taxes.reserve(arr.Size());
    for (auto& t : arr.GetArray()) {
        TaxRule rule;
        rule.id              = json::requireString(t, "id");
        rule.name            = json::getString(t, "name", rule.id);
        rule.category        = json::getString(t, "category", "*");
        rule.rateBasisPoints = json::getInt(t, "rate_bp");
        taxes.push_back(std::move(rule));
    }
    return taxes;
}

std::vector<DiscountRule> JsonLoader::loadDiscounts(const std::string& path) {
    rapidjson::Document doc = json::parseFile(path);
    const rapidjson::Value& arr = json::requireArray(doc, "discounts");

    std::vector<DiscountRule> discounts;
    discounts.reserve(arr.Size());
    for (auto& d : arr.GetArray()) {
        DiscountRule rule;
        rule.id              = json::requireString(d, "id");
        rule.name            = json::getString(d, "name", rule.id);
        rule.kind            = parseDiscountKind(json::getString(d, "kind", "fixed_promo"));
        rule.rateBasisPoints = json::getInt(d, "rate_bp");
        rule.amount          = json::getMoney(d, "amount");
        rule.minLines        = json::getInt(d, "min_lines");
        discounts.push_back(std::move(rule));
    }
    return discounts;
}

Catalog JsonLoader::loadCatalog(const std::string& plansPath,
                                const std::string& taxesPath,
                                const std::string& discountsPath) {
    Catalog cat;
    for (RatePlan& p : loadPlans(plansPath)) {
        const std::string id = p.id;
        cat.plans.emplace(id, std::move(p));
    }
    cat.taxes     = loadTaxes(taxesPath);
    cat.discounts = loadDiscounts(discountsPath);
    return cat;
}

} // namespace tbs
