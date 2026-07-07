#ifndef TBS_TAXRULE_HPP
#define TBS_TAXRULE_HPP

#include <string>

namespace tbs {

// A tax applied to the taxable base of a given category.
// category == "*" applies to every taxable charge regardless of category.
struct TaxRule {
    std::string id;
    std::string name;
    std::string category = "*";
    int         rateBasisPoints = 0;  // 1% = 100 bp
};

} // namespace tbs

#endif // TBS_TAXRULE_HPP
