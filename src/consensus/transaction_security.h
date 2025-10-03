// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_TRANSACTION_SECURITY_H
#define BITCOIN_CONSENSUS_TRANSACTION_SECURITY_H

#include <cstdint>
#include <consensus/amount.h>
#include <consensus/o_amount.h>

/** Transaction Security Configuration for Fast Block Times */
namespace TransactionSecurity {
    
    /** Block time configuration */
    static constexpr int64_t BLOCK_TIME_SECONDS = 12;              // 12-second blocks (Ethereum-like)
    static constexpr int BLOCKS_PER_HOUR = 300;                    // 300 blocks per hour
    static constexpr int BLOCKS_PER_DAY = 7200;                    // 7,200 blocks per day
    
    /** Confirmation requirements based on transaction amount */
    static constexpr int MICRO_TX_CONFIRMATIONS = 1;               // < 1 O (12 seconds)
    static constexpr int SMALL_TX_CONFIRMATIONS = 3;               // 1-100 O (36 seconds)
    static constexpr int MEDIUM_TX_CONFIRMATIONS = 6;              // 100-10,000 O (72 seconds)
    static constexpr int LARGE_TX_CONFIRMATIONS = 12;              // 10,000-100,000 O (2.4 minutes)
    static constexpr int HIGH_VALUE_TX_CONFIRMATIONS = 30;         // > 100,000 O (6 minutes)
    
    /** Transaction amount thresholds */
    static constexpr CAmount MICRO_TX_THRESHOLD = OAmount::O(1);           // 1.00 O
    static constexpr CAmount SMALL_TX_THRESHOLD = OAmount::O(100);         // 100.00 O
    static constexpr CAmount MEDIUM_TX_THRESHOLD = OAmount::O(10000);      // 10,000.00 O
    static constexpr CAmount LARGE_TX_THRESHOLD = OAmount::O(100000);      // 100,000.00 O
    
    /** Security level descriptions */
    enum class SecurityLevel {
        MICRO,      // Very low risk, fast confirmation
        SMALL,      // Low risk, quick confirmation
        MEDIUM,     // Medium risk, moderate confirmation
        LARGE,      // High risk, secure confirmation
        HIGH_VALUE  // Very high risk, maximum security
    };
    
    /** Get required confirmations for transaction amount */
    inline int GetRequiredConfirmations(CAmount amount) {
        if (amount < MICRO_TX_THRESHOLD) {
            return MICRO_TX_CONFIRMATIONS;
        } else if (amount < SMALL_TX_THRESHOLD) {
            return SMALL_TX_CONFIRMATIONS;
        } else if (amount < MEDIUM_TX_THRESHOLD) {
            return MEDIUM_TX_CONFIRMATIONS;
        } else if (amount < LARGE_TX_THRESHOLD) {
            return LARGE_TX_CONFIRMATIONS;
        } else {
            return HIGH_VALUE_TX_CONFIRMATIONS;
        }
    }
    
    /** Get security level for transaction amount */
    inline SecurityLevel GetSecurityLevel(CAmount amount) {
        if (amount < MICRO_TX_THRESHOLD) {
            return SecurityLevel::MICRO;
        } else if (amount < SMALL_TX_THRESHOLD) {
            return SecurityLevel::SMALL;
        } else if (amount < MEDIUM_TX_THRESHOLD) {
            return SecurityLevel::MEDIUM;
        } else if (amount < LARGE_TX_THRESHOLD) {
            return SecurityLevel::LARGE;
        } else {
            return SecurityLevel::HIGH_VALUE;
        }
    }
    
    /** Get estimated confirmation time in seconds */
    inline int64_t GetEstimatedConfirmationTime(CAmount amount) {
        int confirmations = GetRequiredConfirmations(amount);
        return confirmations * BLOCK_TIME_SECONDS;
    }
    
    /** Get estimated confirmation time in minutes */
    inline double GetEstimatedConfirmationTimeMinutes(CAmount amount) {
        return static_cast<double>(GetEstimatedConfirmationTime(amount)) / 60.0;
    }
    
    /** Check if transaction is considered secure */
    inline bool IsTransactionSecure(CAmount amount, int current_confirmations) {
        int required = GetRequiredConfirmations(amount);
        return current_confirmations >= required;
    }
    
    /** Get security level description */
    inline const char* GetSecurityLevelDescription(SecurityLevel level) {
        switch (level) {
            case SecurityLevel::MICRO:      return "Very Low Risk - Fast Confirmation";
            case SecurityLevel::SMALL:      return "Low Risk - Quick Confirmation";
            case SecurityLevel::MEDIUM:     return "Medium Risk - Moderate Confirmation";
            case SecurityLevel::LARGE:      return "High Risk - Secure Confirmation";
            case SecurityLevel::HIGH_VALUE: return "Very High Risk - Maximum Security";
            default:                        return "Unknown Security Level";
        }
    }
    
    /** Get use case description for transaction amount */
    inline const char* GetUseCaseDescription(CAmount amount) {
        if (amount < MICRO_TX_THRESHOLD) {
            return "Coffee, small purchases, micro-payments";
        } else if (amount < SMALL_TX_THRESHOLD) {
            return "Daily purchases, bills, small transfers";
        } else if (amount < MEDIUM_TX_THRESHOLD) {
            return "Rent, salary, medium purchases";
        } else if (amount < LARGE_TX_THRESHOLD) {
            return "Car purchase, large transfers";
        } else {
            return "House purchase, major investments";
        }
    }
    
} // namespace TransactionSecurity

#endif // BITCOIN_CONSENSUS_TRANSACTION_SECURITY_H
