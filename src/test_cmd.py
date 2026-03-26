import hmac
import hashlib
import time
import struct

# Paste your values here
DEVICE_ID  = "123e4567-e89b-12d3-a456-426614174000"
SECRET_HEX = "91e978b548d13b9316eac52ba1030d56837c30e9b58eb45e074c6c1f4319f329"
COMMAND    = 0x01  # 0x01=UNLOCK, 0x02=LOCK, 0x03=STATUS

secret    = bytes.fromhex(SECRET_HEX)
timestamp = int(time.time())

# Build message: device_id + timestamp (4 bytes big-endian) + command
message = DEVICE_ID.encode() + struct.pack(">I", timestamp) + bytes([COMMAND])

# Compute HMAC-SHA256
mac = hmac.new(secret, message, hashlib.sha256).hexdigest()

print(f"Timestamp: {timestamp}")
print(f"HMAC:      {mac}")
print()
print("curl command:")
print(f'curl -X POST http://192.168.4.1/cmd -H "Content-Type: application/json" -d "{{\\"device_id\\":\\"{DEVICE_ID}\\",\\"timestamp\\":{timestamp},\\"command\\":{COMMAND},\\"hmac\\":\\"{mac}\\"}}"')