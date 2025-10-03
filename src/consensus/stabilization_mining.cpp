// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/stabilization_mining.h>
#include <measurement/measurement_system.h>
#include <consensus/user_consensus.h>
#include <logging.h>
#include <random.h>
#include <util/time.h>

#include <algorithm>
#include <cmath>

namespace OConsensus {

StabilizationMining g_stabilization_mining;

StabilizationMining::StabilizationMining()
{
    m_stats.total_coins_created = 0;
    m_stats.total_transactions = 0;
    m_stats.total_recipients = 0;
    
    LogPrintf("O Stabilization Mining: Initialized\n");
}

// See stabilization_helpers.cpp for remaining implementation

} // namespace OConsensus
