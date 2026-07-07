#include "tbs/io/InvoiceWriter.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

namespace tbs {
namespace {

// Plain decimal, no currency symbol (for CSV/JSON payloads).
std::string plain(Money m) { return m.toString(""); }

// CSV field escaping.
std::string csvEscape(const std::string& in) {
    bool needQuote = in.find_first_of(",\"\n") != std::string::npos;
    if (!needQuote) return in;
    std::string out = "\"";
    for (char c : in) {
        if (c == '"') out += "\"\"";
        else out.push_back(c);
    }
    out.push_back('"');
    return out;
}

using JsonWriter = rapidjson::PrettyWriter<rapidjson::StringBuffer>;

void writeMoney(JsonWriter& w, const char* key, Money m) {
    w.Key(key);
    w.String(plain(m).c_str());
}

void writeCharge(JsonWriter& w, const Charge& c) {
    w.StartObject();
    w.Key("subscriber_no"); w.String(c.subscriberNo.c_str());
    w.Key("type");          w.String(toString(c.type));
    w.Key("description");   w.String(c.description.c_str());
    writeMoney(w, "amount", c.amount);
    w.Key("amount_minor");  w.Int64(c.amount.minor());
    w.Key("taxable");       w.Bool(c.taxable);
    w.Key("tax_category");  w.String(c.taxCategory.c_str());
    w.EndObject();
}

void writeInvoiceJson(JsonWriter& w, const Invoice& inv) {
    w.StartObject();
    w.Key("ban");  w.String(inv.ban.c_str());
    w.Key("name"); w.String(inv.name.c_str());

    w.Key("charges");
    w.StartArray();
    for (const Charge& c : inv.charges) writeCharge(w, c);
    w.EndArray();

    w.Key("totals");
    w.StartObject();
    writeMoney(w, "usage",     inv.usageTotal);
    writeMoney(w, "recurring", inv.recurringTotal);
    writeMoney(w, "fees",      inv.feeTotal);
    writeMoney(w, "discounts", inv.discountTotal);
    writeMoney(w, "tax",       inv.taxTotal);
    writeMoney(w, "grand_total", inv.grandTotal);
    w.EndObject();

    w.EndObject();
}

} // namespace

std::string InvoiceWriter::toText(const Invoice& inv, const std::string& symbol) {
    std::ostringstream os;
    os << "============================================================\n";
    os << " INVOICE  -  Account " << inv.ban;
    if (!inv.name.empty()) os << "  (" << inv.name << ")";
    os << "\n============================================================\n";

    std::string lastSub = "\x01"; // sentinel so first header always prints
    for (const Charge& c : inv.charges) {
        if (c.subscriberNo != lastSub) {
            lastSub = c.subscriberNo;
            os << "\n";
            if (c.subscriberNo.empty())
                os << "  [Account-level]\n";
            else
                os << "  Subscriber " << c.subscriberNo << "\n";
        }
        std::ostringstream line;
        line << "    " << std::left << std::setw(40)
             << (std::string("[") + toString(c.type) + "] " + c.description);
        std::string amt = c.amount.toString(symbol);
        os << line.str() << std::right << std::setw(14) << amt << "\n";
    }

    os << "\n------------------------------------------------------------\n";
    os << "  " << std::left << std::setw(40) << "Usage"
       << std::right << std::setw(14) << inv.usageTotal.toString(symbol) << "\n";
    os << "  " << std::left << std::setw(40) << "Recurring"
       << std::right << std::setw(14) << inv.recurringTotal.toString(symbol) << "\n";
    os << "  " << std::left << std::setw(40) << "Fees"
       << std::right << std::setw(14) << inv.feeTotal.toString(symbol) << "\n";
    os << "  " << std::left << std::setw(40) << "Discounts"
       << std::right << std::setw(14) << inv.discountTotal.toString(symbol) << "\n";
    os << "  " << std::left << std::setw(40) << "Tax"
       << std::right << std::setw(14) << inv.taxTotal.toString(symbol) << "\n";
    os << "------------------------------------------------------------\n";
    os << "  " << std::left << std::setw(40) << "GRAND TOTAL"
       << std::right << std::setw(14) << inv.grandTotal.toString(symbol) << "\n";
    os << "============================================================\n";
    return os.str();
}

std::string InvoiceWriter::toText(const std::vector<Invoice>& invs,
                                  const std::string& symbol) {
    std::ostringstream os;
    for (const Invoice& inv : invs) os << toText(inv, symbol) << "\n";
    return os.str();
}

std::string InvoiceWriter::toJson(const std::vector<Invoice>& invs,
                                  const std::string& symbol) {
    (void)symbol;
    rapidjson::StringBuffer sb;
    JsonWriter w(sb);
    w.StartObject();
    w.Key("invoices");
    w.StartArray();
    for (const Invoice& inv : invs) writeInvoiceJson(w, inv);
    w.EndArray();
    w.EndObject();
    return sb.GetString();
}

std::string InvoiceWriter::toCsv(const std::vector<Invoice>& invs) {
    std::ostringstream os;
    os << "ban,subscriber_no,type,description,amount,taxable,tax_category\n";
    for (const Invoice& inv : invs) {
        for (const Charge& c : inv.charges) {
            os << csvEscape(inv.ban) << ','
               << csvEscape(c.subscriberNo) << ','
               << toString(c.type) << ','
               << csvEscape(c.description) << ','
               << plain(c.amount) << ','
               << (c.taxable ? "Y" : "N") << ','
               << csvEscape(c.taxCategory) << '\n';
        }
    }
    return os.str();
}

std::string InvoiceWriter::prepaidToText(const std::vector<PrepaidResult>& results,
                                         const std::string& symbol) {
    std::ostringstream os;
    for (const PrepaidResult& r : results) {
        os << "============================================================\n";
        os << " PREPAID  -  Account " << r.ban
           << "  Subscriber " << r.subscriberNo << "\n";
        os << "============================================================\n";
        os << "  Opening balance: " << r.startBalance.toString(symbol) << "\n\n";
        for (const PrepaidEvent& e : r.events) {
            std::ostringstream left;
            left << "  " << e.time << "  " << std::left << std::setw(6) << e.kind
                 << " " << std::setw(38) << e.description;
            std::string amt = (e.kind == "TOPUP" ? "+" : "-") + e.amount.toString(symbol);
            if (e.rejected) amt = "(rejected)";
            os << left.str() << std::right << std::setw(14) << amt
               << "   bal " << e.balanceAfter.toString(symbol) << "\n";
        }
        os << "\n  Closing balance: " << r.endBalance.toString(symbol)
           << "   (rejected events: " << r.rejectedCount << ")\n";
        os << "============================================================\n\n";
    }
    return os.str();
}

std::string InvoiceWriter::prepaidToJson(const std::vector<PrepaidResult>& results,
                                         const std::string& symbol) {
    (void)symbol;
    rapidjson::StringBuffer sb;
    JsonWriter w(sb);
    w.StartObject();
    w.Key("prepaid");
    w.StartArray();
    for (const PrepaidResult& r : results) {
        w.StartObject();
        w.Key("ban");            w.String(r.ban.c_str());
        w.Key("subscriber_no");  w.String(r.subscriberNo.c_str());
        writeMoney(w, "start_balance", r.startBalance);
        writeMoney(w, "end_balance",   r.endBalance);
        w.Key("rejected_count"); w.Int(r.rejectedCount);
        w.Key("events");
        w.StartArray();
        for (const PrepaidEvent& e : r.events) {
            w.StartObject();
            w.Key("time");        w.String(e.time.c_str());
            w.Key("kind");        w.String(e.kind.c_str());
            w.Key("description"); w.String(e.description.c_str());
            writeMoney(w, "amount",        e.amount);
            writeMoney(w, "balance_after", e.balanceAfter);
            w.Key("rejected");    w.Bool(e.rejected);
            w.EndObject();
        }
        w.EndArray();
        w.EndObject();
    }
    w.EndArray();
    w.EndObject();
    return sb.GetString();
}

std::string InvoiceWriter::prepaidToCsv(const std::vector<PrepaidResult>& results) {
    std::ostringstream os;
    os << "ban,subscriber_no,time,kind,description,amount,balance_after,rejected\n";
    for (const PrepaidResult& r : results) {
        for (const PrepaidEvent& e : r.events) {
            os << csvEscape(r.ban) << ','
               << csvEscape(r.subscriberNo) << ','
               << csvEscape(e.time) << ','
               << e.kind << ','
               << csvEscape(e.description) << ','
               << plain(e.amount) << ','
               << plain(e.balanceAfter) << ','
               << (e.rejected ? "Y" : "N") << '\n';
        }
    }
    return os.str();
}

void InvoiceWriter::writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("cannot write file '" + path + "'");
    }
    out << content;
}

} // namespace tbs
