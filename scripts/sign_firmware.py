#!/usr/bin/env python3
"""
Meantendo Firmware Signing Tool
================================
Signs ESP32 firmware binaries using ECDSA P-256 for OTA verification.

Usage:
    python sign_firmware.py --input firmware.bin --key private_key.pem --output firmware_signed.bin

Author: Debyte
Version: 1.0.0
"""

import argparse
import hashlib
import struct
import sys
import os
from pathlib import Path

try:
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import ec
    from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
    from cryptography.hazmat.primitives.serialization import (
        load_pem_private_key,
        load_pem_public_key,
        Encoding,
        PublicFormat
    )
    from cryptography.hazmat.backends import default_backend
except ImportError:
    print("Error: cryptography package required. Install with: pip install cryptography")
    sys.exit(1)


# Signature magic bytes
SIGNATURE_MAGIC = b'MNTD'  # Meantendo signature header
SIGNATURE_VERSION = 1


def generate_keypair(output_dir: Path):
    """Generate a new ECDSA P-256 keypair for signing."""
    private_key = ec.generate_private_key(ec.SECP256R1(), default_backend())
    public_key = private_key.public_key()
    
    # Save private key
    private_pem = private_key.private_bytes(
        encoding=Encoding.PEM,
        format=PrivateFormat.PKCS8,
        encryption_algorithm=NoEncryption()
    )
    
    private_path = output_dir / "meantendo_private.pem"
    with open(private_path, 'wb') as f:
        f.write(private_pem)
    
    # Save public key
    public_pem = public_key.public_bytes(
        encoding=Encoding.PEM,
        format=PublicFormat.SubjectPublicKeyInfo
    )
    
    public_path = output_dir / "meantendo_public.pem"
    with open(public_path, 'wb') as f:
        f.write(public_pem)
    
    print(f"✅ Generated keypair:")
    print(f"   Private key: {private_path}")
    print(f"   Public key:  {public_path}")
    
    # Also output as C header for embedding
    public_key_c = output_dir / "meantendo_public_key.h"
    with open(public_key_c, 'w') as f:
        f.write("// Auto-generated public key for firmware verification\n")
        f.write("// DO NOT COMMIT THIS FILE IF USING FOR PRODUCTION\n\n")
        f.write("static const char OTA_PUBLIC_KEY[] PROGMEM = R\"(\n")
        f.write(public_pem.decode('ascii'))
        f.write(")\";\n")
    
    print(f"   C header:    {public_key_c}")
    return private_key, public_key


def calculate_sha256(data: bytes) -> bytes:
    """Calculate SHA256 hash of data."""
    return hashlib.sha256(data).digest()


def sign_firmware(firmware_path: Path, private_key_path: Path, output_path: Path) -> bool:
    """
    Sign firmware binary with ECDSA P-256.
    
    Signed firmware format:
    [Original Firmware][MNTD][Version (1 byte)][Hash (32 bytes)][Signature Length (2 bytes)][Signature]
    """
    
    print(f"📦 Signing firmware: {firmware_path}")
    
    # Read firmware
    with open(firmware_path, 'rb') as f:
        firmware = f.read()
    
    original_size = len(firmware)
    print(f"   Original size: {original_size:,} bytes")
    
    # Calculate SHA256 hash
    firmware_hash = calculate_sha256(firmware)
    print(f"   SHA256: {firmware_hash.hex()}")
    
    # Load private key
    with open(private_key_path, 'rb') as f:
        private_key = load_pem_private_key(f.read(), password=None, backend=default_backend())
    
    # Sign the hash
    signature = private_key.sign(firmware_hash, ec.ECDSA(hashes.SHA256()))
    
    # Create signed firmware
    signed_firmware = bytearray(firmware)
    signed_firmware.extend(SIGNATURE_MAGIC)
    signed_firmware.append(SIGNATURE_VERSION)
    signed_firmware.extend(firmware_hash)
    signed_firmware.extend(struct.pack('<H', len(signature)))
    signed_firmware.extend(signature)
    
    # Write signed firmware
    with open(output_path, 'wb') as f:
        f.write(signed_firmware)
    
    signed_size = len(signed_firmware)
    overhead = signed_size - original_size
    
    print(f"   Signed size:   {signed_size:,} bytes (+{overhead} bytes)")
    print(f"   Signature:     {signature[:16].hex()}... ({len(signature)} bytes)")
    print(f"✅ Saved to: {output_path}")
    
    return True


