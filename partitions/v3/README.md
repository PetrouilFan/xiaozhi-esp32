# v3 Partition Layout - LittleFS Unified Storage

## Overview
v3 replaces the old OTA+SPIFFS partitions with a single LittleFS partition that holds everything.

## Partition Layout (16MB Flash)
| Name    | Type | SubType | Offset | Size   | Purpose                    |
|---------|------|---------|--------|--------|----------------------------|
| factory | 0x00 | 0x00    | 0x1000 | 8K     | First boot (fallback)      |
| nvs     | 0x01 | 0x02    | 0x9000 | 16K    | WiFi creds, boot flags     |
| storage | 0x01 | 0x04    | 0xD000 | Remain | All app + models + assets |

## Directory Structure (/storage)
```
/storage/
├── _meta/
│   ├── version.json    # {"app": "3.0.0", "assets": "20260101"}
│   ├── checksum.json  # SHA256 hashes
│   └── boot.conf      # Current boot selection
├── app/
│   ├── main.elf       # Current application
│   ├── rollback.elf  # Previous working version
│   └── update.elf     # Downloaded update
├── models/
│   ├── wakeword/
│   └── speaker/
├── fonts/
├── assets/
│   ├── sounds/
│   ├── images/
│   └── emojis/
└── data/
    └── ...
```

## OTA Update Flow
1. Download update.elf → /storage/app/update.elf
2. Verify SHA256 checksum
3. Copy main.elf → rollback.elf
4. Copy update.elf → main.elf
5. Update boot.conf → "main"
6. Reboot

## Recovery Flow (Boot Early)
1. Read boot.conf from storage
2. If corrupted → boot rollback.elf
3. If rollback invalid → factory (requires PC reflash)