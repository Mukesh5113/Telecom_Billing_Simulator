#ifndef TBS_ENGINES_TAXENGINE_HPP
#define TBS_ENGINES_TAXENGINE_HPP

#include <vector>
#include "tbs/TaxRule.hpp"
#include "tbs/Charge.hpp"

namespace tbs {

// Computes taxes over the taxable base. Each rule taxes the sum of taxable
// charges whose category matches the rule (or all, when category == "*").
class TaxEngine {
public:
    static std::vector<Charge> computeTaxes(const std::vector<TaxRule>& rules,
                                            const std::vector<Charge>&  charges);
};

} // namespace tbs

#endif // TBS_ENGINES_TAXENGINE_HPP
