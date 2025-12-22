// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_IDENTITY_PROVIDER_REGISTRY_H
#define BITCOIN_CONSENSUS_IDENTITY_PROVIDER_REGISTRY_H

#include <pubkey.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <map>
#include <optional>
#include <set>
#include <string>

namespace OConsensus {

/** Identity Provider Metadata */
struct IdentityProvider {
    std::string provider_id;        // Provider identifier (e.g., "brightid", "kyc_usa", "worldcoin")
    CPubKey public_key;             // Provider's public key for signature verification
    std::string name;               // Human-readable name
    std::string description;         // Provider description
    bool is_active;                  // Whether provider is currently active
    int64_t registered_height;      // Block height when registered
    int64_t last_updated_height;    // Block height when last updated
    
    IdentityProvider()
        : provider_id(), public_key(), name(), description(), 
          is_active(false), registered_height(0), last_updated_height(0) {}
    
    IdentityProvider(const std::string& id, const CPubKey& key, const std::string& provider_name)
        : provider_id(id), public_key(key), name(provider_name), description(),
          is_active(true), registered_height(0), last_updated_height(0) {}
    
    SERIALIZE_METHODS(IdentityProvider, obj) {
        READWRITE(obj.provider_id, obj.public_key, obj.name, obj.description,
                  obj.is_active, obj.registered_height, obj.last_updated_height);
    }
    
    bool IsValid() const {
        return !provider_id.empty() && public_key.IsValid();
    }
};

/** Identity Provider Registry
 * 
 * Manages approved identity providers and their public keys.
 * Used to verify provider signatures in user verification transactions.
 */
class IdentityProviderRegistry {
private:
    mutable Mutex m_mutex;
    std::map<std::string, IdentityProvider> m_providers;
    std::set<std::string> m_whitelist;  // Approved provider IDs
    
public:
    IdentityProviderRegistry();
    
    /** Register a new identity provider */
    bool RegisterProvider(const IdentityProvider& provider);
    
    /** Update an existing provider */
    bool UpdateProvider(const std::string& provider_id, const IdentityProvider& provider);
    
    /** Remove a provider (mark as inactive) */
    bool RemoveProvider(const std::string& provider_id);
    
    /** Get provider by ID */
    std::optional<IdentityProvider> GetProvider(const std::string& provider_id) const;
    
    /** Get provider's public key */
    std::optional<CPubKey> GetProviderPublicKey(const std::string& provider_id) const;
    
    /** Check if provider is registered and active */
    bool IsProviderRegistered(const std::string& provider_id) const;
    
    /** Check if provider is in whitelist */
    bool IsProviderWhitelisted(const std::string& provider_id) const;
    
    /** Add provider to whitelist */
    bool AddToWhitelist(const std::string& provider_id);
    
    /** Remove provider from whitelist */
    bool RemoveFromWhitelist(const std::string& provider_id);
    
    /** Get all registered providers */
    std::vector<IdentityProvider> GetAllProviders() const;
    
    /** Get all active providers */
    std::vector<IdentityProvider> GetActiveProviders() const;
    
    /** Initialize with default providers */
    void InitializeDefaultProviders();
    
    /** Verify provider signature */
    bool VerifyProviderSignature(const std::string& provider_id,
                                 const uint256& hash,
                                 const std::vector<unsigned char>& signature) const;
};

/** Global identity provider registry instance */
extern IdentityProviderRegistry g_identity_provider_registry;

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_IDENTITY_PROVIDER_REGISTRY_H

