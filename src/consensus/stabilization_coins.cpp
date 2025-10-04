// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/stabilization_coins.h>
#include <consensus/stabilization_mining.h>
#include <logging.h>
#include <script/script.h>
#include <util/strencodings.h>

namespace OConsensus {

StabilizationCoinsManager g_stabilization_coins_manager;

StabilizationCoinsManager::StabilizationCoinsManager() {
    LogPrintf("O Stabilization Coins: Initializing stabilization coin database manager\n");
}

void StabilizationCoinsManager::UpdateCoinsWithStabilization(
    const std::vector<CTransaction>& stab_txs, 
    CCoinsViewCache& view, 
    int height) {
    
    for (const auto& stab_tx : stab_txs) {
        if (!IsStabilizationTransaction(stab_tx)) {
            continue;
        }
        
        // Validate stabilization transaction
        if (!ValidateStabilizationOutputs(stab_tx, view)) {
            LogPrintf("O Stabilization Coins: Invalid stabilization transaction %s\n", 
                     stab_tx.GetHash().GetHex());
            continue;
        }
        
        // Create coin outputs for stabilization transaction
        for (size_t i = 0; i < stab_tx.vout.size(); ++i) {
            const CTxOut& output = stab_tx.vout[i];
            COutPoint outpoint(stab_tx.GetHash(), i);
            
            // Create coin entry
            Coin coin(output, height, true); // true = coinbase-like (no inputs)
            
            // Add to view
            view.AddCoin(outpoint, std::move(coin), true);
        }
        
        // Update statistics
        m_stats.total_transactions++;
        m_stats.total_recipients += static_cast<int>(stab_tx.vout.size());
        
        CAmount total_value = 0;
        for (const auto& output : stab_tx.vout) {
            total_value += output.nValue;
        }
        m_stats.total_coins_created += total_value;
        
        LogPrintf("O Stabilization Coins: Added stabilization transaction %s with %d outputs, total value %d\n",
                 stab_tx.GetHash().GetHex(), static_cast<int>(stab_tx.vout.size()), total_value);
    }
}

bool StabilizationCoinsManager::ValidateStabilizationOutputs(
    const CTransaction& stab_tx, 
    const CCoinsViewCache& view) const {
    
    // Stabilization transactions should have no inputs (like coinbase)
    if (!stab_tx.vin.empty()) {
        LogPrintf("O Stabilization Coins: Stabilization transaction has inputs (invalid)\n");
        return false;
    }
    
    // Must have at least one output
    if (stab_tx.vout.empty()) {
        LogPrintf("O Stabilization Coins: Stabilization transaction has no outputs\n");
        return false;
    }
    
    // Validate each output
    CAmount total_value = 0;
    for (const auto& output : stab_tx.vout) {
        // Check value is within bounds
        if (output.nValue < StabilizationConfig::MIN_STABILIZATION_REWARD ||
            output.nValue > StabilizationConfig::MAX_STABILIZATION_REWARD) {
            LogPrintf("O Stabilization Coins: Output value %d out of bounds [%d, %d]\n",
                     output.nValue, StabilizationConfig::MIN_STABILIZATION_REWARD,
                     StabilizationConfig::MAX_STABILIZATION_REWARD);
            return false;
        }
        
        total_value += output.nValue;
    }
    
    // Validate total limits
    if (!ValidateStabilizationLimits(total_value, static_cast<int>(stab_tx.vout.size()))) {
        return false;
    }
    
    return true;
}

StabilizationCoinsManager::StabilizationCoinStats StabilizationCoinsManager::GetStabilizationStats() const {
    return m_stats;
}

bool StabilizationCoinsManager::IsStabilizationTransaction(const CTransaction& tx) const {
    // Check if this transaction is recorded as a stabilization transaction
    auto it = m_stabilization_txs.find(tx.GetHash());
    return it != m_stabilization_txs.end();
}

std::optional<StabilizationCoinsManager::StabilizationTxInfo> 
StabilizationCoinsManager::GetStabilizationTxInfo(const uint256& tx_hash) const {
    auto it = m_stabilization_txs.find(tx_hash);
    if (it != m_stabilization_txs.end()) {
        return it->second;
    }
    return std::nullopt;
}

void StabilizationCoinsManager::RecordStabilizationTransaction(
    const uint256& tx_hash, 
    const std::string& currency,
    CAmount coins_created,
    int recipient_count,
    double deviation_ratio) {
    
    StabilizationTxInfo info;
    info.unstable_currency = currency;
    info.total_coins_created = coins_created;
    info.recipient_count = recipient_count;
    info.deviation_ratio = deviation_ratio;
    
    m_stabilization_txs[tx_hash] = info;
    
    // Update currency-specific statistics
    m_stats.coins_by_currency[currency] += coins_created;
    
    LogPrintf("O Stabilization Coins: Recorded stabilization transaction %s for currency %s, %d coins, %d recipients\n",
             tx_hash.GetHex(), currency.c_str(), coins_created, recipient_count);
}

CScript StabilizationCoinsManager::GenerateStabilizationScript(
    const CPubKey& recipient, 
    const std::string& currency) const {
    
    // Create a special script that identifies this as a stabilization coin
    // Format: OP_RETURN <stabilization_marker> <currency> <recipient_pubkey>
    CScript script;
    script << OP_RETURN;
    script << std::vector<unsigned char>{'S', 'T', 'A', 'B'}; // "STAB" marker
    script << std::vector<unsigned char>(currency.begin(), currency.end());
    script << std::vector<unsigned char>(recipient.begin(), recipient.end());
    
    return script;
}

bool StabilizationCoinsManager::ValidateStabilizationLimits(
    CAmount total_coins, 
    int recipient_count) const {
    
    // Check reasonable limits for stabilization transactions
    if (recipient_count < 1 || recipient_count > 50000) {
        LogPrintf("O Stabilization Coins: Invalid recipient count %d\n", recipient_count);
        return false;
    }
    
    if (total_coins < StabilizationConfig::MIN_STABILIZATION_REWARD ||
        total_coins > StabilizationConfig::MAX_STABILIZATION_REWARD * recipient_count) {
        LogPrintf("O Stabilization Coins: Invalid total coins %d for %d recipients\n", 
                 total_coins, recipient_count);
        return false;
    }
    
    return true;
}

} // namespace OConsensus
