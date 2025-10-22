// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_O_PROTOCOL_MESSAGES_H
#define BITCOIN_O_PROTOCOL_MESSAGES_H

#include <measurement/measurement_system.h>
#include <pubkey.h>
#include <serialize.h>
#include <uint256.h>

#include <vector>

/**
 * O Blockchain P2P Protocol Messages
 * 
 * Extends Bitcoin Core's P2P protocol with O-specific messages for:
 * - Measurement invitation announcements
 * - Measurement invitation requests
 */

/**
 * CMeasureInv: Announces measurement invitations to peers
 * 
 * Broadcast when new measurement invitations are created.
 * Peers with matching public keys will notify their wallet UI.
 */
class CMeasureInv
{
public:
    std::vector<OMeasurement::MeasurementInvite> vInvites;

    CMeasureInv() {}
    explicit CMeasureInv(const std::vector<OMeasurement::MeasurementInvite>& invites) 
        : vInvites(invites) {}

    SERIALIZE_METHODS(CMeasureInv, obj)
    {
        READWRITE(obj.vInvites);
    }

    // Check if this message contains invites for a specific user
    bool HasInviteForUser(const CPubKey& user) const {
        for (const auto& invite : vInvites) {
            if (invite.invited_user == user) {
                return true;
            }
        }
        return false;
    }

    // Get invites for a specific user
    std::vector<OMeasurement::MeasurementInvite> GetInvitesForUser(const CPubKey& user) const {
        std::vector<OMeasurement::MeasurementInvite> user_invites;
        for (const auto& invite : vInvites) {
            if (invite.invited_user == user) {
                user_invites.push_back(invite);
            }
        }
        return user_invites;
    }
};

/**
 * CGetMeasureInv: Requests measurement invitations for a specific user
 * 
 * Sent by a node to request pending measurement invitations
 * for a user they control (identified by public key).
 */
class CGetMeasureInv
{
public:
    CPubKey user_pubkey;  // Public key of the user requesting invites
    uint64_t nonce;        // Nonce to prevent replay attacks

    CGetMeasureInv() : nonce(0) {}
    explicit CGetMeasureInv(const CPubKey& pubkey, uint64_t n = 0)
        : user_pubkey(pubkey), nonce(n) {}

    SERIALIZE_METHODS(CGetMeasureInv, obj)
    {
        READWRITE(obj.user_pubkey, obj.nonce);
    }
};

#endif // BITCOIN_O_PROTOCOL_MESSAGES_H

