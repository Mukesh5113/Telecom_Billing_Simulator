#ifndef TBS_MONEY_HPP
#define TBS_MONEY_HPP

#include <cstdint>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace tbs {

// Currency amount stored as signed integer minor units (e.g. cents).
// Rule #1 of billing: never represent money with floating point.
class Money {
public:
    Money() : minor_(0) {}
    explicit Money(std::int64_t minorUnits) : minor_(minorUnits) {}

    static Money fromMinor(std::int64_t minorUnits) { return Money(minorUnits); }

    // Parse "12.34", "-1.50", "$12.34", "12", ".99". Throws on garbage.
    static Money parse(const std::string& in) {
        std::string s;
        s.reserve(in.size());
        for (char c : in) {
            if (c == ' ' || c == '$' || c == ',') continue;
            s.push_back(c);
        }
        if (s.empty()) return Money(0);

        bool neg = false;
        std::size_t i = 0;
        if (s[i] == '+' || s[i] == '-') { neg = (s[i] == '-'); ++i; }

        std::int64_t whole = 0;
        std::int64_t frac = 0;
        int fracDigits = 0;
        bool seenDot = false;
        bool anyDigit = false;

        for (; i < s.size(); ++i) {
            char c = s[i];
            if (c == '.') {
                if (seenDot) throw std::invalid_argument("Money::parse: bad amount '" + in + "'");
                seenDot = true;
                continue;
            }
            if (c < '0' || c > '9') {
                throw std::invalid_argument("Money::parse: bad amount '" + in + "'");
            }
            anyDigit = true;
            if (!seenDot) {
                whole = whole * 10 + (c - '0');
            } else if (fracDigits < 2) {
                frac = frac * 10 + (c - '0');
                ++fracDigits;
            }
            // extra fractional digits beyond 2 are truncated
        }
        if (!anyDigit) throw std::invalid_argument("Money::parse: bad amount '" + in + "'");
        while (fracDigits < 2) { frac *= 10; ++fracDigits; }

        std::int64_t total = whole * 100 + frac;
        return Money(neg ? -total : total);
    }

    std::int64_t minor() const { return minor_; }
    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money operator+(Money o) const { return Money(minor_ + o.minor_); }
    Money operator-(Money o) const { return Money(minor_ - o.minor_); }
    Money operator-() const { return Money(-minor_); }
    Money& operator+=(Money o) { minor_ += o.minor_; return *this; }
    Money& operator-=(Money o) { minor_ -= o.minor_; return *this; }

    bool operator==(Money o) const { return minor_ == o.minor_; }
    bool operator!=(Money o) const { return minor_ != o.minor_; }
    bool operator<(Money o) const { return minor_ < o.minor_; }
    bool operator<=(Money o) const { return minor_ <= o.minor_; }
    bool operator>(Money o) const { return minor_ > o.minor_; }
    bool operator>=(Money o) const { return minor_ >= o.minor_; }

    // Multiply by an integer quantity (exact, no rounding).
    Money timesInt(std::int64_t n) const { return Money(minor_ * n); }

    // Scale by a rate expressed in basis points (1% = 100 bp), rounding
    // half away from zero. Used for tax and percentage discounts.
    Money scaleBasisPoints(int basisPoints) const {
        const std::int64_t den = 10000;
        std::int64_t num = minor_ * static_cast<std::int64_t>(basisPoints);
        std::int64_t q = num / den;
        std::int64_t r = num % den;
        if (r != 0 && (std::llabs(r) * 2 >= den)) {
            q += (num > 0 ? 1 : -1);
        }
        return Money(q);
    }

    // "$12.34" / "-$1.50"
    std::string toString(const std::string& symbol = "$") const {
        std::int64_t m = minor_;
        bool neg = m < 0;
        if (neg) m = -m;
        std::int64_t dollars = m / 100;
        std::int64_t cents = m % 100;
        std::ostringstream oss;
        if (neg) oss << '-';
        oss << symbol << dollars << '.'
            << std::setw(2) << std::setfill('0') << cents;
        return oss.str();
    }

private:
    std::int64_t minor_;
};

} // namespace tbs

#endif // TBS_MONEY_HPP
