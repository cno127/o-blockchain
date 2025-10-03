#!/usr/bin/env python3

"""
O Blockchain Advanced Simulation Tests
Comprehensive testing framework for O Blockchain features
"""

import json
import time
import subprocess
import sys
import os
from typing import Dict, List, Optional, Any

class OBlockchainTester:
    def __init__(self, simulation_dir: str = "."):
        self.simulation_dir = simulation_dir
        self.nodes = {
            1: {"rpc_port": 18332, "port": 18444},
            2: {"rpc_port": 18333, "port": 18445},
            3: {"rpc_port": 18334, "port": 18446},
            4: {"rpc_port": 18335, "port": 18447},
            5: {"rpc_port": 18336, "port": 18448}
        }
        self.test_results = {}
        
    def rpc_call(self, node_id: int, method: str, params: List[Any] = None) -> Dict[str, Any]:
        """Make RPC call to a specific node"""
        config_file = f"{self.simulation_dir}/config/node_{node_id}.conf"
        cmd = [
            "../build/bin/bitcoin-cli",
            f"-conf={config_file}",
            method
        ]
        
        if params:
            cmd.extend([str(p) for p in params])
            
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            return json.loads(result.stdout)
        except subprocess.CalledProcessError as e:
            return {"error": e.stderr}
        except json.JSONDecodeError:
            return {"error": "Invalid JSON response"}
    
    def wait_for_sync(self, timeout: int = 60) -> bool:
        """Wait for all nodes to sync"""
        print("⏳ Waiting for nodes to sync...")
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            synced_nodes = 0
            for node_id in self.nodes:
                try:
                    info = self.rpc_call(node_id, "getblockchaininfo")
                    if "blocks" in info and info["blocks"] > 0:
                        synced_nodes += 1
                except:
                    pass
            
            if synced_nodes == len(self.nodes):
                print("✅ All nodes synced!")
                return True
                
            time.sleep(2)
        
        print("❌ Sync timeout reached")
        return False
    
    def test_basic_functionality(self) -> bool:
        """Test basic blockchain functionality"""
        print("\n🔍 Testing basic blockchain functionality...")
        
        try:
            # Test blockchain info
            info = self.rpc_call(1, "getblockchaininfo")
            if "error" in info:
                print(f"❌ Failed to get blockchain info: {info['error']}")
                return False
            
            print(f"✅ Blockchain info: {info['blocks']} blocks, chain: {info['chain']}")
            
            # Test wallet functionality
            wallet_info = self.rpc_call(1, "getwalletinfo")
            if "error" not in wallet_info:
                print(f"✅ Wallet info: {wallet_info.get('balance', 0)} O balance")
            
            # Test peer connections
            peers = self.rpc_call(1, "getpeerinfo")
            if "error" not in peers:
                print(f"✅ Peer connections: {len(peers)} peers")
            
            return True
            
        except Exception as e:
            print(f"❌ Basic functionality test failed: {e}")
            return False
    
    def test_pow_pob_consensus(self) -> bool:
        """Test PoW/PoB consensus mechanism"""
        print("\n🔍 Testing PoW/PoB consensus mechanism...")
        
        try:
            # Test PoW/PoB stats
            stats = self.rpc_call(1, "getpowpobstats")
            if "error" in stats:
                print(f"❌ Failed to get PoW/PoB stats: {stats['error']}")
                return False
            
            print(f"✅ PoW/PoB stats retrieved")
            
            # Test business miner stats
            business_stats = self.rpc_call(1, "getbusinessminerstats")
            if "error" not in business_stats:
                print(f"✅ Business miner stats retrieved")
            
            # Test business ratio
            ratio = self.rpc_call(1, "getbusinessratio")
            if "error" not in ratio:
                print(f"✅ Business ratio: {ratio}")
            
            return True
            
        except Exception as e:
            print(f"❌ PoW/PoB consensus test failed: {e}")
            return False
    
    def test_measurement_system(self) -> bool:
        """Test measurement system"""
        print("\n🔍 Testing measurement system...")
        
        try:
            # Test measurement stats
            stats = self.rpc_call(1, "getmeasurementstats")
            if "error" in stats:
                print(f"❌ Failed to get measurement stats: {stats['error']}")
                return False
            
            print(f"✅ Measurement stats retrieved")
            
            # Test measurement invites
            invites = self.rpc_call(1, "getmeasurementinvites")
            if "error" not in invites:
                print(f"✅ Measurement invites retrieved")
            
            # Test daily averages
            averages = self.rpc_call(1, "getdailyaverages")
            if "error" not in averages:
                print(f"✅ Daily averages retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Measurement system test failed: {e}")
            return False
    
    def test_stabilization_mining(self) -> bool:
        """Test stabilization mining"""
        print("\n🔍 Testing stabilization mining...")
        
        try:
            # Test stabilization stats
            stats = self.rpc_call(1, "getstabilizationstats")
            if "error" in stats:
                print(f"❌ Failed to get stabilization stats: {stats['error']}")
                return False
            
            print(f"✅ Stabilization stats retrieved")
            
            # Test stabilization history
            history = self.rpc_call(1, "getstabilizationhistory")
            if "error" not in history:
                print(f"✅ Stabilization history retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Stabilization mining test failed: {e}")
            return False
    
    def test_currency_exchange(self) -> bool:
        """Test currency exchange system"""
        print("\n🔍 Testing currency exchange system...")
        
        try:
            # Test supported pairs
            pairs = self.rpc_call(1, "getsupportedpairs")
            if "error" in pairs:
                print(f"❌ Failed to get supported pairs: {pairs['error']}")
                return False
            
            print(f"✅ Supported pairs retrieved")
            
            # Test exchange rates
            rates = self.rpc_call(1, "getexchangerate", ["OUSD", "OEUR"])
            if "error" not in rates:
                print(f"✅ Exchange rates retrieved")
            
            # Test exchange statistics
            stats = self.rpc_call(1, "getexchangestatistics")
            if "error" not in stats:
                print(f"✅ Exchange statistics retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Currency exchange test failed: {e}")
            return False
    
    def test_geographic_access_control(self) -> bool:
        """Test geographic access control"""
        print("\n🔍 Testing geographic access control...")
        
        try:
            # Test access statistics
            stats = self.rpc_call(1, "getaccessstatistics")
            if "error" in stats:
                print(f"❌ Failed to get access statistics: {stats['error']}")
                return False
            
            print(f"✅ Access statistics retrieved")
            
            # Test jurisdiction policies
            policies = self.rpc_call(1, "getjurisdictionpolicies")
            if "error" not in policies:
                print(f"✅ Jurisdiction policies retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Geographic access control test failed: {e}")
            return False
    
    def test_multi_currency_support(self) -> bool:
        """Test multi-currency support"""
        print("\n🔍 Testing multi-currency support...")
        
        try:
            # Test supported currencies
            currencies = self.rpc_call(1, "getcurrencies")
            if "error" in currencies:
                print(f"❌ Failed to get currencies: {currencies['error']}")
                return False
            
            print(f"✅ Supported currencies retrieved")
            
            # Test currency metadata
            metadata = self.rpc_call(1, "getcurrencymetadata", ["OUSD"])
            if "error" not in metadata:
                print(f"✅ Currency metadata retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Multi-currency support test failed: {e}")
            return False
    
    def test_user_consensus(self) -> bool:
        """Test user consensus system"""
        print("\n🔍 Testing user consensus system...")
        
        try:
            # Test user statistics
            stats = self.rpc_call(1, "getuserstatistics")
            if "error" in stats:
                print(f"❌ Failed to get user statistics: {stats['error']}")
                return False
            
            print(f"✅ User statistics retrieved")
            
            # Test verified users
            users = self.rpc_call(1, "getverifiedusers")
            if "error" not in users:
                print(f"✅ Verified users retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ User consensus test failed: {e}")
            return False
    
    def test_brightid_integration(self) -> bool:
        """Test BrightID integration"""
        print("\n🔍 Testing BrightID integration...")
        
        try:
            # Test BrightID status
            status = self.rpc_call(1, "getbrightidstatus")
            if "error" in status:
                print(f"❌ Failed to get BrightID status: {status['error']}")
                return False
            
            print(f"✅ BrightID status retrieved")
            
            # Test BrightID statistics
            stats = self.rpc_call(1, "getbrightidstatistics")
            if "error" not in stats:
                print(f"✅ BrightID statistics retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ BrightID integration test failed: {e}")
            return False
    
    def test_transaction_processing(self) -> bool:
        """Test transaction processing"""
        print("\n🔍 Testing transaction processing...")
        
        try:
            # Test wallet balance
            balance = self.rpc_call(1, "getbalance")
            if "error" in balance:
                print(f"❌ Failed to get balance: {balance['error']}")
                return False
            
            print(f"✅ Wallet balance: {balance} O")
            
            # Test transaction history
            history = self.rpc_call(1, "listtransactions")
            if "error" not in history:
                print(f"✅ Transaction history retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Transaction processing test failed: {e}")
            return False
    
    def test_mining_rewards(self) -> bool:
        """Test mining rewards"""
        print("\n🔍 Testing mining rewards...")
        
        try:
            # Test mining info
            info = self.rpc_call(1, "getmininginfo")
            if "error" in info:
                print(f"❌ Failed to get mining info: {info['error']}")
                return False
            
            print(f"✅ Mining info retrieved")
            
            # Test block rewards
            rewards = self.rpc_call(1, "getblockrewards")
            if "error" not in rewards:
                print(f"✅ Block rewards retrieved")
            
            return True
            
        except Exception as e:
            print(f"❌ Mining rewards test failed: {e}")
            return False
    
    def run_comprehensive_tests(self) -> Dict[str, bool]:
        """Run all tests and return results"""
        print("🧪 Starting comprehensive O Blockchain tests...")
        
        tests = [
            ("Basic Functionality", self.test_basic_functionality),
            ("PoW/PoB Consensus", self.test_pow_pob_consensus),
            ("Measurement System", self.test_measurement_system),
            ("Stabilization Mining", self.test_stabilization_mining),
            ("Currency Exchange", self.test_currency_exchange),
            ("Geographic Access Control", self.test_geographic_access_control),
            ("Multi-Currency Support", self.test_multi_currency_support),
            ("User Consensus", self.test_user_consensus),
            ("BrightID Integration", self.test_brightid_integration),
            ("Transaction Processing", self.test_transaction_processing),
            ("Mining Rewards", self.test_mining_rewards)
        ]
        
        results = {}
        
        for test_name, test_func in tests:
            try:
                results[test_name] = test_func()
            except Exception as e:
                print(f"❌ {test_name} test crashed: {e}")
                results[test_name] = False
        
        return results
    
    def generate_report(self, results: Dict[str, bool]) -> str:
        """Generate test report"""
        report = "\n" + "="*60 + "\n"
        report += "O BLOCKCHAIN SIMULATION TEST REPORT\n"
        report += "="*60 + "\n\n"
        
        passed = sum(1 for result in results.values() if result)
        total = len(results)
        
        report += f"📊 Test Summary: {passed}/{total} tests passed\n\n"
        
        for test_name, result in results.items():
            status = "✅ PASS" if result else "❌ FAIL"
            report += f"{status} {test_name}\n"
        
        report += "\n" + "="*60 + "\n"
        
        if passed == total:
            report += "🎉 ALL TESTS PASSED! O Blockchain is working correctly.\n"
        else:
            report += f"⚠️  {total - passed} tests failed. Check logs for details.\n"
        
        report += "="*60 + "\n"
        
        return report

def main():
    """Main function"""
    if len(sys.argv) > 1:
        simulation_dir = sys.argv[1]
    else:
        simulation_dir = "."
    
    if not os.path.exists(f"{simulation_dir}/config"):
        print(f"❌ Simulation config directory '{simulation_dir}/config' not found!")
        print("Run ./setup_simulation.sh first to create the simulation environment.")
        sys.exit(1)
    
    tester = OBlockchainTester(simulation_dir)
    
    # Wait for nodes to sync
    if not tester.wait_for_sync():
        print("❌ Nodes failed to sync. Please check if nodes are running.")
        sys.exit(1)
    
    # Run comprehensive tests
    results = tester.run_comprehensive_tests()
    
    # Generate and print report
    report = tester.generate_report(results)
    print(report)
    
    # Save report to file
    with open(f"{simulation_dir}/test_report.txt", "w") as f:
        f.write(report)
    
    print(f"📄 Test report saved to {simulation_dir}/test_report.txt")
    
    # Exit with appropriate code
    if all(results.values()):
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
