# Telecom Billing Simulator

A modern **C++17** command-line telecom billing simulator. It loads rate plans,
tax rules, discount rules, subscribers, and CDRs (Call Detail Records), then runs
a staged billing pipeline to produce invoices for **postpaid** accounts and a
real-time balance-depletion simulation for **prepaid** subscribers.

Output can be rendered as human-readable **text**, **JSON** (via
[RapidJSON](https://github.com/Tencent/rapidjson)), or **CSV**.

---

## Features

- **Usage rating** - prices voice / SMS / data CDRs, pooling usage per subscriber,
  deducting plan allowances, and charging overage.
- **Recurring charges + fees** - monthly plan charge plus regulatory and
  administrative fees (REG / MNT / TAX-fee style line items).
- **Discounts** - percent-off-recurring, flat promo credits, and multi-line
  bundle discounts.
- **Taxes** - percentage taxes by category (e.g. federal excise + state sales),
  applied to the taxable base.
- **Billing cycle / invoicing** - aggregates all line items into a per-account
  invoice with per-subscriber breakdown and section subtotals.
- **Prepaid real-time charging** - processes CDRs and top-ups in time order,
  depleting a wallet and flagging insufficient balance.

## Design highlights

- **Money is never floating point.** All amounts are stored as signed integer
  minor units (cents) in the [`Money`](include/tbs/Money.hpp) type. Percentages
  (tax, discounts) use basis points with half-away-from-zero rounding.
- **Core library + thin CLI.** All logic lives in `libtbs`; `main.cpp` only
  parses arguments and orchestrates. This keeps the logic unit-testable.
- **Staged engine pipeline.** Each engine takes records in and emits `Charge`
  line items, so stages compose and can be tested independently.
- **RapidJSON is isolated** behind a thin [`tbs::json`](include/tbs/io/Json.hpp)
  wrapper, so engine code never touches RapidJSON types directly.

### Pipeline

```
CDRs ─► RatingEngine ─┐
                      ├─► pre-tax charges ─► DiscountEngine ─► TaxEngine ─► BillingEngine ─► Invoice
Plans ─► Recurring ───┘

(prepaid) CDRs + Top-ups ─► PrepaidEngine ─► balance history
```

---

## Build

Requirements: a C++17 compiler (GCC 9+, Clang 9+, or MSVC 2019+) and CMake 3.16+.
RapidJSON is vendored under `third_party/rapidjson/`, so there is nothing to
install.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

The CLI binary is `build/tbs` (or `build/Release/tbs.exe` on MSVC).

## Usage

```
tbs [options]

  --data <dir>       Data directory (default: data)
  --mode <mode>      postpaid | prepaid | both     (default: both)
  --format <fmt>     text | json | csv             (default: text)
  --out <dir>        Write output files to <dir>   (default: console only)
  --account <ban>    Restrict to one account (postpaid)
  --symbol <s>       Currency symbol                (default: $)
  --demo             Run bundled data/ end-to-end (both modes, text)
  --help             Show help
```

### Examples

```bash
# Run the bundled sample end-to-end
./build/tbs --demo

# Postpaid invoices as JSON, written to out/invoices.json
./build/tbs --data data --mode postpaid --format json --out out

# Prepaid balance history as CSV
./build/tbs --data data --mode prepaid --format csv --out out

# Single account
./build/tbs --data data --mode postpaid --account 1001
```

---

## Data formats

All input lives in one directory (see [`data/`](data/) for a working example).

### `rate_plans.json`
```json
{
  "plans": [
    {
      "id": "PLAN_S",
      "name": "Starter 500",
      "monthly_recurring_charge": "20.00",
      "allowance": { "voice_seconds": 30000, "sms": 200, "data_kb": 2097152 },
      "overage":   { "per_voice_minute": "0.10", "per_sms": "0.05", "per_mb": "0.01" },
      "regulatory_fee": "1.50",
      "admin_fee": "0.99",
      "tax_category": "STD"
    }
  ]
}
```

### `tax_rules.json`
```json
{ "taxes": [ { "id": "STATE", "name": "State Sales Tax", "category": "STD", "rate_bp": 825 } ] }
```
`rate_bp` is basis points (1% = 100 bp). `category` of `"*"` taxes every taxable charge.

### `discount_rules.json`
```json
{ "discounts": [
  { "id": "LOYAL",  "name": "Loyalty 10%",  "kind": "percent_off_recurring", "rate_bp": 1000 },
  { "id": "FAMILY", "name": "Multi-line",   "kind": "multi_line_bundle", "amount": "10.00", "min_lines": 3 }
] }
```
`kind` is one of `percent_off_recurring`, `fixed_promo`, `multi_line_bundle`.

### `subscribers.csv`
```
ban,name,mode,subscriber_no,plan_id,balance
1001,Alice Family,postpaid,5551000001,PLAN_S,0
2001,Carol Prepaid,prepaid,5559000001,PLAN_PRE,10.00
```
Rows are grouped into accounts by `ban`. `balance` is the prepaid opening balance
(ignored for postpaid).

### `cdrs.csv`
```
subscriber_no,service_type,timestamp,quantity
5551000001,VOICE,2026-07-01T09:15:00,18000
```
`quantity` units: **VOICE** = seconds, **SMS** = message count, **DATA** = kilobytes.

### `topups.csv` (optional, prepaid)
```
subscriber_no,timestamp,amount
5559000001,2026-07-02T09:00:00,20.00
```

---

## Modeling notes

- **Rounding**: overage is billed per *started* unit (minute / MB rounded up);
  tax/discount percentages round half away from zero at the line-item level;
  invoice totals are the sum of already-rounded line items.
- **Discounts** are modeled as non-taxable credits and therefore do **not**
  reduce the tax base (tax is computed on gross taxable charges). This is a
  deliberate simplification and can be revisited per jurisdiction.
- **One billing cycle per run.** Timestamps are used for prepaid ordering; a
  cycle-window filter can be added later.

## Project layout

```
include/tbs/          Public headers (domain, engines, io)
src/                  Implementations + main.cpp
data/                 Sample input data
tests/                Self-contained test suite (CTest)
third_party/rapidjson RapidJSON (vendored, MIT)
.github/workflows     CI (build + test on Linux and Windows)
```

## License

MIT - see [LICENSE](LICENSE). Bundles RapidJSON under the MIT license.
