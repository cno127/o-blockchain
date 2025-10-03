// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_O_AMOUNT_H
#define BITCOIN_CONSENSUS_O_AMOUNT_H

#include <consensus/amount.h>
#include <util/strencodings.h>

namespace OAmount {

/**
 * O Blockchain uses 2 decimal places (like USD cents)
 * 
 * Unlike Bitcoin where 1 BTC = 100,000,000 satoshis (8 decimals for high value),
 * O blockchain uses 1 O = 100 units (2 decimals) because:
 * 
 * 1. Water prices vary: $1.50, $2.25, etc. (need precision)
 * 2. Similar to traditional currency: Like cents in USD
 * 3. User friendly: Everyone understands cents
 * 4. Sufficient precision: 0.01 O = 1 cent equivalent
 * 
 * This makes O blockchain familiar and practical.
 */

/** The O coin unit - 100 cents per O (2 decimals) */
static constexpr CAmount O_COIN = 100;

/** For clarity in code, use these helpers */
inline constexpr CAmount O(int64_t n) { 
    return n * O_COIN; 
}

/** O cent (smallest unit) - like USD cents */
inline constexpr CAmount O_CENT(int64_t n) {
    return n;
}

/** Still compatible with CAmount (int64_t) for all Bitcoin Core functions */
static constexpr CAmount O_SATOSHI = 1;  // Smallest unit = 0.01 O

/** Maximum money supply */
// O Blockchain has UNLIMITED supply (unlike Bitcoin's 21M cap)
// This is intentional because O is a STABLE COIN, not a scarce asset
static constexpr CAmount MAX_O_SUPPLY = 0;  // 0 = unlimited

/** Check if amount is valid (non-negative only) */
inline bool MoneyRange(const CAmount& nValue) {
    return nValue >= 0;  // No upper limit for O coins
}

/** Format O amount for display (2 decimals) */
inline std::string FormatO(const CAmount& n) {
    double amount = static_cast<double>(n) / O_COIN;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f O", amount);
    return std::string(buffer);
}

} // namespace OAmount

// For convenience, make O() function available globally in O blockchain code
using OAmount::O;

#endif // BITCOIN_CONSENSUS_O_AMOUNT_H

