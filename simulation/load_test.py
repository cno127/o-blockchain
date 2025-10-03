#!/usr/bin/env python3

"""
O Blockchain Load Testing Script
Tests the O Blockchain under various load conditions
"""

import json
import time
import subprocess
import threading
import random
import sys
import os
from typing import Dict, List, Optional, Any
from concurrent.futures import ThreadPoolExecutor, as_completed

class OBlockchainLoadTester:
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
        self.transaction_count = 0
        self.measurement_count = 0
        self.exchange_count = 0
        
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
    
    def generate_random_transaction(self, node_id: int) -> bool:
        """Generate a random transaction"""
        try:
            # Get a random target node
            target_node = random.choice([n for n in self.nodes.keys() if n != node_id])
            
            # Generate random amount (0.01 to 10.00 O)
            amount = random.uniform(0.01, 10.00)
            
            # Get target address
            target_address = self.rpc_call(target_node, "getnewaddress")
            if "error" in target_address:
                return False
            
            # Send transaction
            tx_result = self.rpc_call(node_id, "sendtoaddress", [target_address, amount])
            if "error" not in tx_result:
                self.transaction_count += 1
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Transaction generation failed: {e}")
            return False
    
    def generate_random_measurement(self, node_id: int) -> bool:
        """Generate a random measurement"""
        try:
            # Random measurement type
            measurement_types = ["WATER_PRICE", "EXCHANGE_RATE"]
            measurement_type = random.choice(measurement_types)
            
            # Random currency
            currencies = ["OUSD", "OEUR", "OJPY", "OGBP", "OCAD"]
            currency = random.choice(currencies)
            
            # Random value
            if measurement_type == "WATER_PRICE":
                value = random.uniform(0.5, 5.0)  # Water price in O
            else:
                value = random.uniform(0.8, 1.2)  # Exchange rate
            
            # Submit measurement
            result = self.rpc_call(node_id, "submitmeasurement", [
                measurement_type, currency, value, "Load test measurement"
            ])
            
            if "error" not in result:
                self.measurement_count += 1
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Measurement generation failed: {e}")
            return False
    
    def generate_random_exchange(self, node_id: int) -> bool:
        """Generate a random currency exchange"""
        try:
            # Random currency pair
            currencies = ["OUSD", "OEUR", "OJPY", "OGBP", "OCAD"]
            from_currency = random.choice(currencies)
            to_currency = random.choice([c for c in currencies if c != from_currency])
            
            # Random amount
            amount = random.uniform(1.0, 100.0)
            
            # Get exchange rate
            rate_result = self.rpc_call(node_id, "getexchangerate", [from_currency, to_currency])
            if "error" in rate_result:
                return False
            
            # Execute exchange
            exchange_result = self.rpc_call(node_id, "exchangecurrency", [
                from_currency, to_currency, amount, "Load test exchange"
            ])
            
            if "error" not in exchange_result:
                self.exchange_count += 1
                return True
            
            return False
            
        except Exception as e:
            print(f"❌ Exchange generation failed: {e}")
            return False
    
    def load_test_worker(self, node_id: int, duration: int, operations_per_second: int):
        """Worker thread for load testing"""
        start_time = time.time()
        operation_count = 0
        
        while time.time() - start_time < duration:
            # Random operation
            operation = random.choice(["transaction", "measurement", "exchange"])
            
            success = False
            if operation == "transaction":
                success = self.generate_random_transaction(node_id)
            elif operation == "measurement":
                success = self.generate_random_measurement(node_id)
            elif operation == "exchange":
                success = self.generate_random_exchange(node_id)
            
            if success:
                operation_count += 1
            
            # Rate limiting
            time.sleep(1.0 / operations_per_second)
        
        return {
            "node_id": node_id,
            "operations": operation_count,
            "duration": duration
        }
    
    def run_load_test(self, duration: int = 60, operations_per_second: int = 10) -> Dict[str, Any]:
        """Run load test on all nodes"""
        print(f"🚀 Starting load test: {duration}s, {operations_per_second} ops/sec per node")
        
        start_time = time.time()
        
        # Reset counters
        self.transaction_count = 0
        self.measurement_count = 0
        self.exchange_count = 0
        
        # Start load test on all nodes
        with ThreadPoolExecutor(max_workers=len(self.nodes)) as executor:
            futures = []
            for node_id in self.nodes:
                future = executor.submit(self.load_test_worker, node_id, duration, operations_per_second)
                futures.append(future)
            
            # Collect results
            results = []
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
                print(f"✅ Node {result['node_id']}: {result['operations']} operations")
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        # Calculate statistics
        total_operations = sum(r["operations"] for r in results)
        actual_ops_per_second = total_operations / actual_duration
        
        return {
            "duration": actual_duration,
            "total_operations": total_operations,
            "operations_per_second": actual_ops_per_second,
            "transactions": self.transaction_count,
            "measurements": self.measurement_count,
            "exchanges": self.exchange_count,
            "node_results": results
        }
    
    def test_network_performance(self) -> Dict[str, Any]:
        """Test network performance under load"""
        print("\n🔍 Testing network performance...")
        
        # Get initial network stats
        initial_stats = {}
        for node_id in self.nodes:
            peers = self.rpc_call(node_id, "getpeerinfo")
            if "error" not in peers:
                initial_stats[node_id] = len(peers)
        
        # Run load test
        load_results = self.run_load_test(duration=30, operations_per_second=5)
        
        # Get final network stats
        final_stats = {}
        for node_id in self.nodes:
            peers = self.rpc_call(node_id, "getpeerinfo")
            if "error" not in peers:
                final_stats[node_id] = len(peers)
        
        return {
            "load_results": load_results,
            "initial_peers": initial_stats,
            "final_peers": final_stats,
            "network_stable": all(final_stats.get(n, 0) == initial_stats.get(n, 0) for n in self.nodes)
        }
    
    def test_memory_usage(self) -> Dict[str, Any]:
        """Test memory usage under load"""
        print("\n🔍 Testing memory usage...")
        
        # Get initial memory stats
        initial_memory = {}
        for node_id in self.nodes:
            info = self.rpc_call(node_id, "getmemoryinfo")
            if "error" not in info:
                initial_memory[node_id] = info.get("locked", {}).get("used", 0)
        
        # Run load test
        load_results = self.run_load_test(duration=60, operations_per_second=10)
        
        # Get final memory stats
        final_memory = {}
        for node_id in self.nodes:
            info = self.rpc_call(node_id, "getmemoryinfo")
            if "error" not in info:
                final_memory[node_id] = info.get("locked", {}).get("used", 0)
        
        # Calculate memory growth
        memory_growth = {}
        for node_id in self.nodes:
            initial = initial_memory.get(node_id, 0)
            final = final_memory.get(node_id, 0)
            memory_growth[node_id] = final - initial
        
        return {
            "load_results": load_results,
            "initial_memory": initial_memory,
            "final_memory": final_memory,
            "memory_growth": memory_growth
        }
    
    def test_block_production(self) -> Dict[str, Any]:
        """Test block production under load"""
        print("\n🔍 Testing block production...")
        
        # Get initial block count
        initial_blocks = {}
        for node_id in self.nodes:
            info = self.rpc_call(node_id, "getblockchaininfo")
            if "error" not in info:
                initial_blocks[node_id] = info.get("blocks", 0)
        
        # Run load test
        load_results = self.run_load_test(duration=120, operations_per_second=15)
        
        # Get final block count
        final_blocks = {}
        for node_id in self.nodes:
            info = self.rpc_call(node_id, "getblockchaininfo")
            if "error" not in info:
                final_blocks[node_id] = info.get("blocks", 0)
        
        # Calculate block production
        blocks_produced = {}
        for node_id in self.nodes:
            initial = initial_blocks.get(node_id, 0)
            final = final_blocks.get(node_id, 0)
            blocks_produced[node_id] = final - initial
        
        return {
            "load_results": load_results,
            "initial_blocks": initial_blocks,
            "final_blocks": final_blocks,
            "blocks_produced": blocks_produced
        }
    
    def run_comprehensive_load_tests(self) -> Dict[str, Any]:
        """Run comprehensive load tests"""
        print("🧪 Starting comprehensive load tests...")
        
        results = {}
        
        # Test 1: Network performance
        results["network_performance"] = self.test_network_performance()
        
        # Test 2: Memory usage
        results["memory_usage"] = self.test_memory_usage()
        
        # Test 3: Block production
        results["block_production"] = self.test_block_production()
        
        return results
    
    def generate_load_report(self, results: Dict[str, Any]) -> str:
        """Generate load test report"""
        report = "\n" + "="*60 + "\n"
        report += "O BLOCKCHAIN LOAD TEST REPORT\n"
        report += "="*60 + "\n\n"
        
        # Network Performance
        if "network_performance" in results:
            np = results["network_performance"]
            report += "🌐 NETWORK PERFORMANCE\n"
            report += f"Total Operations: {np['load_results']['total_operations']}\n"
            report += f"Operations/Second: {np['load_results']['operations_per_second']:.2f}\n"
            report += f"Network Stable: {'✅ Yes' if np['network_stable'] else '❌ No'}\n"
            report += f"Transactions: {np['load_results']['transactions']}\n"
            report += f"Measurements: {np['load_results']['measurements']}\n"
            report += f"Exchanges: {np['load_results']['exchanges']}\n\n"
        
        # Memory Usage
        if "memory_usage" in results:
            mu = results["memory_usage"]
            report += "💾 MEMORY USAGE\n"
            report += f"Total Operations: {mu['load_results']['total_operations']}\n"
            report += f"Operations/Second: {mu['load_results']['operations_per_second']:.2f}\n"
            report += "Memory Growth by Node:\n"
            for node_id, growth in mu["memory_growth"].items():
                report += f"  Node {node_id}: {growth} bytes\n"
            report += "\n"
        
        # Block Production
        if "block_production" in results:
            bp = results["block_production"]
            report += "⛏️  BLOCK PRODUCTION\n"
            report += f"Total Operations: {bp['load_results']['total_operations']}\n"
            report += f"Operations/Second: {bp['load_results']['operations_per_second']:.2f}\n"
            report += "Blocks Produced by Node:\n"
            for node_id, blocks in bp["blocks_produced"].items():
                report += f"  Node {node_id}: {blocks} blocks\n"
            report += "\n"
        
        report += "="*60 + "\n"
        
        # Overall assessment
        total_ops = sum(r["load_results"]["total_operations"] for r in results.values())
        avg_ops_per_sec = sum(r["load_results"]["operations_per_second"] for r in results.values()) / len(results)
        
        report += f"📊 OVERALL PERFORMANCE\n"
        report += f"Total Operations: {total_ops}\n"
        report += f"Average Ops/Second: {avg_ops_per_sec:.2f}\n"
        
        if avg_ops_per_sec > 50:
            report += "🎉 EXCELLENT PERFORMANCE!\n"
        elif avg_ops_per_sec > 20:
            report += "✅ GOOD PERFORMANCE\n"
        elif avg_ops_per_sec > 10:
            report += "⚠️  MODERATE PERFORMANCE\n"
        else:
            report += "❌ POOR PERFORMANCE\n"
        
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
    
    tester = OBlockchainLoadTester(simulation_dir)
    
    # Run comprehensive load tests
    results = tester.run_comprehensive_load_tests()
    
    # Generate and print report
    report = tester.generate_load_report(results)
    print(report)
    
    # Save report to file
    with open(f"{simulation_dir}/load_test_report.txt", "w") as f:
        f.write(report)
    
    print(f"📄 Load test report saved to {simulation_dir}/load_test_report.txt")

if __name__ == "__main__":
    main()
