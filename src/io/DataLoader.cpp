#include "tbs/io/DataLoader.hpp"
#include "tbs/io/CsvReader.hpp"
#include "tbs/io/JsonLoader.hpp"

#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace tbs {
namespace {

// Map header row -> column index by name (case-insensitive).
std::unordered_map<std::string, std::size_t>
headerIndex(const std::vector<std::string>& header) {
    std::unordered_map<std::string, std::size_t> idx;
    for (std::size_t i = 0; i < header.size(); ++i) {
        idx[toUpper(header[i])] = i;
    }
    return idx;
}

const std::string& cell(const std::vector<std::string>& row,
                        const std::unordered_map<std::string, std::size_t>& idx,
                        const char* name) {
    static const std::string empty;
    auto it = idx.find(toUpper(name));
    if (it == idx.end() || it->second >= row.size()) return empty;
    return row[it->second];
}

std::string join(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    char back = dir.back();
    if (back == '/' || back == '\\') return dir + file;
    return dir + "/" + file;
}

bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

} // namespace

std::vector<Account> DataLoader::loadAccounts(const std::string& path) {
    auto rows = CsvReader::read(path);
    if (rows.empty()) return {};

    auto idx = headerIndex(rows[0]);

    std::vector<Account> accounts;
    std::unordered_map<std::string, std::size_t> banToPos;

    for (std::size_t r = 1; r < rows.size(); ++r) {
        const auto& row = rows[r];
        const std::string ban = cell(row, idx, "ban");
        if (ban.empty()) continue;

        auto it = banToPos.find(ban);
        if (it == banToPos.end()) {
            Account acc;
            acc.ban  = ban;
            acc.name = cell(row, idx, "name");
            acc.mode = parseAccountMode(cell(row, idx, "mode"));
            banToPos[ban] = accounts.size();
            accounts.push_back(std::move(acc));
            it = banToPos.find(ban);
        }

        Subscriber sub;
        sub.subscriberNo = cell(row, idx, "subscriber_no");
        sub.planId       = cell(row, idx, "plan_id");
        const std::string bal = cell(row, idx, "balance");
        sub.openingBalance = bal.empty() ? Money(0) : Money::parse(bal);

        if (!sub.subscriberNo.empty()) {
            accounts[it->second].subscribers.push_back(std::move(sub));
        }
    }
    return accounts;
}

std::vector<Cdr> DataLoader::loadCdrs(const std::string& path) {
    auto rows = CsvReader::read(path);
    if (rows.empty()) return {};

    auto idx = headerIndex(rows[0]);
    std::vector<Cdr> cdrs;
    cdrs.reserve(rows.size());

    for (std::size_t r = 1; r < rows.size(); ++r) {
        const auto& row = rows[r];
        Cdr cdr;
        cdr.subscriberNo = cell(row, idx, "subscriber_no");
        if (cdr.subscriberNo.empty()) continue;
        cdr.type      = parseServiceType(cell(row, idx, "service_type"));
        cdr.timestamp = cell(row, idx, "timestamp");
        const std::string q = cell(row, idx, "quantity");
        cdr.quantity = q.empty() ? 0 : std::stoll(q);
        cdrs.push_back(std::move(cdr));
    }
    return cdrs;
}

std::vector<Topup> DataLoader::loadTopups(const std::string& path) {
    auto rows = CsvReader::read(path);
    if (rows.empty()) return {};

    auto idx = headerIndex(rows[0]);
    std::vector<Topup> topups;

    for (std::size_t r = 1; r < rows.size(); ++r) {
        const auto& row = rows[r];
        Topup t;
        t.subscriberNo = cell(row, idx, "subscriber_no");
        if (t.subscriberNo.empty()) continue;
        t.timestamp = cell(row, idx, "timestamp");
        const std::string a = cell(row, idx, "amount");
        t.amount = a.empty() ? Money(0) : Money::parse(a);
        topups.push_back(std::move(t));
    }
    return topups;
}

Dataset DataLoader::loadFromDir(const std::string& dir) {
    Dataset ds;
    ds.catalog = JsonLoader::loadCatalog(join(dir, "rate_plans.json"),
                                         join(dir, "tax_rules.json"),
                                         join(dir, "discount_rules.json"));
    ds.accounts = loadAccounts(join(dir, "subscribers.csv"));
    ds.cdrs     = loadCdrs(join(dir, "cdrs.csv"));

    const std::string topupsPath = join(dir, "topups.csv");
    if (fileExists(topupsPath)) {
        ds.topups = loadTopups(topupsPath);
    }
    return ds;
}

} // namespace tbs
