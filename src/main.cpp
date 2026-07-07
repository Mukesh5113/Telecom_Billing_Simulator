// Telecom Billing Simulator - command-line entry point.
//
// Loads a data directory (rate plans, taxes, discounts, subscribers, CDRs,
// optional top-ups) and runs the postpaid billing pipeline and/or the prepaid
// real-time depletion model, emitting results as text, JSON, or CSV.

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

#include "tbs/io/DataLoader.hpp"
#include "tbs/engines/BillingEngine.hpp"
#include "tbs/engines/PrepaidEngine.hpp"
#include "tbs/io/InvoiceWriter.hpp"

namespace {

struct Options {
    std::string dataDir = "data";
    std::string mode    = "both";      // postpaid | prepaid | both
    std::string outDir;                // empty => console only
    std::string format  = "text";      // text | json | csv
    std::string account;               // empty => all
    std::string symbol  = "$";
};

void printUsage(const char* prog) {
    std::cout <<
        "Telecom Billing Simulator (C++17)\n"
        "\n"
        "Usage:\n"
        "  " << prog << " [options]\n"
        "\n"
        "Options:\n"
        "  --data <dir>       Data directory (default: data)\n"
        "  --mode <mode>      postpaid | prepaid | both     (default: both)\n"
        "  --format <fmt>     text | json | csv             (default: text)\n"
        "  --out <dir>        Write output files to <dir>   (default: console only)\n"
        "  --account <ban>    Restrict to one account (postpaid)\n"
        "  --symbol <s>       Currency symbol                (default: $)\n"
        "  --demo             Run bundled data/ end-to-end (both modes, text)\n"
        "  --help             Show this help\n"
        "\n"
        "Examples:\n"
        "  " << prog << " --demo\n"
        "  " << prog << " --data data --mode postpaid --format json --out out\n"
        "  " << prog << " --data data --mode prepaid --format csv --out out\n";
}

std::string ext(const std::string& fmt) {
    if (fmt == "json") return "json";
    if (fmt == "csv")  return "csv";
    return "txt";
}

std::string nextArg(int argc, char** argv, int& i, const std::string& flag) {
    if (i + 1 >= argc) throw std::runtime_error("missing value for " + flag);
    return argv[++i];
}

} // namespace

int main(int argc, char** argv) {
    Options opt;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--help" || a == "-h") { printUsage(argv[0]); return 0; }
            else if (a == "--demo") {
                opt.dataDir = "data"; opt.mode = "both";
                opt.format = "text";  opt.outDir.clear();
            }
            else if (a == "--data")    opt.dataDir = nextArg(argc, argv, i, a);
            else if (a == "--mode")    opt.mode    = nextArg(argc, argv, i, a);
            else if (a == "--format")  opt.format  = nextArg(argc, argv, i, a);
            else if (a == "--out")     opt.outDir  = nextArg(argc, argv, i, a);
            else if (a == "--account") opt.account = nextArg(argc, argv, i, a);
            else if (a == "--symbol")  opt.symbol  = nextArg(argc, argv, i, a);
            else {
                std::cerr << "Unknown argument: " << a << "\n\n";
                printUsage(argv[0]);
                return 2;
            }
        }

        if (opt.mode != "postpaid" && opt.mode != "prepaid" && opt.mode != "both") {
            std::cerr << "Invalid --mode '" << opt.mode << "'\n";
            return 2;
        }
        if (opt.format != "text" && opt.format != "json" && opt.format != "csv") {
            std::cerr << "Invalid --format '" << opt.format << "'\n";
            return 2;
        }

        tbs::Dataset ds = tbs::DataLoader::loadFromDir(opt.dataDir);

        if (!opt.outDir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(opt.outDir, ec);
            if (ec) {
                std::cerr << "Cannot create output dir '" << opt.outDir
                          << "': " << ec.message() << "\n";
                return 1;
            }
        }

        const bool doPost = (opt.mode == "postpaid" || opt.mode == "both");
        const bool doPre  = (opt.mode == "prepaid"  || opt.mode == "both");

        // ---------------- Postpaid ----------------
        if (doPost) {
            tbs::BillingEngine engine(ds);
            std::vector<tbs::Invoice> invoices;

            if (!opt.account.empty()) {
                const tbs::Account* found = nullptr;
                for (const tbs::Account& a : ds.accounts) {
                    if (a.ban == opt.account) { found = &a; break; }
                }
                if (!found) {
                    std::cerr << "Account '" << opt.account << "' not found\n";
                    return 1;
                }
                if (found->mode == tbs::AccountMode::Postpaid) {
                    invoices.push_back(engine.billAccount(*found));
                }
            } else {
                invoices = engine.billAll();
            }

            std::string out;
            if (opt.format == "json")      out = tbs::InvoiceWriter::toJson(invoices, opt.symbol);
            else if (opt.format == "csv")  out = tbs::InvoiceWriter::toCsv(invoices);
            else                           out = tbs::InvoiceWriter::toText(invoices, opt.symbol);

            std::cout << out << std::endl;
            if (!opt.outDir.empty()) {
                std::string path = opt.outDir + "/invoices." + ext(opt.format);
                tbs::InvoiceWriter::writeFile(path, out);
                std::cerr << "[written] " << path << "\n";
            }
        }

        // ---------------- Prepaid ----------------
        if (doPre) {
            tbs::PrepaidEngine engine(ds);
            std::vector<tbs::PrepaidResult> results = engine.run();

            std::string out;
            if (opt.format == "json")      out = tbs::InvoiceWriter::prepaidToJson(results, opt.symbol);
            else if (opt.format == "csv")  out = tbs::InvoiceWriter::prepaidToCsv(results);
            else                           out = tbs::InvoiceWriter::prepaidToText(results, opt.symbol);

            std::cout << out << std::endl;
            if (!opt.outDir.empty()) {
                std::string path = opt.outDir + "/prepaid." + ext(opt.format);
                tbs::InvoiceWriter::writeFile(path, out);
                std::cerr << "[written] " << path << "\n";
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
