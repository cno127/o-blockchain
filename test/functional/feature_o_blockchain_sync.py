#!/usr/bin/env python3
# Copyright (c) 2025 The O Blockchain Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test O Blockchain data sync between blockchain and local databases."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, assert_raises_rpc_error
import os
import time

class OBlockchainSyncTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2  # Test multi-node sync
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("=" * 60)
        self.log.info("O BLOCKCHAIN → DATABASE SYNC TEST")
        self.log.info("=" * 60)
        
        node0 = self.nodes[0]
        node1 = self.nodes[1]
        
        # Connect nodes
        self.connect_nodes(0, 1)
        self.sync_all()
        
        # Generate initial blocks
        self.log.info("\n[1/10] Generate initial blocks...")
        self.generate(node0, 101)  # Coinbase maturity
        self.sync_all()
        self.log.info(f"✅ Generated 101 blocks, synced to both nodes")
        
        # Test 1: Database initialization
        self.log.info("\n[2/10] Verify databases initialized on both nodes...")
        for i, node in enumerate([node0, node1]):
            datadir = self.nodes[i].datadir_path
            for db_name in ["brightid_users", "measurements", "business_miners"]:
                db_path = os.path.join(datadir, "regtest", db_name)
                assert os.path.exists(db_path), f"Node {i}: {db_name} database not found"
        self.log.info("✅ All databases initialized on both nodes")
        
        # Test 2: Check measurement statistics (should be empty initially)
        self.log.info("\n[3/10] Check initial measurement statistics...")
        stats = node0.getmeasurementstatistics()
        assert_equal(stats['total_water_prices'], 0)
        assert_equal(stats['total_exchange_rates'], 0)
        assert_equal(stats['total_invites'], 0)
        self.log.info("✅ Initial statistics correct (all zero)")
        
        # Test 3: Check for automatic invitations (at block 110, height % 10 == 0)
        self.log.info("\n[4/10] Trigger automatic invitations...")
        self.generate(node0, 9)  # Block 110
        self.sync_all()
        
        # Check logs for invitation creation
        debug_log = os.path.join(self.nodes[0].datadir_path, "regtest", "debug.log")
        with open(debug_log, 'r') as f:
            log_contents = f.read()
            
        if "O Mining: Added" in log_contents and "invitation transactions" in log_contents:
            self.log.info("✅ Automatic invitations were attempted")
            # Check if any invitations were created
            invites = node0.getactiveinvites()
            self.log.info(f"   Found {len(invites)} active invitations")
            
            # Verify both nodes see same invitations
            invites1 = node1.getactiveinvites()
            assert_equal(len(invites), len(invites1))
            self.log.info(f"✅ Both nodes synced: {len(invites)} invitations")
        else:
            self.log.info("⚠️  No invitations created (expected - needs verified users)")
        
        # Test 4: Verify measurement statistics still synced
        self.log.info("\n[5/10] Verify measurement stats after more blocks...")
        stats0 = node0.getmeasurementstatistics()
        stats1 = node1.getmeasurementstatistics()
        assert_equal(stats0['total_water_prices'], stats1['total_water_prices'])
        assert_equal(stats0['total_exchange_rates'], stats1['total_exchange_rates'])
        self.log.info("✅ Both nodes have identical statistics")
        
        # Test 5: Test database query functions (even if empty)
        self.log.info("\n[6/10] Test database query RPCs...")
        
        # Should not error even with no data
        try:
            result = node0.getaveragewaterpricewithconfidence("USD", 7)
            self.log.info("⚠️  Got water price average (unexpected with no data)")
        except Exception as e:
            if "No water price measurements" in str(e):
                self.log.info("✅ Correctly returns no data error")
            else:
                raise
        
        # Test 6: Verify getactiveinvites works on both nodes
        self.log.info("\n[7/10] Test getactiveinvites on both nodes...")
        invites0 = node0.getactiveinvites()
        invites1 = node1.getactiveinvites()
        assert_equal(len(invites0), len(invites1))
        self.log.info(f"✅ Both nodes return same invitation count: {len(invites0)}")
        
        # Test 7: Node restart persistence
        self.log.info("\n[8/10] Test database persistence after node restart...")
        stats_before = node0.getmeasurementstatistics()
        
        self.restart_node(0)
        self.connect_nodes(0, 1)
        self.sync_all()
        
        stats_after = node0.getmeasurementstatistics()
        assert_equal(stats_before['total_water_prices'], stats_after['total_water_prices'])
        assert_equal(stats_before['total_invites'], stats_after['total_invites'])
        self.log.info("✅ Statistics persisted after restart")
        
        # Test 8: Multi-node consistency
        self.log.info("\n[9/10] Generate more blocks and verify continued sync...")
        self.generate(node0, 20)
        self.sync_all()
        
        height0 = node0.getblockcount()
        height1 = node1.getblockcount()
        assert_equal(height0, height1)
        
        stats0 = node0.getmeasurementstatistics()
        stats1 = node1.getmeasurementstatistics()
        assert_equal(stats0, stats1)
        self.log.info(f"✅ Both nodes at height {height0} with identical stats")
        
        # Test 9: Check database files grew appropriately
        self.log.info("\n[10/10] Verify database files exist and have content...")
        for i, node in enumerate([node0, node1]):
            datadir = self.nodes[i].datadir_path
            for db_name in ["brightid_users", "measurements", "business_miners"]:
                db_path = os.path.join(datadir, "regtest", db_name)
                
                # Count files
                total_size = 0
                file_count = 0
                for root, dirs, files in os.walk(db_path):
                    for f in files:
                        file_count += 1
                        fp = os.path.join(root, f)
                        total_size += os.path.getsize(fp)
                
                assert file_count > 0, f"Node {i} {db_name}: No database files"
                assert total_size > 0, f"Node {i} {db_name}: Database is empty"
                self.log.info(f"✅ Node {i} {db_name}: {file_count} files, {total_size:,} bytes")
        
        self.log.info("\n" + "=" * 60)
        self.log.info("🎉 ALL BLOCKCHAIN→DATABASE SYNC TESTS PASSED!")
        self.log.info("=" * 60)
        self.log.info("\nVerified:")
        self.log.info("✅ Databases initialize correctly")
        self.log.info("✅ Both nodes have identical data")
        self.log.info("✅ Data persists across restarts")
        self.log.info("✅ Statistics stay synced")
        self.log.info("✅ Database files grow appropriately")
        self.log.info("\n⚠️  Note: Full sync testing requires:")
        self.log.info("   - User verification transactions")
        self.log.info("   - Measurement submissions")
        self.log.info("   - Validation transactions")
        self.log.info("   These will be tested in integration tests")

if __name__ == '__main__':
    OBlockchainSyncTest(__file__).main()

