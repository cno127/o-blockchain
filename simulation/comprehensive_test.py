#!/usr/bin/env python3
"""
O Blockchain Comprehensive Test Suite
Tests all implemented features of the O Blockchain
"""

import subprocess
import json
import time
import sys
import os

class OBlockchainTester:
    def __init__(self, simulation_dir="."):
        self.simulation_dir = simulation_dir
        self.config_dir = f"{simulation_dir}/config"
        self.results = {
            "basic_functionality": {},
            "pow_pob_system": {},
            "measurement_system": {},
            "stabilization_system": {},
            "currency_exchange": {},
            "integration_tests": {}
        }
        
    def rpc_call(self, node_id, method, params=None):
        """Make an RPC call to a specific node"""
        config_file = f"{self.config_dir}/node_{node_id}.conf"
        cmd = [f"{self.simulation_dir}/../build/bin/bitcoin-cli", 
               f"-conf={os.path.abspath(config_file)}", method]
        
        if params:
            if isinstance(params, list):
                cmd.extend([str(p) for p in params])
            else:
                cmd.append(str(params))
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                try:
                    return json.loads(result.stdout)
                except json.JSONDecodeError:
                    return result.stdout.strip()
            else:
                return {"error": result.stderr.strip()}
        except subprocess.TimeoutExpired:
            return {"error": "Timeout"}
        except Exception as e:
            return {"error": str(e)}
    
    def test_basic_functionality(self):
        """Test basic Bitcoin Core functionality"""
        print("🔍 Testing Basic Functionality...")
        
        # Test blockchain info
        result = self.rpc_call(1, "getblockchaininfo")
        if "error" not in result:
            self.results["basic_functionality"]["blockchain_info"] = "✅ PASS"
            print(f"   ✅ Blockchain info: {result.get('blocks', 0)} blocks")
        else:
            self.results["basic_functionality"]["blockchain_info"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Blockchain info failed: {result['error']}")
        
        # Test mining info
        result = self.rpc_call(1, "getmininginfo")
        if "error" not in result:
            self.results["basic_functionality"]["mining_info"] = "✅ PASS"
            print(f"   ✅ Mining info: {result.get('blocks', 0)} blocks")
        else:
            self.results["basic_functionality"]["mining_info"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Mining info failed: {result['error']}")
        
        # Test wallet functionality
        result = self.rpc_call(1, "getnewaddress")
        if "error" not in result:
            self.results["basic_functionality"]["wallet"] = "✅ PASS"
            print(f"   ✅ Wallet: Generated address {result[:20]}...")
        else:
            self.results["basic_functionality"]["wallet"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Wallet failed: {result['error']}")
    
    def test_pow_pob_system(self):
        """Test PoW/PoB consensus system"""
        print("⛏️  Testing PoW/PoB System...")
        
        # Test business ratio
        result = self.rpc_call(1, "getbusinessratio")
        if "error" not in result:
            self.results["pow_pob_system"]["business_ratio"] = "✅ PASS"
            print(f"   ✅ Business ratio: {result.get('business_ratio', 0)}")
        else:
            self.results["pow_pob_system"]["business_ratio"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Business ratio failed: {result['error']}")
        
        # Test business miner stats
        result = self.rpc_call(1, "getbusinessminerstats")
        if "error" not in result:
            self.results["pow_pob_system"]["business_stats"] = "✅ PASS"
            print(f"   ✅ Business miner stats: Available")
        else:
            self.results["pow_pob_system"]["business_stats"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Business miner stats failed: {result['error']}")
    
    def test_measurement_system(self):
        """Test measurement system"""
        print("📊 Testing Measurement System...")
        
        # Test create invites
        result = self.rpc_call(1, "createinvites", ["OUSD", 5])
        if "error" not in result:
            self.results["measurement_system"]["create_invites"] = "✅ PASS"
            print(f"   ✅ Create invites: {result}")
        else:
            self.results["measurement_system"]["create_invites"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Create invites failed: {result['error']}")
        
        # Test measurement statistics
        result = self.rpc_call(1, "getmeasurementstatistics")
        if "error" not in result:
            self.results["measurement_system"]["statistics"] = "✅ PASS"
            print(f"   ✅ Measurement statistics: Available")
        else:
            self.results["measurement_system"]["statistics"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Measurement statistics failed: {result['error']}")
    
    def test_stabilization_system(self):
        """Test stabilization mining system"""
        print("🎯 Testing Stabilization System...")
        
        # Test stabilization status
        result = self.rpc_call(1, "getstabilizationstatus")
        if "error" not in result:
            self.results["stabilization_system"]["status"] = "✅ PASS"
            print(f"   ✅ Stabilization status: Available")
        else:
            self.results["stabilization_system"]["status"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Stabilization status failed: {result['error']}")
        
        # Test stabilization integration
        result = self.rpc_call(1, "getstabilizationintegrationstatus")
        if "error" not in result:
            self.results["stabilization_system"]["integration"] = "✅ PASS"
            print(f"   ✅ Stabilization integration: Available")
        else:
            self.results["stabilization_system"]["integration"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Stabilization integration failed: {result['error']}")
    
    def test_currency_exchange(self):
        """Test currency exchange system"""
        print("💱 Testing Currency Exchange...")
        
        # Test supported pairs
        result = self.rpc_call(1, "getsupportedpairs")
        if "error" not in result:
            self.results["currency_exchange"]["supported_pairs"] = "✅ PASS"
            print(f"   ✅ Supported pairs: Available")
        else:
            self.results["currency_exchange"]["supported_pairs"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Supported pairs failed: {result['error']}")
        
        # Test exchange rate
        result = self.rpc_call(1, "getexchangerate", ["OUSD", "OEUR"])
        if "error" not in result:
            self.results["currency_exchange"]["exchange_rate"] = "✅ PASS"
            print(f"   ✅ Exchange rate: Available")
        else:
            self.results["currency_exchange"]["exchange_rate"] = f"❌ FAIL: {result['error']}"
            print(f"   ❌ Exchange rate failed: {result['error']}")
    
    def test_integration(self):
        """Test integration between systems"""
        print("🔗 Testing Integration...")
        
        # Test block generation with O Blockchain features
        address = self.rpc_call(1, "getnewaddress")
        if "error" not in address:
            result = self.rpc_call(1, "generatetoaddress", [1, address])
            if "error" not in result:
                self.results["integration_tests"]["block_generation"] = "✅ PASS"
                print(f"   ✅ Block generation: {result}")
            else:
                self.results["integration_tests"]["block_generation"] = f"❌ FAIL: {result['error']}"
                print(f"   ❌ Block generation failed: {result['error']}")
        else:
            self.results["integration_tests"]["block_generation"] = f"❌ FAIL: {address['error']}"
            print(f"   ❌ Block generation failed: {address['error']}")
    
    def run_comprehensive_test(self):
        """Run all tests"""
        print("🌊 O Blockchain Comprehensive Test Suite")
        print("=" * 50)
        
        # Test basic functionality
        self.test_basic_functionality()
        print()
        
        # Test PoW/PoB system
        self.test_pow_pob_system()
        print()
        
        # Test measurement system
        self.test_measurement_system()
        print()
        
        # Test stabilization system
        self.test_stabilization_system()
        print()
        
        # Test currency exchange
        self.test_currency_exchange()
        print()
        
        # Test integration
        self.test_integration()
        print()
        
        # Print summary
        self.print_summary()
    
    def print_summary(self):
        """Print test summary"""
        print("📊 Test Summary")
        print("=" * 50)
        
        total_tests = 0
        passed_tests = 0
        
        for category, tests in self.results.items():
            print(f"\n{category.replace('_', ' ').title()}:")
            for test_name, result in tests.items():
                total_tests += 1
                if "✅ PASS" in result:
                    passed_tests += 1
                print(f"  {test_name}: {result}")
        
        print(f"\n🎯 Overall Results: {passed_tests}/{total_tests} tests passed")
        
        if passed_tests == total_tests:
            print("🎉 All tests passed! O Blockchain is working perfectly!")
        elif passed_tests > total_tests * 0.8:
            print("✅ Most tests passed! O Blockchain is mostly functional!")
        else:
            print("⚠️  Some tests failed. Check the implementation.")

if __name__ == "__main__":
    tester = OBlockchainTester()
    tester.run_comprehensive_test()