def verify_firmware(firmware_path: Path, public_key_path: Path) -> bool:
    """Verify a signed firmware binary."""
    
    print(f"🔍 Verifying firmware: {firmware_path}")
    
    with open(firmware_path, 'rb') as f:
        data = f.read()
    
    # Find signature magic
    magic_pos = data.rfind(SIGNATURE_MAGIC)
    if magic_pos == -1:
        print("❌ No signature found in firmware")
        return False
    
    # Extract components
    firmware = data[:magic_pos]
    signature_data = data[magic_pos:]
    
    # Parse signature header
    if len(signature_data) < 4 + 1 + 32 + 2:
        print("❌ Invalid signature format")
        return False
    
    offset = 4  # Skip magic
    version = signature_data[offset]
    offset += 1
    
    if version != SIGNATURE_VERSION:
        print(f"❌ Unsupported signature version: {version}")
        return False
    
    stored_hash = signature_data[offset:offset+32]
    offset += 32
    
    sig_len = struct.unpack('<H', signature_data[offset:offset+2])[0]
    offset += 2
    
    signature = signature_data[offset:offset+sig_len]
    
    # Verify hash
    calculated_hash = calculate_sha256(firmware)
    if calculated_hash != stored_hash:
        print("❌ Hash mismatch - firmware may be corrupted")
        return False
    
    print(f"   SHA256: {calculated_hash.hex()}")
    
    # Load public key
    with open(public_key_path, 'rb') as f:
        public_key = load_pem_public_key(f.read(), backend=default_backend())
    
    # Verify signature
    try:
        public_key.verify(signature, calculated_hash, ec.ECDSA(hashes.SHA256()))
        print("✅ Signature is VALID")
        print(f"   Firmware size: {len(firmware):,} bytes")
        return True
    except Exception as e:
        print(f"❌ Signature verification FAILED: {e}")
        return False


def create_ota_manifest(firmware_path: Path, version: str, output_path: Path):
    """Create OTA manifest JSON file."""
    import json
    
    with open(firmware_path, 'rb') as f:
        firmware = f.read()
    
    firmware_hash = calculate_sha256(firmware)
    
    manifest = {
        "version": version,
        "url": f"https://github.com/Debyte404/Meantendo/releases/download/v{version}/{firmware_path.name}",
        "sha256": firmware_hash.hex(),
        "size": len(firmware),
        "priority": "normal",
        "delta": False,
        "releaseDate": "",  # Will be filled by CI
        "releaseNotes": ""
    }
    
    with open(output_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"📋 Created manifest: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description='Meantendo Firmware Signing Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  Generate keypair:
    python sign_firmware.py --generate-keys --output-dir ./keys
    
  Sign firmware:
    python sign_firmware.py --input firmware.bin --key private.pem --output firmware_signed.bin
    
  Verify firmware:
    python sign_firmware.py --verify firmware_signed.bin --public-key public.pem
        """
    )
    
    parser.add_argument('--generate-keys', action='store_true',
                        help='Generate new ECDSA keypair')
    parser.add_argument('--output-dir', type=Path, default=Path('.'),
                        help='Output directory for generated keys')
    
    parser.add_argument('--input', '-i', type=Path,
                        help='Input firmware binary')
    parser.add_argument('--key', '-k', type=Path,
                        help='Private key for signing')
    parser.add_argument('--output', '-o', type=Path,
                        help='Output signed firmware')
    
    parser.add_argument('--verify', '-v', type=Path,
                        help='Verify signed firmware')
    parser.add_argument('--public-key', '-p', type=Path,
                        help='Public key for verification')
    
    parser.add_argument('--manifest', action='store_true',
                        help='Generate OTA manifest')
    parser.add_argument('--version', type=str, default='1.0.0',
                        help='Version for manifest')
    
    args = parser.parse_args()
    
    if args.generate_keys:
        from cryptography.hazmat.primitives.serialization import (
            PrivateFormat, NoEncryption
        )
        args.output_dir.mkdir(parents=True, exist_ok=True)
        generate_keypair(args.output_dir)
        return 0
    
    if args.verify:
        if not args.public_key:
            print("Error: --public-key required for verification")
            return 1
        success = verify_firmware(args.verify, args.public_key)
        return 0 if success else 1
    
    if args.input:
        if not args.key:
            print("Error: --key required for signing")
            return 1
        if not args.output:
            args.output = args.input.with_suffix('.signed.bin')
        
        success = sign_firmware(args.input, args.key, args.output)
        
        if success and args.manifest:
            manifest_path = args.output.with_suffix('.json')
            create_ota_manifest(args.output, args.version, manifest_path)
        
        return 0 if success else 1
    
    parser.print_help()
    return 0


if __name__ == '__main__':
    sys.exit(main())
