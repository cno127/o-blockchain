#!/usr/bin/env python3

"""
O Blockchain Stress Testing Script
Tests the O Blockchain under extreme conditions
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

class OBlockchainStressTester:
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
        self.stress_level = 1
        
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
    
    def stress_test_worker(self, node_id: int, duration: int, stress_level: int):
        """Worker thread for stress testing"""
        start_time = time.time()
        operation_count = 0
        error_count = 0
        
        # Calculate operations per second based on stress level
        ops_per_second = stress_level * 10
        
        while time.time() - start_time < duration:
            try:
                # Random operation
                operation = random.choice([
                    "getblockchaininfo", "getpeerinfo", "getmininginfo",
                    "getwalletinfo", "getpowpobstats", "getmeasurementstats",
                    "getstabilizationstats", "getexchangerate", "getcurrencies"
                ])
                
                result = self.rpc_call(node_id, operation)
                
                if "error" in result:
                    error_count += 1
                else:
                    operation_count += 1
                
                # Rate limiting
                time.sleep(1.0 / ops_per_second)
                
            except Exception as e:
                error_count += 1
        
        return {
            "node_id": node_id,
            "operations": operation_count,
            "errors": error_count,
            "duration": duration,
            "stress_level": stress_level
        }
    
    def test_high_frequency_requests(self, duration: int = 60) -> Dict[str, Any]:
        """Test high frequency RPC requests"""
        print(f"🚀 Testing high frequency requests for {duration}s...")
        
        start_time = time.time()
        
        # Start stress test on all nodes
        with ThreadPoolExecutor(max_workers=len(self.nodes)) as executor:
            futures = []
            for node_id in self.nodes:
                future = executor.submit(self.stress_test_worker, node_id, duration, 5)
                futures.append(future)
            
            # Collect results
            results = []
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
                print(f"✅ Node {result['node_id']}: {result['operations']} ops, {result['errors']} errors")
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        # Calculate statistics
        total_operations = sum(r["operations"] for r in results)
        total_errors = sum(r["errors"] for r in results)
        error_rate = total_errors / (total_operations + total_errors) if (total_operations + total_errors) > 0 else 0
        
        return {
            "duration": actual_duration,
            "total_operations": total_operations,
            "total_errors": total_errors,
            "error_rate": error_rate,
            "operations_per_second": total_operations / actual_duration,
            "node_results": results
        }
    
    def test_concurrent_operations(self, duration: int = 60) -> Dict[str, Any]:
        """Test concurrent operations"""
        print(f"🚀 Testing concurrent operations for {duration}s...")
        
        start_time = time.time()
        
        # Start stress test on all nodes with higher concurrency
        with ThreadPoolExecutor(max_workers=len(self.nodes) * 2) as executor:
            futures = []
            for node_id in self.nodes:
                # Start multiple workers per node
                for _ in range(2):
                    future = executor.submit(self.stress_test_worker, node_id, duration, 3)
                    futures.append(future)
            
            # Collect results
            results = []
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        # Calculate statistics
        total_operations = sum(r["operations"] for r in results)
        total_errors = sum(r["errors"] for r in results)
        error_rate = total_errors / (total_operations + total_errors) if (total_operations + total_errors) > 0 else 0
        
        return {
            "duration": actual_duration,
            "total_operations": total_operations,
            "total_errors": total_errors,
            "error_rate": error_rate,
            "operations_per_second": total_operations / actual_duration,
            "node_results": results
        }
    
    def test_memory_stress(self, duration: int = 120) -> Dict[str, Any]:
        """Test memory stress"""
        print(f"🚀 Testing memory stress for {duration}s...")
        
        # Get initial memory stats
        initial_memory = {}
        for node_id in self.nodes:
            info = self.rpc_call(node_id, "getmemoryinfo")
            if "error" not in info:
                initial_memory[node_id] = info.get("locked", {}).get("used", 0)
        
        start_time = time.time()
        
        # Start stress test with memory-intensive operations
        with ThreadPoolExecutor(max_workers=len(self.nodes)) as executor:
            futures = []
            for node_id in self.nodes:
                future = executor.submit(self.memory_stress_worker, node_id, duration)
                futures.append(future)
            
            # Collect results
            results = []
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
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
            "duration": actual_duration,
            "initial_memory": initial_memory,
            "final_memory": final_memory,
            "memory_growth": memory_growth,
            "node_results": results
        }
    
    def memory_stress_worker(self, node_id: int, duration: int):
        """Worker for memory stress testing"""
        start_time = time.time()
        operation_count = 0
        
        while time.time() - start_time < duration:
            try:
                # Memory-intensive operations
                operations = [
                    "getblockchaininfo", "getpeerinfo", "getmininginfo",
                    "getwalletinfo", "listtransactions", "getrawmempool"
                ]
                
                operation = random.choice(operations)
                result = self.rpc_call(node_id, operation)
                
                if "error" not in result:
                    operation_count += 1
                
                # Shorter sleep for more intensive testing
                time.sleep(0.1)
                
            except Exception as e:
                pass
        
        return {
            "node_id": node_id,
            "operations": operation_count,
            "duration": duration
        }
    
    def test_network_stress(self, duration: int = 60) -> Dict[str, Any]:
        """Test network stress"""
        print(f"🚀 Testing network stress for {duration}s...")
        
        # Get initial network stats
        initial_stats = {}
        for node_id in self.nodes:
            peers = self.rpc_call(node_id, "getpeerinfo")
            if "error" not in peers:
                initial_stats[node_id] = len(peers)
        
        start_time = time.time()
        
        # Start network stress test
        with ThreadPoolExecutor(max_workers=len(self.nodes)) as executor:
            futures = []
            for node_id in self.nodes:
                future = executor.submit(self.network_stress_worker, node_id, duration)
                futures.append(future)
            
            # Collect results
            results = []
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        # Get final network stats
        final_stats = {}
        for node_id in self.nodes:
            peers = self.rpc_call(node_id, "getpeerinfo")
            if "error" not in peers:
                final_stats[node_id] = len(peers)
        
        return {
            "duration": actual_duration,
            "initial_stats": initial_stats,
            "final_stats": final_stats,
            "network_stable": all(final_stats.get(n, 0) == initial_stats.get(n, 0) for n in self.nodes),
            "node_results": results
        }
    
    def network_stress_worker(self, node_id: int, duration: int):
        """Worker for network stress testing"""
        start_time = time.time()
        operation_count = 0
        
        while time.time() - start_time < duration:
            try:
                # Network-intensive operations
                operations = [
                    "getpeerinfo", "getnetworkinfo", "getblockchaininfo",
                    "getmininginfo", "getwalletinfo"
                ]
                
                operation = random.choice(operations)
                result = self.rpc_call(node_id, operation)
                
                if "error" not in result:
                    operation_count += 1
                
                # Very short sleep for maximum stress
                time.sleep(0.05)
                
            except Exception as e:
                pass
        
        return {
            "node_id": node_id,
            "operations": operation_count,
            "duration": duration
        }
    
    def test_extreme_load(self, duration: int = 180) -> Dict[str, Any]:
        """Test extreme load conditions"""
        print(f"🚀 Testing extreme load for {duration}s...")
        
        start_time = time.time()
        
        # Start extreme load test
        with ThreadPoolExecutor(max_workers=len(self.nodes) * 3) as executor:
            futures = []
            for node_id in self.nodes:
                # Start multiple workers per node
                for _ in range(3):
                    future = executor.submit(self.stress_test_worker, node_id, duration, 10)
                    futures.append(future)
            
            # Collect results
            results = []
            for future in as_completed(futures):
                result = future.result()
                results.append(result)
        
        end_time = time.time()
        actual_duration = end_time - start_time
        
        # Calculate statistics
        total_operations = sum(r["operations"] for r in results)
        total_errors = sum(r["errors"] for r in results)
        error_rate = total_errors / (total_operations + total_errors) if (total_operations + total_errors) > 0 else 0
        
        return {
            "duration": actual_duration,
            "total_operations": total_operations,
            "total_errors": total_errors,
            "error_rate": error_rate,
            "operations_per_second": total_operations / actual_duration,
            "node_results": results
        }
    
    def run_comprehensive_stress_tests(self) -> Dict[str, Any]:
        """Run comprehensive stress tests"""
        print("🧪 Starting comprehensive stress tests...")
        
        results = {}
        
        # Test 1: High frequency requests
        results["high_frequency"] = self.test_high_frequency_requests(60)
        
        # Test 2: Concurrent operations
        results["concurrent"] = self.test_concurrent_operations(60)
        
        # Test 3: Memory stress
        results["memory_stress"] = self.test_memory_stress(120)
        
        # Test 4: Network stress
        results["network_stress"] = self.test_network_stress(60)
        
        # Test 5: Extreme load
        results["extreme_load"] = self.test_extreme_load(180)
        
        return results
    
    def generate_stress_report(self, results: Dict[str, Any]) -> str:
        """Generate stress test report"""
        report = "\n" + "="*60 + "\n"
        report += "O BLOCKCHAIN STRESS TEST REPORT\n"
        report += "="*60 + "\n\n"
        
        # High Frequency Requests
        if "high_frequency" in results:
            hf = results["high_frequency"]
            report += "⚡ HIGH FREQUENCY REQUESTS\n"
            report += f"Duration: {hf['duration']:.2f}s\n"
            report += f"Total Operations: {hf['total_operations']}\n"
            report += f"Operations/Second: {hf['operations_per_second']:.2f}\n"
            report += f"Error Rate: {hf['error_rate']:.2%}\n"
            report += f"Status: {'✅ PASS' if hf['error_rate'] < 0.1 else '❌ FAIL'}\n\n"
        
        # Concurrent Operations
        if "concurrent" in results:
            co = results["concurrent"]
            report += "🔄 CONCURRENT OPERATIONS\n"
            report += f"Duration: {co['duration']:.2f}s\n"
            report += f"Total Operations: {co['total_operations']}\n"
            report += f"Operations/Second: {co['operations_per_second']:.2f}\n"
            report += f"Error Rate: {co['error_rate']:.2%}\n"
            report += f"Status: {'✅ PASS' if co['error_rate'] < 0.15 else '❌ FAIL'}\n\n"
        
        # Memory Stress
        if "memory_stress" in results:
            ms = results["memory_stress"]
            report += "💾 MEMORY STRESS\n"
            report += f"Duration: {ms['duration']:.2f}s\n"
            report += "Memory Growth by Node:\n"
            for node_id, growth in ms["memory_growth"].items():
                report += f"  Node {node_id}: {growth} bytes\n"
            report += f"Status: {'✅ PASS' if all(g < 1000000 for g in ms['memory_growth'].values()) else '❌ FAIL'}\n\n"
        
        # Network Stress
        if "network_stress" in results:
            ns = results["network_stress"]
            report += "🌐 NETWORK STRESS\n"
            report += f"Duration: {ns['duration']:.2f}s\n"
            report += f"Network Stable: {'✅ Yes' if ns['network_stable'] else '❌ No'}\n"
            report += f"Status: {'✅ PASS' if ns['network_stable'] else '❌ FAIL'}\n\n"
        
        # Extreme Load
        if "extreme_load" in results:
            el = results["extreme_load"]
            report += "🔥 EXTREME LOAD\n"
            report += f"Duration: {el['duration']:.2f}s\n"
            report += f"Total Operations: {el['total_operations']}\n"
            report += f"Operations/Second: {el['operations_per_second']:.2f}\n"
            report += f"Error Rate: {el['error_rate']:.2%}\n"
            report += f"Status: {'✅ PASS' if el['error_rate'] < 0.2 else '❌ FAIL'}\n\n"
        
        report += "="*60 + "\n"
        
        # Overall assessment
        total_ops = sum(r["total_operations"] for r in results.values() if "total_operations" in r)
        avg_error_rate = sum(r["error_rate"] for r in results.values() if "error_rate" in r) / len([r for r in results.values() if "error_rate" in r])
        
        report += f"📊 OVERALL STRESS TEST RESULTS\n"
        report += f"Total Operations: {total_ops}\n"
        report += f"Average Error Rate: {avg_error_rate:.2%}\n"
        
        if avg_error_rate < 0.1:
            report += "🎉 EXCELLENT STRESS RESISTANCE!\n"
        elif avg_error_rate < 0.2:
            report += "✅ GOOD STRESS RESISTANCE\n"
        elif avg_error_rate < 0.3:
            report += "⚠️  MODERATE STRESS RESISTANCE\n"
        else:
            report += "❌ POOR STRESS RESISTANCE\n"
        
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
    
    tester = OBlockchainStressTester(simulation_dir)
    
    # Run comprehensive stress tests
    results = tester.run_comprehensive_stress_tests()
    
    # Generate and print report
    report = tester.generate_stress_report(results)
    print(report)
    
    # Save report to file
    with open(f"{simulation_dir}/stress_test_report.txt", "w") as f:
        f.write(report)
    
    print(f"📄 Stress test report saved to {simulation_dir}/stress_test_report.txt")

if __name__ == "__main__":
    main()
