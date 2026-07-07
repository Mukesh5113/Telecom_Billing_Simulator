#ifndef TBS_TYPES_HPP
#define TBS_TYPES_HPP

#include <string>
#include <algorithm>
#include <cctype>

namespace tbs {

enum class ServiceType { Voice, Sms, Data, Unknown };

enum class ChargeType { Usage, Recurring, Fee, Discount, Tax };

enum class AccountMode { Postpaid, Prepaid };

inline std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(::toupper(c)); });
    return s;
}

inline ServiceType parseServiceType(const std::string& in) {
    const std::string s = toUpper(in);
    if (s == "VOICE" || s == "V") return ServiceType::Voice;
    if (s == "SMS"   || s == "S") return ServiceType::Sms;
    if (s == "DATA"  || s == "D") return ServiceType::Data;
    return ServiceType::Unknown;
}

inline const char* toString(ServiceType t) {
    switch (t) {
        case ServiceType::Voice: return "VOICE";
        case ServiceType::Sms:   return "SMS";
        case ServiceType::Data:  return "DATA";
        default:                 return "UNKNOWN";
    }
}

inline const char* toString(ChargeType t) {
    switch (t) {
        case ChargeType::Usage:     return "USAGE";
        case ChargeType::Recurring: return "RECURRING";
        case ChargeType::Fee:       return "FEE";
        case ChargeType::Discount:  return "DISCOUNT";
        case ChargeType::Tax:       return "TAX";
    }
    return "UNKNOWN";
}

inline AccountMode parseAccountMode(const std::string& in) {
    return toUpper(in) == "PREPAID" ? AccountMode::Prepaid : AccountMode::Postpaid;
}

inline const char* toString(AccountMode m) {
    return m == AccountMode::Prepaid ? "PREPAID" : "POSTPAID";
}

} // namespace tbs

#endif // TBS_TYPES_HPP
