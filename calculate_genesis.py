#!/usr/bin/env python3
"""
O Blockchain Genesis Block Calculator

This script helps calculate the proper nonce for the O Blockchain genesis block
to ensure it meets the difficulty requirements.
"""

import hashlib
import struct
import time

def double_sha256(data):
    """Calculate double SHA256 hash"""
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()

def calculate_genesis_hash(timestamp, nonce, bits, version, merkle_root, prev_block_hash):
    """Calculate genesis block hash"""
    # Block header structure (80 bytes)
    header = struct.pack('<I', version)  # Version (4 bytes)
    header += prev_block_hash[::-1]      # Previous block hash (32 bytes, reversed)
    header += merkle_root[::-1]          # Merkle root (32 bytes, reversed)
    header += struct.pack('<I', timestamp)  # Timestamp (4 bytes)
    header += struct.pack('<I', bits)    # Bits (4 bytes)
    header += struct.pack('<I', nonce)   # Nonce (4 bytes)
    
    return double_sha256(header)

def check_difficulty(hash_bytes, bits):
    """Check if hash meets difficulty requirement"""
    # Convert bits to target
    exponent = bits >> 24
    mantissa = bits & 0xffffff
    
    if exponent <= 3:
        target = mantissa >> (8 * (3 - exponent))
    else:
        target = mantissa << (8 * (exponent - 3))
    
    # Convert hash to integer
    hash_int = int.from_bytes(hash_bytes, byteorder='big')
    
    return hash_int < target

def find_genesis_nonce():
    """Find valid nonce for genesis block"""
    print("🌊 O Blockchain Genesis Block Calculator")
    print("=" * 50)
    
    # Genesis block parameters
    timestamp = 1735689600  # 2025-01-01 00:00:00 UTC
    bits = 0x1d00ffff       # Bitcoin difficulty
    version = 1
    
    # Placeholder values (these would be calculated from the actual genesis transaction)
    prev_block_hash = b'\x00' * 32  # Genesis has no previous block
    merkle_root = b'\x00' * 32      # Placeholder - needs actual transaction merkle root
    
    print(f"Timestamp: {timestamp} ({time.ctime(timestamp)})")
    print(f"Bits: 0x{bits:08x}")
    print(f"Version: {version}")
    print(f"Previous Block Hash: {'00' * 32}")
    print(f"Merkle Root: {'00' * 32} (placeholder)")
    print()
    
    print("Searching for valid nonce...")
    print("This may take a while...")
    
    nonce = 0
    start_time = time.time()
    
    while nonce < 0xffffffff:  # 32-bit nonce
        if nonce % 1000000 == 0:
            elapsed = time.time() - start_time
            print(f"Checked {nonce:,} nonces in {elapsed:.1f} seconds...")
        
        hash_bytes = calculate_genesis_hash(timestamp, nonce, bits, version, merkle_root, prev_block_hash)
        
        if check_difficulty(hash_bytes, bits):
            hash_hex = hash_bytes[::-1].hex()  # Reverse for display
            print()
            print("🎉 Found valid genesis block!")
            print(f"Nonce: {nonce}")
            print(f"Hash: {hash_hex}")
            print(f"Time taken: {time.time() - start_time:.1f} seconds")
            return nonce, hash_hex
        
        nonce += 1
    
    print("❌ No valid nonce found in 32-bit range")
    return None, None

def main():
    """Main function"""
    print("Note: This is a simplified genesis block calculator.")
    print("For the actual O Blockchain, you'll need to:")
    print("1. Create the actual genesis transaction")
    print("2. Calculate the real merkle root")
    print("3. Use those values in this calculation")
    print()
    
    nonce, hash_hex = find_genesis_nonce()
    
    if nonce is not None:
        print()
        print("To use this in your code:")
        print(f"uint32_t genesis_nonce = {nonce};")
        print(f"// Genesis hash: {hash_hex}")
        print()
        print("Add these assertions to chainparams.cpp:")
        print(f'assert(consensus.hashGenesisBlock == uint256{{"{hash_hex}"}});')
        print(f'assert(genesis.hashMerkleRoot == uint256{{"<merkle_root_here>"}});')

if __name__ == "__main__":
    main()
