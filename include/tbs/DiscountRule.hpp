#ifndef TBS_DISCOUNTRULE_HPP
#define TBS_DISCOUNTRULE_HPP

#include <string>
#include "tbs/Money.hpp"

namespace tbs {

enum class DiscountKind {
    PercentOffRecurring,  // percentage off each subscriber's recurring charge
    FixedPromo,           // flat account-level credit
    MultiLineBundle       // flat account-level credit when line count >= minLines
};

inline DiscountKind parseDiscountKind(const std::string& in) {
    if (in == "percent_off_recurring") return DiscountKind::PercentOffRecurring;
    if (in == "fixed_promo")           return DiscountKind::FixedPromo;
    if (in == "multi_line_bundle")     return DiscountKind::MultiLineBundle;
    return DiscountKind::FixedPromo;
}

inline const char* toString(DiscountKind k) {
    switch (k) {
        case DiscountKind::PercentOffRecurring: return "percent_off_recurring";
        case DiscountKind::FixedPromo:          return "fixed_promo";
        case DiscountKind::MultiLineBundle:     return "multi_line_bundle";
    }
    return "unknown";
}

struct DiscountRule {
    std::string  id;
    std::string  name;
    DiscountKind kind = DiscountKind::FixedPromo;
    int          rateBasisPoints = 0;  // for PercentOffRecurring
    Money        amount;               // for FixedPromo / MultiLineBundle
    int          minLines = 0;         // for MultiLineBundle
};

} // namespace tbs

#endif // TBS_DISCOUNTRULE_HPP
