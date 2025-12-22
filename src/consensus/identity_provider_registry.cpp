// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/identity_provider_registry.h>
#include <logging.h>
#include <util/strencodings.h>
#include <sync.h>

namespace OConsensus {

IdentityProviderRegistry g_identity_provider_registry;

IdentityProviderRegistry::IdentityProviderRegistry() {
    InitializeDefaultProviders();
}

bool IdentityProviderRegistry::RegisterProvider(const IdentityProvider& provider) {
    LOCK(m_mutex);
    
    if (!provider.IsValid()) {
        LogPrintf("O Identity Provider: Cannot register invalid provider\n");
        return false;
    }
    
    if (m_providers.find(provider.provider_id) != m_providers.end()) {
        LogPrintf("O Identity Provider: Provider %s already registered\n", 
                 provider.provider_id.c_str());
        return false;
    }
    
    m_providers[provider.provider_id] = provider;
    LogPrintf("O Identity Provider: Registered provider %s (%s)\n",
             provider.provider_id.c_str(), provider.name.c_str());
    
    return true;
}

bool IdentityProviderRegistry::UpdateProvider(const std::string& provider_id, 
                                               const IdentityProvider& provider) {
    LOCK(m_mutex);
    
    auto it = m_providers.find(provider_id);
    if (it == m_providers.end()) {
        LogPrintf("O Identity Provider: Provider %s not found for update\n",
                 provider_id.c_str());
        return false;
    }
    
    it->second = provider;
    it->second.provider_id = provider_id;  // Ensure ID matches
    LogPrintf("O Identity Provider: Updated provider %s\n", provider_id.c_str());
    
    return true;
}

bool IdentityProviderRegistry::RemoveProvider(const std::string& provider_id) {
    LOCK(m_mutex);
    
    auto it = m_providers.find(provider_id);
    if (it == m_providers.end()) {
        return false;
    }
    
    it->second.is_active = false;
    LogPrintf("O Identity Provider: Deactivated provider %s\n", provider_id.c_str());
    
    return true;
}

std::optional<IdentityProvider> IdentityProviderRegistry::GetProvider(
    const std::string& provider_id) const {
    LOCK(m_mutex);
    
    auto it = m_providers.find(provider_id);
    if (it == m_providers.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::optional<CPubKey> IdentityProviderRegistry::GetProviderPublicKey(
    const std::string& provider_id) const {
    auto provider = GetProvider(provider_id);
    if (!provider.has_value() || !provider->is_active) {
        return std::nullopt;
    }
    
    return provider->public_key;
}

bool IdentityProviderRegistry::IsProviderRegistered(const std::string& provider_id) const {
    LOCK(m_mutex);
    
    auto it = m_providers.find(provider_id);
    return it != m_providers.end() && it->second.is_active;
}

bool IdentityProviderRegistry::IsProviderWhitelisted(const std::string& provider_id) const {
    LOCK(m_mutex);
    
    // Security: Require explicit whitelisting
    // If whitelist is empty, NO providers are allowed (fail-safe)
    // This prevents accidentally allowing unapproved providers
    if (m_whitelist.empty()) {
        LogPrintf("O Identity Provider: Whitelist is empty - no providers allowed\n");
        return false;
    }
    
    bool whitelisted = m_whitelist.find(provider_id) != m_whitelist.end();
    if (!whitelisted) {
        LogPrintf("O Identity Provider: Provider %s is not in whitelist\n", provider_id.c_str());
    }
    
    return whitelisted;
}

bool IdentityProviderRegistry::AddToWhitelist(const std::string& provider_id) {
    LOCK(m_mutex);
    
    auto it = m_providers.find(provider_id);
    if (it == m_providers.end()) {
        LogPrintf("O Identity Provider: Cannot whitelist unregistered provider %s\n",
                 provider_id.c_str());
        return false;
    }
    
    // Security: Only whitelist providers with valid public keys
    if (!it->second.public_key.IsValid()) {
        LogPrintf("O Identity Provider: Cannot whitelist provider %s without valid public key\n",
                 provider_id.c_str());
        return false;
    }
    
    // Security: Only whitelist active providers
    if (!it->second.is_active) {
        LogPrintf("O Identity Provider: Cannot whitelist inactive provider %s\n",
                 provider_id.c_str());
        return false;
    }
    
    m_whitelist.insert(provider_id);
    LogPrintf("O Identity Provider: Added %s to whitelist\n", provider_id.c_str());
    
    return true;
}

bool IdentityProviderRegistry::RemoveFromWhitelist(const std::string& provider_id) {
    LOCK(m_mutex);
    
    auto it = m_whitelist.find(provider_id);
    if (it == m_whitelist.end()) {
        return false;
    }
    
    m_whitelist.erase(it);
    LogPrintf("O Identity Provider: Removed %s from whitelist\n", provider_id.c_str());
    
    return true;
}

std::vector<IdentityProvider> IdentityProviderRegistry::GetAllProviders() const {
    LOCK(m_mutex);
    
    std::vector<IdentityProvider> providers;
    providers.reserve(m_providers.size());
    
    for (const auto& pair : m_providers) {
        providers.push_back(pair.second);
    }
    
    return providers;
}

std::vector<IdentityProvider> IdentityProviderRegistry::GetActiveProviders() const {
    LOCK(m_mutex);
    
    std::vector<IdentityProvider> active;
    
    for (const auto& pair : m_providers) {
        if (pair.second.is_active) {
            active.push_back(pair.second);
        }
    }
    
    return active;
}

void IdentityProviderRegistry::InitializeDefaultProviders() {
    LOCK(m_mutex);
    
    LogPrintf("O Identity Provider: Initializing default providers\n");
    
    // NOTE: Default providers are registered without public keys initially
    // Public keys should be set via governance or configuration
    // For now, we register them as inactive until public keys are configured
    // They are NOT automatically whitelisted - must be explicitly whitelisted after public key is set
    
    // BrightID
    IdentityProvider brightid("brightid", CPubKey(), "BrightID");
    brightid.description = "Social graph-based Proof of Personhood";
    brightid.is_active = false;  // Inactive until public key is set
    m_providers["brightid"] = brightid;
    
    // WorldCoin
    IdentityProvider worldcoin("worldcoin", CPubKey(), "WorldCoin");
    worldcoin.description = "Orb-based biometric verification";
    worldcoin.is_active = false;
    m_providers["worldcoin"] = worldcoin;
    
    // Idena
    IdentityProvider idena("idena", CPubKey(), "Idena");
    idena.description = "Cryptographic verification protocol";
    idena.is_active = false;
    m_providers["idena"] = idena;
    
    // KYC providers (country-specific)
    // These will be registered dynamically as needed
    // Format: kyc_<country_code> (e.g., kyc_usa, kyc_fra, kyc_mex)
    
    LogPrintf("O Identity Provider: Initialized %d default providers (not whitelisted yet)\n",
             static_cast<int>(m_providers.size()));
    LogPrintf("O Identity Provider: Providers must be explicitly whitelisted after public keys are configured\n");
}

bool IdentityProviderRegistry::VerifyProviderSignature(
    const std::string& provider_id,
    const uint256& hash,
    const std::vector<unsigned char>& signature) const {
    
    auto pubkey_opt = GetProviderPublicKey(provider_id);
    if (!pubkey_opt.has_value()) {
        LogPrintf("O Identity Provider: No public key found for provider %s\n",
                 provider_id.c_str());
        return false;
    }
    
    const CPubKey& pubkey = pubkey_opt.value();
    bool valid = pubkey.Verify(hash, signature);
    
    if (!valid) {
        LogPrintf("O Identity Provider: Signature verification failed for provider %s\n",
                 provider_id.c_str());
    }
    
    return valid;
}

} // namespace OConsensus

