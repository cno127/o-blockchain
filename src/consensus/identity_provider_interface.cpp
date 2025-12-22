// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/identity_provider_interface.h>
#include <consensus/identity_provider_registry.h>
#include <logging.h>

#include <map>

namespace OConsensus {

std::unique_ptr<IIdentityProvider> IdentityProviderFactory::CreateProvider(const std::string& provider_id) {
    auto provider_opt = g_identity_provider_registry.GetProvider(provider_id);
    if (!provider_opt.has_value()) {
        LogPrintf("O Identity Provider Factory: Provider %s not found\n", provider_id.c_str());
        return nullptr;
    }
    
    const IdentityProvider& provider = provider_opt.value();
    if (!provider.is_active) {
        LogPrintf("O Identity Provider Factory: Provider %s is not active\n", provider_id.c_str());
        return nullptr;
    }
    
    return std::unique_ptr<IIdentityProvider>(new RegisteredIdentityProvider(provider));
}

std::vector<std::unique_ptr<IIdentityProvider>> IdentityProviderFactory::CreateAllActiveProviders() {
    auto providers = g_identity_provider_registry.GetActiveProviders();
    std::vector<std::unique_ptr<IIdentityProvider>> instances;
    
    for (const auto& provider : providers) {
        instances.push_back(std::unique_ptr<IIdentityProvider>(new RegisteredIdentityProvider(provider)));
    }
    
    LogPrintf("O Identity Provider Factory: Created %d active provider instances\n",
             static_cast<int>(instances.size()));
    
    return instances;
}

bool IdentityProviderFactory::IsProviderAvailable(const std::string& provider_id) {
    return g_identity_provider_registry.IsProviderRegistered(provider_id) &&
           g_identity_provider_registry.IsProviderWhitelisted(provider_id);
}

} // namespace OConsensus

