#ifndef TBS_IO_INVOICEWRITER_HPP
#define TBS_IO_INVOICEWRITER_HPP

#include <string>
#include <vector>
#include "tbs/Invoice.hpp"
#include "tbs/engines/PrepaidEngine.hpp"

namespace tbs {

// Renders invoices and prepaid reports to text, JSON (RapidJSON), and CSV.
class InvoiceWriter {
public:
    // --- Postpaid invoices ---
    static std::string toText(const Invoice& inv, const std::string& symbol = "$");
    static std::string toText(const std::vector<Invoice>& invs,
                              const std::string& symbol = "$");
    static std::string toJson(const std::vector<Invoice>& invs,
                              const std::string& symbol = "$");
    static std::string toCsv(const std::vector<Invoice>& invs);

    // --- Prepaid reports ---
    static std::string prepaidToText(const std::vector<PrepaidResult>& results,
                                     const std::string& symbol = "$");
    static std::string prepaidToJson(const std::vector<PrepaidResult>& results,
                                     const std::string& symbol = "$");
    static std::string prepaidToCsv(const std::vector<PrepaidResult>& results);

    // Convenience: write a string to a file (throws on failure).
    static void writeFile(const std::string& path, const std::string& content);
};

} // namespace tbs

#endif // TBS_IO_INVOICEWRITER_HPP
