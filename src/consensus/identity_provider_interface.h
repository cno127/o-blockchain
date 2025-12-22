// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_IDENTITY_PROVIDER_INTERFACE_H
#define BITCOIN_CONSENSUS_IDENTITY_PROVIDER_INTERFACE_H

#include <consensus/brightid_integration.h>
#include <consensus/identity_provider_registry.h>
#include <hash.h>
#include <pubkey.h>
#include <uint256.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace OConsensus {

/** Identity Provider Interface
 * 
 * Abstract interface for identity verification providers.
 * Allows extensible support for different identity verification systems.
 */
class IIdentityProvider {
public:
    virtual ~IIdentityProvider() = default;
    
    /** Get provider identifier (e.g., "brightid", "worldcoin", "kyc_usa") */
    virtual std::string GetProviderId() const = 0;
    
    /** Get provider's display name */
    virtual std::string GetProviderName() const = 0;
    
    /** Get provider's public key for signature verification */
    virtual CPubKey GetPublicKey() const = 0;
    
    /** Verify a signature from this provider */
    virtual bool VerifySignature(const uint256& hash, const std::vector<unsigned char>& signature) const = 0;
    
    /** Verify user verification data from this provider */
    virtual bool VerifyUserData(const std::string& provider_address,
                                const std::string& verification_data,
                                const std::string& signature) const = 0;
    
    /** Check if provider is currently active */
    virtual bool IsActive() const = 0;
    
    /** Validate provider address format */
    virtual bool ValidateAddressFormat(const std::string& address) const = 0;
    
    /** Get verification status for a user */
    virtual std::optional<BrightIDStatus> GetUserStatus(const std::string& provider_address) const = 0;
    
    /** Extract birth currency from verification data (format: "COUNTRY:CURRENCY") */
    virtual std::optional<std::string> ExtractBirthCurrency(const std::string& verification_data) const = 0;
    
    /** Get provider-specific metadata */
    virtual std::map<std::string, std::string> GetMetadata() const = 0;
};

/** Concrete implementation for registered providers */
class RegisteredIdentityProvider : public IIdentityProvider {
private:
    IdentityProvider m_provider;
    
public:
    explicit RegisteredIdentityProvider(const IdentityProvider& provider)
        : m_provider(provider) {}
    
    std::string GetProviderId() const override { return m_provider.provider_id; }
    std::string GetProviderName() const override { return m_provider.name; }
    CPubKey GetPublicKey() const override { return m_provider.public_key; }
    
    bool VerifySignature(const uint256& hash, const std::vector<unsigned char>& signature) const override {
        if (!m_provider.public_key.IsValid()) {
            return false;
        }
        return m_provider.public_key.Verify(hash, signature);
    }
    
    bool VerifyUserData(const std::string& provider_address,
                       const std::string& verification_data,
                       const std::string& signature) const override {
        // Generic verification - validate signature format and address
        if (!ValidateAddressFormat(provider_address)) {
            return false;
        }
        
        // Create hash from verification data
        HashWriter hasher{};
        hasher << verification_data;
        uint256 hash = hasher.GetHash();
        
        // Parse signature (assuming DER format)
        std::vector<unsigned char> sig_bytes(signature.begin(), signature.end());
        
        return VerifySignature(hash, sig_bytes);
    }
    
    bool IsActive() const override { return m_provider.is_active; }
    
    bool ValidateAddressFormat(const std::string& address) const override {
        // Generic validation - non-empty and reasonable length
        // Provider-specific implementations can override for stricter validation
        return !address.empty() && address.length() <= 256;
    }
    
    std::optional<BrightIDStatus> GetUserStatus(const std::string& provider_address) const override {
        // Generic implementation - would query the identity database
        // Provider-specific implementations should override
        return std::nullopt;
    }
    
    std::optional<std::string> ExtractBirthCurrency(const std::string& verification_data) const override {
        // Generic implementation - looks for "COUNTRY:CURRENCY" format
        // This is a simplified parser - provider-specific implementations should override
        size_t colon_pos = verification_data.find(':');
        if (colon_pos != std::string::npos && colon_pos < verification_data.length() - 1) {
            return verification_data.substr(colon_pos + 1);
        }
        return std::nullopt;
    }
    
    std::map<std::string, std::string> GetMetadata() const override {
        std::map<std::string, std::string> metadata;
        metadata["provider_id"] = m_provider.provider_id;
        metadata["name"] = m_provider.name;
        metadata["description"] = m_provider.description;
        metadata["is_active"] = m_provider.is_active ? "true" : "false";
        return metadata;
    }
};

/** Provider Factory - Creates provider instances from registry */
class IdentityProviderFactory {
public:
    /** Create a provider instance from the registry */
    static std::unique_ptr<IIdentityProvider> CreateProvider(const std::string& provider_id);
    
    /** Create all active providers */
    static std::vector<std::unique_ptr<IIdentityProvider>> CreateAllActiveProviders();
    
    /** Check if a provider is available */
    static bool IsProviderAvailable(const std::string& provider_id);
};

} // namespace OConsensus

#endif // BITCOIN_CONSENSUS_IDENTITY_PROVIDER_INTERFACE_H

