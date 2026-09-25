#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later
// BLE GATT map — adapted taxonomy from OpenWink constants.h (GPLv3,
// seasaltsaige/openwink). UUID values kept for interop; OTA marked reserved
// (local-only policy). Changes: auth/challenge semantics + lockout documented
// in firmware/BLE_PROTOCOL.md. No OpenWink code copied.
#define FW_VERSION "0.2.1"
#define WINK_SERVICE_UUID "a144c6b0-5e1a-4460-bb92-3674b2f51520"
#define OTA_SERVICE_UUID "e24c13d7-d7c7-4301-903a-7750b09fc935"  // RESERVED: local USB/BLE-file update only, no cloud
#define MODULE_SETTINGS_SERVICE_UUID "cb5f7a1f-59f2-418e-b9d1-d6fc5c85a749"
#define HEADLIGHT_CHAR_UUID "034a383c-d3e4-4501-b7a5-1c950db4f3c7"
#define BUSY_CHAR_UUID "8d2b7b9f-c6a3-4f56-9f4f-2dc7d7873c18"
#define LEFT_STATUS_UUID "c4907f4a-fb0c-440c-bbf1-4836b0636478"
#define RIGHT_STATUS_UUID "784dd553-d837-4027-9143-280cb035163a"
#define SLEEPY_EYE_UUID "a8237fed-e0a4-4ecd-9881-9b5dbb3f5902"
#define SYNC_UUID "eceed349-998f-46a2-9835-4f2db7552381"
#define CUSTOM_COMMAND_UUID "1313c33f-e793-422c-8c04-c82be9fe8a02"
#define PASSKEY_UUID "f61146f2-791d-4ef7-95aa-b565097f69c2"
#define UNPAIR_UUID "c67c4fd1-21ce-4a75-bd16-629f990e575d"
#define RESET_UUID "a55946b8-1978-4522-8a29-27d17e21b092"
#define HEADLIGHT_BYPASS_UUID "ada2537e-0399-4d2a-9eab-0c7cb60d3500"
#define SWAP_ORIENTATION_UUID "3ddd922d-14ca-4785-9cd0-39a530e8b14d"
#define AUTH_TIME_MS 5000
#define RATE_LIMIT_PER_S 10
#define LOCKOUT_S 5
#define WDT_TIMEOUT_MS 2000
#define MOTION_TIMEOUT_MS 5000
#define STAGGER_MS 150
