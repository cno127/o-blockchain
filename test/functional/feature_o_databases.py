#!/usr/bin/env python3
# Copyright (c) 2025 The O Blockchain Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test O Blockchain database functionality."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than
import os

class OBlockchainDatabaseTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Starting O Blockchain database tests...")
        
        node = self.nodes[0]
        datadir = node.datadir_path
        
        # Test 1: Verify databases were created
        self.log.info("Test 1: Verify database directories exist")
        brightid_path = os.path.join(datadir, "regtest", "brightid_users")
        measurement_path = os.path.join(datadir, "regtest", "measurements")
        business_path = os.path.join(datadir, "regtest", "business_miners")
        
        assert os.path.exists(brightid_path), "BrightID database directory not found"
        assert os.path.exists(measurement_path), "Measurement database directory not found"
        assert os.path.exists(business_path), "Business database directory not found"
        self.log.info("✅ All 3 database directories exist")
        
        # Test 2: Verify LevelDB files
        self.log.info("Test 2: Verify LevelDB file structure")
        for db_name, db_path in [("BrightID", brightid_path), 
                                  ("Measurement", measurement_path), 
                                  ("Business", business_path)]:
            files = os.listdir(db_path)
            assert "CURRENT" in files, f"{db_name}: CURRENT file missing"
            assert "LOCK" in files, f"{db_name}: LOCK file missing"
            
            # Check for log or manifest files
            has_log = any(f.endswith('.log') for f in files)
            has_manifest = any(f.startswith('MANIFEST') for f in files)
            assert has_log or has_manifest, f"{db_name}: No LevelDB data files"
            
            self.log.info(f"✅ {db_name} database has valid LevelDB structure")
        
        # Test 3: Generate blocks and test persistence
        self.log.info("Test 3: Test blockchain persistence")
        initial_blocks = node.getblockcount()
        
        # Generate some blocks
        self.generate(node, 10)
        current_blocks = node.getblockcount()
        assert_equal(current_blocks, initial_blocks + 10)
        self.log.info(f"✅ Generated 10 blocks (total: {current_blocks})")
        
        # Test 4: Restart node and verify persistence
        self.log.info("Test 4: Restart node and verify persistence")
        self.restart_node(0)
        
        # Check block count persisted
        after_restart_blocks = node.getblockcount()
        assert_equal(after_restart_blocks, current_blocks)
        self.log.info(f"✅ Block count persisted after restart: {after_restart_blocks}")
        
        # Test 5: Verify databases still exist after restart
        self.log.info("Test 5: Verify databases reloaded after restart")
        assert os.path.exists(brightid_path), "BrightID database missing after restart"
        assert os.path.exists(measurement_path), "Measurement database missing after restart"
        assert os.path.exists(business_path), "Business database missing after restart"
        self.log.info("✅ All databases persisted and reloaded")
        
        # Test 6: Check debug logs for database initialization
        self.log.info("Test 6: Verify database initialization in logs")
        debug_log = os.path.join(datadir, "regtest", "debug.log")
        with open(debug_log, 'r') as f:
            log_contents = f.read()
            
            assert "O Measurement DB: Opened database" in log_contents, "Measurement DB not initialized"
            assert "O BrightID DB: Opened database" in log_contents, "BrightID DB not initialized"
            assert "O Business DB: Opened database" in log_contents, "Business DB not initialized"
            assert "O Blockchain databases initialized successfully" in log_contents, "Database initialization incomplete"
            
        self.log.info("✅ All databases initialized successfully in logs")
        
        # Test 7: Database size check
        self.log.info("Test 7: Check database sizes")
        for db_name, db_path in [("BrightID", brightid_path), 
                                  ("Measurement", measurement_path), 
                                  ("Business", business_path)]:
            total_size = 0
            for dirpath, dirnames, filenames in os.walk(db_path):
                for f in filenames:
                    fp = os.path.join(dirpath, f)
                    if os.path.exists(fp):
                        total_size += os.path.getsize(fp)
            
            self.log.info(f"✅ {db_name} database size: {total_size:,} bytes")
            assert total_size > 0, f"{db_name} database is empty"
        
        self.log.info("\n🎉 ALL O BLOCKCHAIN DATABASE TESTS PASSED!")

if __name__ == '__main__':
    OBlockchainDatabaseTest(__file__).main()

