/*
 * KCB 運行管理システム
 * ブートスクリーン + メイン画面（ダイヤルメニュー）
 * 
 * ブートスクリーン → 3秒後 → メイン画面へ遷移
 * メイン画面: タッチ左右でメニュー切替、ダイヤル回転対応予定
 */

#include <M5Dial.h>
#include <TinyGPSPlus.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "secrets.h"  // gitignore対象

#define AWS_REGION            "us-east-1"
#define S3_HOST               "drive-log-cloud.s3.us-east-1.amazonaws.com"
#define AP_PASSWORD "16001600"   // 8文字以上必須（リリース時に変更）
// ============================================================
// ビットマップアイコンデータ（PNG, PROGMEM）
// ============================================================

// --- car icon (active, 40x40) ---
static const uint8_t icon_car_active[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x28, 0x08, 0x06, 0x00, 0x00, 0x00, 0x8c, 0xfe, 0xb8,
  0x6d, 0x00, 0x00, 0x00, 0xcc, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x96, 0xd1, 0x0d, 0xc3,
  0x20, 0x10, 0x43, 0xd3, 0xaa, 0x93, 0x74, 0x9c, 0x4c, 0xc5, 0x54, 0x1d, 0x27, 0xab, 0x24, 0x5f,
  0x48, 0x11, 0x6a, 0xee, 0x6c, 0x13, 0x41, 0x3e, 0xfc, 0xfe, 0x82, 0x62, 0xee, 0x30, 0x46, 0xb0,
  0x2c, 0xc6, 0x18, 0x63, 0x4c, 0x0f, 0x2f, 0x55, 0xb8, 0x6e, 0x65, 0x67, 0x35, 0xbf, 0x6f, 0xa1,
  0xeb, 0xbd, 0x59, 0xc1, 0x68, 0xa4, 0x06, 0xab, 0x7b, 0xa8, 0x23, 0xf5, 0x3f, 0xc5, 0xf5, 0x0f,
  0x2b, 0x88, 0x1a, 0xa8, 0x4d, 0xb4, 0xdf, 0x77, 0xd4, 0x80, 0x59, 0xb7, 0xb2, 0xd7, 0xa2, 0xff,
  0x8a, 0x47, 0x63, 0x67, 0x2d, 0xca, 0xe3, 0x33, 0x48, 0x91, 0xb9, 0x87, 0x8c, 0xb3, 0x2e, 0xa6,
  0x21, 0x1f, 0x91, 0xa1, 0xe8, 0xb0, 0x85, 0x5b, 0x3c, 0x2a, 0xe0, 0x52, 0x1d, 0x25, 0xd0, 0x2a,
  0x51, 0xad, 0xe9, 0x87, 0x24, 0x33, 0x61, 0x7a, 0x83, 0x19, 0xd3, 0x1b, 0xcc, 0x6e, 0xa3, 0xe9,
  0x0d, 0x66, 0x50, 0x57, 0xdd, 0x39, 0x2f, 0xca, 0xcb, 0x44, 0xd1, 0xc3, 0x0e, 0xb6, 0x61, 0x66,
  0x4f, 0xb8, 0xaa, 0x0f, 0x57, 0x31, 0xf2, 0xa2, 0xbf, 0x72, 0x34, 0x74, 0x50, 0xd9, 0x46, 0x85,
  0xa8, 0x0e, 0xd4, 0x40, 0xe4, 0x24, 0xb2, 0x88, 0x1e, 0x3d, 0x94, 0xc1, 0xab, 0x49, 0xd8, 0x07,
  0xab, 0xa2, 0x87, 0x0f, 0x49, 0x3b, 0x19, 0xbb, 0xfd, 0xbd, 0x7a, 0x63, 0x8c, 0x31, 0x0f, 0xe5,
  0x00, 0x01, 0xb0, 0x7d, 0x47, 0x0f, 0xe4, 0xaa, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e,
  0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_car_active_len = 261;

// --- car icon (inactive, 28x28) ---
static const uint8_t icon_car_inactive[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0x0d, 0xdf,
  0x94, 0x00, 0x00, 0x00, 0x8b, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x18, 0x05, 0xa3,
  0x60, 0x14, 0xa0, 0x01, 0x46, 0x62, 0x15, 0xaa, 0xe6, 0x3b, 0x6c, 0xc1, 0x27, 0x7f, 0x7b, 0xe2,
  0x01, 0x1f, 0x62, 0xcc, 0x61, 0x22, 0xd6, 0x42, 0x42, 0x96, 0x11, 0x72, 0x10, 0x49, 0x40, 0x35,
  0xdf, 0x61, 0x0b, 0xba, 0x81, 0xc8, 0x7c, 0x6c, 0xf2, 0xb8, 0x00, 0x55, 0x7c, 0xc8, 0xc0, 0x40,
  0xbc, 0x2f, 0xb1, 0xc6, 0x21, 0xb5, 0x82, 0x87, 0xa8, 0x78, 0xa5, 0x66, 0x5c, 0x60, 0x33, 0x8b,
  0x6a, 0x41, 0x4a, 0x8c, 0x65, 0x34, 0xb5, 0x10, 0x17, 0xa0, 0xbb, 0x85, 0x2c, 0xf8, 0x24, 0x91,
  0x83, 0x85, 0x50, 0x02, 0x40, 0x57, 0x8b, 0x2b, 0xd5, 0xe2, 0xf4, 0x21, 0xbe, 0x7c, 0x47, 0x89,
  0x5a, 0xba, 0x67, 0x0b, 0x9c, 0x65, 0x29, 0x36, 0x4b, 0x71, 0x05, 0x2b, 0x29, 0x6a, 0x71, 0x06,
  0x29, 0xba, 0x06, 0x7c, 0x71, 0x48, 0x8a, 0xda, 0x51, 0x30, 0x0a, 0x46, 0x01, 0x06, 0x00, 0x00,
  0x5f, 0xbc, 0x44, 0x57, 0xfd, 0xa8, 0xe2, 0xbd, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
  0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_car_inactive_len = 196;

// --- gps icon (active, 40x40) ---
static const uint8_t icon_gps_active[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x28, 0x08, 0x06, 0x00, 0x00, 0x00, 0x8c, 0xfe, 0xb8,
  0x6d, 0x00, 0x00, 0x00, 0xc6, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x97, 0x5b, 0x12, 0x40,
  0x30, 0x0c, 0x45, 0x31, 0x96, 0x63, 0x19, 0x96, 0x6d, 0x19, 0xdd, 0x4f, 0x7d, 0x99, 0x21, 0xd2,
  0x56, 0x92, 0xdb, 0x99, 0x20, 0xe7, 0xcf, 0xab, 0x8e, 0xdb, 0x86, 0x18, 0x86, 0x20, 0x08, 0xaa,
  0x8c, 0xb0, 0x91, 0xb6, 0x94, 0x6f, 0xfb, 0xd6, 0xc5, 0x3c, 0xbe, 0x5d, 0x90, 0x13, 0xa3, 0x18,
  0x44, 0x6d, 0x82, 0x67, 0x39, 0x4e, 0xa2, 0x75, 0xbc, 0x2b, 0x5b, 0xca, 0x8f, 0xd2, 0x93, 0x9e,
  0x0b, 0x41, 0x73, 0x43, 0xa5, 0xe4, 0x2c, 0xbd, 0xa0, 0x29, 0x71, 0x00, 0x9a, 0xd2, 0x49, 0x2d,
  0x41, 0x05, 0x68, 0x3a, 0x74, 0xfb, 0x38, 0x5f, 0x98, 0xa2, 0x5c, 0x90, 0xa3, 0x74, 0x53, 0xc0,
  0xba, 0xc3, 0x08, 0x76, 0xe4, 0x27, 0x82, 0xa5, 0x82, 0x00, 0x14, 0x8a, 0x5c, 0xb0, 0xb4, 0xd8,
  0xa9, 0x4c, 0xa9, 0x88, 0x84, 0xd2, 0xd8, 0xd7, 0x4c, 0x87, 0xaf, 0x85, 0x7e, 0x40, 0x4d, 0x85,
  0x2a, 0x1e, 0xe0, 0xc3, 0x45, 0x22, 0x4d, 0x43, 0x39, 0xfd, 0xee, 0xab, 0xd8, 0x4e, 0xad, 0x09,
  0x00, 0x74, 0x31, 0xee, 0xd7, 0x20, 0x06, 0x2e, 0x29, 0x50, 0x0f, 0xe8, 0x3e, 0x41, 0xf7, 0x82,
  0x38, 0xce, 0x53, 0x0a, 0x6c, 0xf1, 0xdd, 0x27, 0x88, 0x7d, 0x4f, 0xb9, 0x68, 0xf9, 0x5f, 0x0f,
  0xf8, 0xf7, 0xf2, 0x87, 0x09, 0x06, 0x41, 0x10, 0x5c, 0xd8, 0x01, 0x3a, 0x5c, 0x65, 0x1c, 0xc1,
  0xcd, 0xc9, 0xff, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_gps_active_len = 255;

// --- gps icon (inactive, 28x28) ---
static const uint8_t icon_gps_inactive[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0x0d, 0xdf,
  0x94, 0x00, 0x00, 0x00, 0x80, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x18, 0x05, 0x43,
  0x1d, 0x30, 0x92, 0xac, 0x23, 0x2e, 0x6b, 0x0b, 0x86, 0xd8, 0xa2, 0x69, 0x3e, 0xd4, 0x70, 0x0c,
  0x71, 0x96, 0xe1, 0x13, 0xa7, 0x89, 0x65, 0x34, 0xb1, 0x14, 0xdd, 0xb0, 0xb8, 0xac, 0x2d, 0x70,
  0x4c, 0xa2, 0xa5, 0x4c, 0x54, 0xb1, 0x9c, 0x04, 0x40, 0xba, 0x85, 0x14, 0x82, 0x21, 0x60, 0x21,
  0x7a, 0x16, 0x20, 0x31, 0x4b, 0x10, 0x97, 0x0f, 0x89, 0x8d, 0x27, 0xaa, 0xe6, 0x47, 0x6c, 0xa9,
  0x92, 0x18, 0x39, 0x34, 0xc0, 0x42, 0x92, 0xa5, 0x8b, 0xa6, 0xf9, 0xe0, 0x34, 0x98, 0x26, 0xa5,
  0x0d, 0x5d, 0x4b, 0x1a, 0x5c, 0x06, 0xd3, 0xcc, 0x32, 0x5c, 0x16, 0x0c, 0xf6, 0x8c, 0x4f, 0x1e,
  0x80, 0xf9, 0x8a, 0x8c, 0xe0, 0x1c, 0x22, 0x3e, 0x64, 0x60, 0x20, 0x3b, 0xb1, 0x0c, 0x21, 0x1f,
  0x8e, 0x82, 0x11, 0x0b, 0x00, 0xc0, 0xf1, 0x31, 0x41, 0x3b, 0x8c, 0xa2, 0x86, 0x00, 0x00, 0x00,
  0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_gps_inactive_len = 185;

// --- obd icon (active, 40x40) ---
static const uint8_t icon_obd_active[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x28, 0x08, 0x06, 0x00, 0x00, 0x00, 0x8c, 0xfe, 0xb8,
  0x6d, 0x00, 0x00, 0x00, 0x82, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0xd5, 0x51, 0x0e, 0x40,
  0x40, 0x0c, 0x84, 0xe1, 0xe5, 0xb4, 0x8e, 0xb4, 0xb7, 0xad, 0x27, 0xc9, 0x46, 0x82, 0x69, 0x4c,
  0x25, 0xf8, 0xbf, 0x27, 0x0f, 0x52, 0xdb, 0xd1, 0xd2, 0x1a, 0x00, 0x00, 0xc0, 0x9b, 0x4d, 0x57,
  0x37, 0x44, 0x6f, 0x51, 0x7e, 0x88, 0xe5, 0xf8, 0x1c, 0x73, 0xf5, 0xc3, 0x1f, 0x51, 0x95, 0xa2,
  0x52, 0x57, 0x4e, 0x30, 0x7a, 0x8b, 0xb1, 0xe0, 0xdd, 0x6b, 0xb5, 0xe9, 0xcb, 0x19, 0xdc, 0x17,
  0x77, 0x3b, 0x9b, 0x3f, 0xd9, 0xd6, 0xb1, 0x3b, 0x41, 0x6b, 0xe3, 0xee, 0x14, 0xd5, 0x7a, 0xa9,
  0x2d, 0x76, 0x1d, 0x32, 0x53, 0x27, 0xfd, 0x99, 0x71, 0xbd, 0x66, 0x95, 0x3c, 0xa0, 0x15, 0x8b,
  0x62, 0x59, 0x90, 0x91, 0x6b, 0x51, 0x32, 0xcd, 0xa6, 0x3a, 0x70, 0xa6, 0xa8, 0xa6, 0xf7, 0x8d,
  0x5f, 0x1d, 0x00, 0x00, 0xf8, 0xaf, 0x15, 0x96, 0x5d, 0x73, 0x7f, 0x7a, 0x1d, 0xe8, 0x12, 0x00,
  0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_obd_active_len = 187;

// --- obd icon (inactive, 28x28) ---
static const uint8_t icon_obd_inactive[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0x0d, 0xdf,
  0x94, 0x00, 0x00, 0x00, 0x6c, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x91, 0x51, 0x0e, 0xc0,
  0x20, 0x08, 0x43, 0xd9, 0xce, 0xe7, 0x9d, 0x96, 0xdd, 0xc9, 0x03, 0xee, 0x97, 0x18, 0x28, 0x8d,
  0x33, 0xfe, 0xd8, 0xf7, 0x43, 0xa2, 0xb6, 0x58, 0x30, 0x13, 0x42, 0x1c, 0xc7, 0x15, 0x1d, 0x3e,
  0xcd, 0xfa, 0x0a, 0xf3, 0xb7, 0x5b, 0xa3, 0x1e, 0xae, 0x68, 0x98, 0x79, 0xdc, 0x8c, 0x68, 0xac,
  0xe8, 0xae, 0xfa, 0x6c, 0x38, 0x52, 0x46, 0x58, 0x91, 0x8d, 0x13, 0x26, 0xf4, 0x42, 0x5f, 0xa3,
  0x33, 0xd4, 0x84, 0xe2, 0x4f, 0x42, 0xa4, 0x2d, 0x13, 0xce, 0xec, 0x11, 0x91, 0xee, 0x90, 0x35,
  0x88, 0x40, 0xa3, 0x2d, 0x13, 0x7a, 0x03, 0x66, 0x8f, 0x15, 0xdb, 0x13, 0x0a, 0x21, 0x0e, 0xe4,
  0x03, 0x21, 0xf6, 0x39, 0x09, 0x36, 0xb1, 0x48, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e,
  0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_obd_inactive_len = 165;

// --- driver icon (active, 40x40) ---
static const uint8_t icon_driver_active[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x28, 0x08, 0x06, 0x00, 0x00, 0x00, 0x8c, 0xfe, 0xb8,
  0x6d, 0x00, 0x00, 0x00, 0xce, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x97, 0x4b, 0x0e, 0x85,
  0x20, 0x0c, 0x45, 0xc1, 0xb8, 0x3d, 0x77, 0xe4, 0xd0, 0x1d, 0xb9, 0x40, 0x1c, 0x91, 0x10, 0x53,
  0x4b, 0x4b, 0x2f, 0xc6, 0xbc, 0x77, 0xcf, 0x50, 0x0c, 0x1c, 0xfb, 0x01, 0x4c, 0x89, 0x10, 0x42,
  0xfe, 0x9b, 0xec, 0x79, 0x79, 0xdf, 0x4a, 0x41, 0x2d, 0x7c, 0x9c, 0xd9, 0xb4, 0xf6, 0x62, 0x9d,
  0x10, 0x29, 0xe7, 0x99, 0x6f, 0xf5, 0x4e, 0x2c, 0x7d, 0x79, 0x5d, 0xac, 0x1d, 0x93, 0x9e, 0x79,
  0xe5, 0x52, 0x72, 0x44, 0xb0, 0xc7, 0x5d, 0xc4, 0x9a, 0xc2, 0x1e, 0x61, 0xc1, 0x7d, 0x2b, 0xe5,
  0x49, 0xe6, 0x38, 0x73, 0x8e, 0x96, 0x06, 0x2c, 0x82, 0xb3, 0xf8, 0x7d, 0x41, 0x2d, 0x8d, 0x5a,
  0xfa, 0xad, 0xc0, 0x22, 0x78, 0x97, 0x44, 0x6d, 0x4b, 0xee, 0x6d, 0x46, 0xa2, 0x46, 0xa9, 0x95,
  0x42, 0x75, 0x31, 0x44, 0xb0, 0x82, 0x92, 0x6a, 0x81, 0x09, 0x4a, 0x29, 0x45, 0x08, 0x87, 0x05,
  0xb5, 0x5a, 0xd3, 0x4e, 0x13, 0x2b, 0x21, 0xc1, 0x5e, 0xcd, 0xd5, 0xf1, 0x48, 0x37, 0x0f, 0x77,
  0x71, 0x1b, 0x1d, 0xed, 0x24, 0x91, 0x1a, 0x68, 0xba, 0xa0, 0x37, 0x75, 0x11, 0xc9, 0xcf, 0x9f,
  0x24, 0x43, 0x35, 0x38, 0x52, 0x4f, 0xaf, 0xd7, 0xe0, 0x5b, 0x50, 0x30, 0x0a, 0x05, 0xa3, 0xb8,
  0xbb, 0x18, 0xfd, 0x77, 0xd7, 0xc3, 0x1c, 0x41, 0xf4, 0x4d, 0x65, 0xc6, 0xcd, 0x87, 0x10, 0x42,
  0x06, 0xb8, 0x00, 0x1d, 0xfa, 0x63, 0xed, 0xa5, 0x91, 0xb2, 0x36, 0x00, 0x00, 0x00, 0x00, 0x49,
  0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_driver_active_len = 263;

// --- driver icon (inactive, 28x28) ---
static const uint8_t icon_driver_inactive[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0x0d, 0xdf,
  0x94, 0x00, 0x00, 0x00, 0x7e, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x18, 0x05, 0xa3,
  0x80, 0x44, 0xc0, 0x48, 0x48, 0x81, 0x9d, 0x5a, 0xfd, 0x16, 0x52, 0x0d, 0x3d, 0x74, 0xab, 0xd1,
  0x87, 0x2c, 0xd7, 0x90, 0x63, 0x19, 0x21, 0x7d, 0x2c, 0x94, 0x1a, 0x48, 0xb6, 0x6f, 0x08, 0x19,
  0x8c, 0x4b, 0x8c, 0x18, 0x35, 0xc8, 0x80, 0x89, 0x3a, 0x4e, 0x23, 0x1e, 0xd0, 0xdd, 0x42, 0x92,
  0xe3, 0xf0, 0xd0, 0xad, 0x46, 0x1f, 0x4a, 0xe2, 0x90, 0xac, 0x44, 0x43, 0x49, 0x42, 0x21, 0xd9,
  0x42, 0xf4, 0x04, 0x41, 0xd3, 0x54, 0x4a, 0x6c, 0x8a, 0x24, 0x37, 0xff, 0xa2, 0x68, 0xc4, 0x67,
  0x08, 0x31, 0x0e, 0x23, 0xc9, 0x42, 0x62, 0x0c, 0x20, 0xd6, 0x71, 0x74, 0xcf, 0x16, 0x78, 0x01,
  0x2d, 0xca, 0xd2, 0xe1, 0x5f, 0xd2, 0xd0, 0xdd, 0xc2, 0xc1, 0x55, 0x01, 0x8f, 0x82, 0x51, 0x80,
  0x0d, 0x00, 0x00, 0x29, 0x22, 0x38, 0xbd, 0xca, 0xbc, 0x63, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x49,
  0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const size_t icon_driver_inactive_len = 183;

// --- gear icon (active, 40x40) ---
static const uint8_t icon_gear_active[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x28, 0x08, 0x06, 0x00, 0x00, 0x00, 0x8c, 0xfe, 0xb8,
  0x6d, 0x00, 0x00, 0x01, 0x1a, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x97, 0x41, 0x0e, 0x84,
  0x30, 0x08, 0x45, 0x71, 0x32, 0x47, 0xf5, 0x00, 0x2e, 0x3d, 0x82, 0x4b, 0x0f, 0xe0, 0x5d, 0x9d,
  0x55, 0x93, 0x0e, 0x81, 0xc2, 0x87, 0xea, 0x8a, 0x97, 0x4c, 0x26, 0xa9, 0x96, 0x7e, 0x29, 0xfc,
  0x2a, 0x51, 0x51, 0x14, 0x45, 0x91, 0x62, 0x3f, 0xaf, 0x5b, 0x1b, 0x6f, 0xbf, 0x4c, 0xfc, 0x4f,
  0x66, 0x72, 0x5b, 0x9c, 0x8b, 0xc8, 0x8a, 0xea, 0xf9, 0x46, 0x26, 0xcd, 0x14, 0x60, 0x91, 0xca,
  0xe0, 0xb1, 0xad, 0xcb, 0xb1, 0xad, 0x0b, 0x91, 0x9c, 0x4d, 0x7e, 0xed, 0x35, 0x81, 0x6d, 0x61,
  0x8e, 0x24, 0x2e, 0xcb, 0x94, 0x1a, 0x6c, 0x62, 0xf8, 0xbf, 0x67, 0xae, 0x45, 0xf8, 0x29, 0xb9,
  0xb8, 0xa8, 0x10, 0x6b, 0x7e, 0x28, 0x83, 0x48, 0x4d, 0xf5, 0x02, 0x5a, 0xcd, 0x22, 0xdb, 0x9f,
  0xda, 0x0a, 0xbe, 0x10, 0x52, 0x83, 0xde, 0x7b, 0xdd, 0x02, 0x47, 0x41, 0xa2, 0x0f, 0x61, 0xc5,
  0x25, 0x02, 0x7c, 0x70, 0x3f, 0xaf, 0xdb, 0x0a, 0xd6, 0x5f, 0x97, 0x44, 0x3f, 0xd1, 0xe5, 0x7f,
  0xc1, 0xa5, 0xa3, 0x6b, 0x74, 0x9c, 0x69, 0xf7, 0x23, 0xeb, 0xba, 0x9b, 0xc4, 0xca, 0x8e, 0x07,
  0xed, 0x68, 0x1c, 0x01, 0x75, 0x31, 0xe2, 0x73, 0xa3, 0xf9, 0x44, 0x0f, 0xf9, 0xa0, 0x56, 0x43,
  0x52, 0x23, 0x59, 0xcd, 0xe5, 0xad, 0x47, 0x57, 0x93, 0x78, 0x3b, 0x0f, 0xf5, 0x47, 0xcf, 0xfd,
  0x90, 0x0f, 0x66, 0xbc, 0x0d, 0xb1, 0x22, 0x48, 0x60, 0x1f, 0x1c, 0xd9, 0x42, 0x4d, 0x1c, 0x5a,
  0xbf, 0x90, 0x40, 0x0d, 0x6f, 0x66, 0xa7, 0x7b, 0x9f, 0xb4, 0x08, 0x1f, 0xd3, 0x7c, 0xae, 0x1f,
  0xcf, 0xbc, 0xfa, 0x87, 0x7c, 0x50, 0x83, 0x8b, 0x9b, 0x91, 0xb9, 0xd4, 0xfb, 0x60, 0x8f, 0xa7,
  0xce, 0x22, 0x59, 0x4c, 0xd7, 0xc4, 0xc8, 0x82, 0x24, 0x41, 0x68, 0x36, 0x43, 0x1f, 0x4d, 0x11,
  0xa2, 0xdb, 0x3c, 0x6d, 0x8b, 0x25, 0x11, 0xd9, 0xa3, 0xb1, 0x28, 0x8a, 0xe2, 0x05, 0x7e, 0x92,
  0xdb, 0xf5, 0x00, 0xa9, 0xd2, 0xc8, 0x82, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
  0x42, 0x60, 0x82,
};
static const size_t icon_gear_active_len = 339;

// --- gear icon (inactive, 28x28) ---
static const uint8_t icon_gear_inactive[] PROGMEM = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0x0d, 0xdf,
  0x94, 0x00, 0x00, 0x00, 0xaa, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xed, 0x95, 0xcd, 0x09, 0x80,
  0x30, 0x0c, 0x85, 0xa3, 0x38, 0x96, 0x03, 0x78, 0x10, 0x87, 0x15, 0x0f, 0x0e, 0xe0, 0x60, 0x9e,
  0x0a, 0x12, 0x5e, 0xfe, 0x9a, 0x0a, 0x82, 0x7d, 0xa7, 0xd8, 0xd4, 0x7c, 0x69, 0x9b, 0xa6, 0x44,
  0x5d, 0x5d, 0x5f, 0xd7, 0xe0, 0x9d, 0x38, 0x2f, 0xdb, 0x41, 0x44, 0x74, 0x9d, 0xfb, 0xca, 0xc7,
  0xf8, 0x78, 0x5a, 0xcf, 0xc0, 0xc8, 0x7e, 0x8e, 0x59, 0x1a, 0x33, 0x49, 0x34, 0x5d, 0x15, 0x02,
  0x20, 0x1b, 0xf9, 0x22, 0x2b, 0x76, 0x01, 0x91, 0x8f, 0x83, 0xa4, 0xf9, 0x53, 0x0d, 0x80, 0x2b,
  0xb2, 0xb5, 0x22, 0x50, 0x0b, 0xa4, 0x55, 0x27, 0xaa, 0x66, 0x97, 0xa4, 0xb3, 0xb0, 0xce, 0x2f,
  0x04, 0x91, 0xc0, 0x99, 0xef, 0x34, 0xd4, 0x03, 0xa8, 0xae, 0x54, 0x6d, 0x5b, 0xad, 0xa0, 0x92,
  0x4f, 0x6c, 0x6d, 0xd1, 0x8b, 0x8d, 0x00, 0xe8, 0x7f, 0xb5, 0x4a, 0x51, 0x30, 0x2d, 0x89, 0x74,
  0xe7, 0xb1, 0x2e, 0x73, 0xf3, 0x0a, 0x45, 0x09, 0x70, 0x3b, 0x0a, 0xad, 0x6a, 0xde, 0x99, 0xc6,
  0xed, 0x7e, 0x0f, 0x0b, 0xa8, 0xd8, 0xe8, 0x5d, 0x7c, 0xf5, 0xf5, 0xe8, 0xfa, 0xaf, 0x6e, 0xb6,
  0xb0, 0x76, 0x8a, 0x74, 0x3a, 0x28, 0x9a, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
  0x42, 0x60, 0x82,
};
static const size_t icon_gear_inactive_len = 227;

// ============================================================
// アイコンデータ参照テーブル
// ============================================================
struct IconData {
  const uint8_t* active;
  size_t activeLen;
  const uint8_t* inactive;
  size_t inactiveLen;
};

static const IconData icons[5] = {
  { icon_car_active,    icon_car_active_len,    icon_car_inactive,    icon_car_inactive_len },
  { icon_gps_active,    icon_gps_active_len,    icon_gps_inactive,    icon_gps_inactive_len },
  { icon_obd_active,    icon_obd_active_len,    icon_obd_inactive,    icon_obd_inactive_len },
  { icon_driver_active, icon_driver_active_len, icon_driver_inactive, icon_driver_inactive_len },
  { icon_gear_active,   icon_gear_active_len,   icon_gear_inactive,   icon_gear_inactive_len },
};

// ============================================================
// 色定義
// ============================================================
#define COL_BG        0x0208    // 暗いグリーンブラック
#define COL_GREEN     0x4F66    // メインのグリーン (#4ade80相当)
#define COL_GREEN_DIM 0x1B22    // 暗めのグリーン
#define COL_DARK      0x0A41    // 背景グラデ用
#define COL_GRAY_DIM  0x3186    // 薄いグレー（未初期化）
#define COL_WHITE_DIM 0x6B4D    // 薄い白

// メイン画面用追加色
#define COL_CYAN      0x07FF
#define COL_ORANGE    0xFD20
#define COL_PURPLE    0x781F
#define COL_GRAY      0x7BEF
#define COL_DIM_LINE  0x2104    // 非常に薄い白

// ============================================================
// 画面定数
// ============================================================
#define CX 120
#define CY 120
#define SCREEN_R 120

// メーター中心（少し上寄せ）
#define GAUGE_CX 120
#define GAUGE_CY 100
#define GAUGE_R  65

// ============================================================
// モジュール定義（ブート画面用）
// ============================================================
#define MOD_GPS  0
#define MOD_OBD  1
#define MOD_NFC  2
#define MOD_WIFI 3
#define MOD_COUNT 4

const char* modNames[MOD_COUNT] = { "GPS", "OBD", "NFC", "WiFi" };
bool modReady[MOD_COUNT] = { false, false, false, false };

// ============================================================
// NFC関連
// ============================================================

// WiFi接続（ノンブロッキング）
bool wifiConnecting = false;
unsigned long wifiConnectStart = 0;
const unsigned long WIFI_TIMEOUT = 10000;  // 10秒タイムアウト
// WiFi設定（SPIFFSから読み込み、なければsecrets.hにフォールバック）
String wifiSSID = WIFI_SSID;
String wifiPass = WIFI_PASS;
int configSelectedIndex = 0;
const int CONFIG_ITEMS = 4;

// APモード
//WebServer apServer(80);
//WiFiServer apTcpServer(80);
WiFiServer apTcpServer(8080);
bool apModeActive = false;
const char* AP_SSID = "DLC-Setup";
DNSServer dnsServer;

// GPS
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// 国土地理院 逆ジオコーディング
String currentAddress = "";
unsigned long lastAddressUpdate = 0;
const unsigned long ADDRESS_INTERVAL = 30000;  // 30秒ごとに更新

#define GPS_RX 1  // PORT.A: GPS TX → M5Dial G1
#define GPS_TX 2  // PORT.A: GPS RX → M5Dial G2

// 変更後
//double baseLat = 43.804443;  // デフォルト値（SPIFFS未保存時のフォールバック）
double baseLat = 43.8023501;
//double baseLng = 143.892911;
double baseLng = 143.816459;
#define RETURN_RADIUS 150.0  

// 最遠地点記録
double farthestLat = 0.0;
double farthestLng = 0.0;
double farthestDist = 0.0;

// --- 運行中ステータス永続化 ---
bool tripActive = false;
String currentDriverName = "";
unsigned long lastTripStateSave = 0;
const unsigned long TRIP_STATE_SAVE_INTERVAL = 30000UL; // 30秒
unsigned long tripCooldownUntil = 0;
unsigned long tripStartTime = 0;

float tripDistAccum = 0.0;   // 積算走行距離(km)
double prevLat = 0.0;
double prevLng = 0.0;
bool prevPosValid = false;
uint32_t lastOdometer = 0;  // 前回終了時のオドメーター値

uint32_t manualDistKm = 0;  // 手動入力中の距離値

// 画面状態
enum Screen { SCREEN_MENU, SCREEN_GPS, SCREEN_OBD2, SCREEN_DRIVER, SCREEN_CONFIG, SCREEN_MANUAL_DIST };
Screen currentScreen = SCREEN_MENU;

unsigned long apModeStartTime = 0;

// ドライバー情報
String currentDriverUID = "";
String displayUID = "";
bool nfcReady = false;
unsigned long lastNfcRead = 0;
long lastEncoderPos = 0;

#define NFC_READ_INTERVAL 500  // 読み取り間隔(ms)

const int modX[MOD_COUNT] = { 52, 96, 140, 184 };
const int modY[MOD_COUNT] = { 200, 200, 200, 200 };
const int dotR = 4;

// ============================================================
// メニュー定義（メイン画面用）
// ============================================================
#define MENU_COUNT 5

struct MenuItem {
  const char* label;
  uint16_t color;
};

static const MenuItem menuItems[MENU_COUNT] = {
  { "ステータス",   COL_GREEN  },
  { "GPS情報",     COL_CYAN   },
  { "OBD2",       COL_ORANGE },
  { "ドライバー",   COL_PURPLE },
  { "設定",        COL_GRAY   },
};

// アイコン配置角度: 10, 11, 12, 1, 2 時の位置
static const float iconAngles[MENU_COUNT] = { -150.0, -120.0, -90.0, -60.0, -30.0 };
static const int ICON_ORBIT_R = 82;

uint32_t tripStartOdometer = 0;  // 運行開始時オドメーター(km)

// ============================================================
// アプリケーション状態
// ============================================================
enum AppState {
  STATE_BOOT,
  STATE_BOOT_GAUGE,
  STATE_TRANSITION,
  STATE_MAIN
};

AppState appState = STATE_BOOT;
int selectedIndex = 0;
bool needsRedraw = true;
unsigned long transitionStart = 0;

// ============================================================
// ブート画面描画関数（元のコードをそのまま保持）
// ============================================================

void drawBackground() {
  M5Dial.Display.fillScreen(COL_BG);
}

void drawHexGrid() {
  uint16_t col = M5Dial.Display.color565(10, 40, 20);
  const int hexPoints[][2] = {
    {120, 20}, {150, 37}, {150, 71}, {120, 88}, {90, 71}, {90, 37}
  };
  for (int i = 0; i < 6; i++) {
    int next = (i + 1) % 6;
    M5Dial.Display.drawLine(hexPoints[i][0], hexPoints[i][1],
                        hexPoints[next][0], hexPoints[next][1], col);
  }
  const int hex2[][2] = {
    {120, 90}, {150, 107}, {150, 141}, {120, 158}, {90, 141}, {90, 107}
  };
  for (int i = 0; i < 6; i++) {
    int next = (i + 1) % 6;
    M5Dial.Display.drawLine(hex2[i][0], hex2[i][1],
                        hex2[next][0], hex2[next][1], col);
  }
}

void drawCornerBrackets() {
  uint16_t col = M5Dial.Display.color565(15, 60, 30);
  int len = 10;
  int margin = 28;
  M5Dial.Display.drawLine(margin, margin, margin + len, margin, col);
  M5Dial.Display.drawLine(margin, margin, margin, margin + len, col);
  M5Dial.Display.drawLine(240 - margin, margin, 240 - margin - len, margin, col);
  M5Dial.Display.drawLine(240 - margin, margin, 240 - margin, margin + len, col);
  M5Dial.Display.drawLine(margin, 240 - margin, margin + len, 240 - margin, col);
  M5Dial.Display.drawLine(margin, 240 - margin, margin, 240 - margin - len, col);
  M5Dial.Display.drawLine(240 - margin, 240 - margin, 240 - margin - len, 240 - margin, col);
  M5Dial.Display.drawLine(240 - margin, 240 - margin, 240 - margin, 240 - margin - len, col);
}

void drawGaugeBackground() {
  for (int deg = -210; deg <= 30; deg += 2) {
    float rad = deg * PI / 180.0;
    int x = GAUGE_CX + (int)(GAUGE_R * cos(rad));
    int y = GAUGE_CY + (int)(GAUGE_R * sin(rad));
    M5Dial.Display.drawPixel(x, y, COL_GREEN_DIM);
    x = GAUGE_CX + (int)((GAUGE_R - 1) * cos(rad));
    y = GAUGE_CY + (int)((GAUGE_R - 1) * sin(rad));
    M5Dial.Display.drawPixel(x, y, COL_GREEN_DIM);
  }
  for (int i = 0; i < 7; i++) {
    int deg = -210 + i * 40;
    float rad = deg * PI / 180.0;
    int x1 = GAUGE_CX + (int)(GAUGE_R * cos(rad));
    int y1 = GAUGE_CY + (int)(GAUGE_R * sin(rad));
    int x2 = GAUGE_CX + (int)((GAUGE_R - 7) * cos(rad));
    int y2 = GAUGE_CY + (int)((GAUGE_R - 7) * sin(rad));
    M5Dial.Display.drawLine(x1, y1, x2, y2, COL_GREEN_DIM);
  }
  M5Dial.Display.drawCircle(GAUGE_CX, GAUGE_CY, 28, COL_GREEN_DIM);
}

void drawGpsPin(uint16_t col) {
  int cx = GAUGE_CX;
  int cy = GAUGE_CY;
  M5Dial.Display.drawCircle(cx, cy - 6, 8, col);
  M5Dial.Display.drawLine(cx - 6, cy - 1, cx, cy + 10, col);
  M5Dial.Display.drawLine(cx + 6, cy - 1, cx, cy + 10, col);
  M5Dial.Display.drawCircle(cx, cy - 6, 3, col);
}

void drawKCBText() {
  M5Dial.Display.setTextColor(COL_GREEN);
  M5Dial.Display.setTextDatum(top_center);
  M5Dial.Display.setFont(&fonts::FreeSansBold18pt7b);
  M5Dial.Display.drawString("KCB", CX, 140);
  M5Dial.Display.setFont(&fonts::lgfxJapanGothic_16);
  M5Dial.Display.setTextColor(M5Dial.Display.color565(40, 140, 70));
  M5Dial.Display.drawString("運行管理システム", CX, 177);
}

void drawModuleIndicators() {
  M5Dial.Display.setFont(&fonts::Font0);
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextDatum(middle_left);
  for (int i = 0; i < MOD_COUNT; i++) {
    int x = modX[i];
    int y = modY[i];
    if (modReady[i]) {
      M5Dial.Display.fillCircle(x, y, dotR, COL_GREEN);
    } else {
      M5Dial.Display.drawCircle(x, y, dotR, COL_GRAY_DIM);
    }
  }
  M5Dial.Display.setTextDatum(top_center);
  M5Dial.Display.setFont(&fonts::Font0);
  M5Dial.Display.setTextSize(1);
  for (int i = 0; i < MOD_COUNT; i++) {
    uint16_t col = modReady[i] ? COL_GREEN : COL_GRAY_DIM;
    M5Dial.Display.setTextColor(col);
    M5Dial.Display.drawString(modNames[i], modX[i], modY[i] + dotR + 3);
  }
}

void drawGaugeActive() {
  for (int deg = -210; deg <= -130; deg += 2) {
    float rad = deg * PI / 180.0;
    for (int t = 0; t < 3; t++) {
      int x = GAUGE_CX + (int)((GAUGE_R - t) * cos(rad));
      int y = GAUGE_CY + (int)((GAUGE_R - t) * sin(rad));
      M5Dial.Display.drawPixel(x, y, COL_GREEN);
    }
    if (deg % 10 == 0) delay(5);
  }
  // 針アニメーション（-210度から-130度まで徐々に移動）
  for (int deg = -210; deg <= -130; deg += 2) {
    // 前の針を消す（中心ドットより外側）
    if (deg > -210) {
      float prevRad = (deg - 2) * PI / 180.0;
      int px = GAUGE_CX + (int)(50 * cos(prevRad));
      int py = GAUGE_CY + (int)(50 * sin(prevRad));
      M5Dial.Display.drawLine(GAUGE_CX, GAUGE_CY, px, py, COL_BG);
      M5Dial.Display.drawLine(GAUGE_CX + 1, GAUGE_CY, px + 1, py, COL_BG);
    }
    // 新しい針を描画
    float rad = deg * PI / 180.0;
    int nx = GAUGE_CX + (int)(50 * cos(rad));
    int ny = GAUGE_CY + (int)(50 * sin(rad));
    M5Dial.Display.drawLine(GAUGE_CX, GAUGE_CY, nx, ny, COL_GREEN);
    M5Dial.Display.drawLine(GAUGE_CX + 1, GAUGE_CY, nx + 1, ny, COL_GREEN);
    M5Dial.Display.fillCircle(GAUGE_CX, GAUGE_CY, 4, COL_GREEN);
    delay(8);
  }
  drawGpsPin(COL_GREEN);
}

void drawBootScreen() {
  for (int i = 0; i < MOD_COUNT; i++) {
    modReady[i] = false;
  }
  drawBackground();
  drawHexGrid();
  drawCornerBrackets();
  drawGaugeBackground();
  drawKCBText();
  drawModuleIndicators();
}

void setModuleReady(int moduleIndex) {
  if (moduleIndex < 0 || moduleIndex >= MOD_COUNT) return;
  modReady[moduleIndex] = true;
  int x = modX[moduleIndex];
  int y = modY[moduleIndex];
  M5Dial.Display.fillCircle(x, y, dotR + 1, COL_BG);
  M5Dial.Display.fillCircle(x, y, dotR, COL_GREEN);
  M5Dial.Display.setFont(&fonts::Font0);
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextDatum(top_center);
  M5Dial.Display.setTextColor(COL_GREEN, COL_BG);
  M5Dial.Display.drawString(modNames[moduleIndex], x, y + dotR + 3);
}
// ============================================================
// NFC初期化・読み取り
// ============================================================
bool initNFC() {
  // M5Dial.begin() が内蔵RFID(Wire1)を初期化済み
  byte v = M5Dial.Rfid.PICC_IsNewCardPresent();  // ダミー呼び出しでウェイクアップ
  // バージョン確認
  byte ver = M5Dial.Rfid.PCD_ReadRegister(0x37);  // VersionReg = 0x37
  if (ver == 0x00 || ver == 0xFF) {
    Serial.println("NFC: WS1850S not found");
    return false;
  }
  Serial.printf("NFC: WS1850S found (v=0x%02X)\n", ver);
  return true;
}

String readNFCCard() {
  if (!M5Dial.Rfid.PICC_IsNewCardPresent()) return "";
  if (!M5Dial.Rfid.PICC_ReadCardSerial()) return "";
  
  String uid = "";
  for (byte i = 0; i < M5Dial.Rfid.uid.size; i++) {
    if (i > 0) uid += ":";
    if (M5Dial.Rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(M5Dial.Rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  M5Dial.Rfid.PICC_HaltA();
  return uid;
}

void pollNFC() {
  if (!nfcReady) return;
  if (millis() - lastNfcRead < NFC_READ_INTERVAL) return;
  lastNfcRead = millis();
  
  String uid = readNFCCard();
  if (uid.length() > 0) {
    currentDriverUID = uid;
    displayUID = uid;
    currentDriverName = lookupDriver(uid);
    saveTripState(); // ← 追加：NFC読み取り時は即時保存
    Serial.printf("NFC: UID=%s -> %s\n", uid.c_str(), currentDriverName.c_str());
    
    // ブザーフィードバック
    M5Dial.Speaker.tone(2000, 80);
    delay(80);
    M5Dial.Speaker.tone(2600, 80);
    
    needsRedraw = true;
  }
}

// ============================================================
// メイン画面描画関数
// ============================================================

void drawMainScreen() {
  // 背景（ブート画面と同系色のダークグリーン）
  M5Dial.Display.fillScreen(COL_BG);
  
  // アイコン軌道の弧（薄い線）
  for (int deg = -155; deg <= -25; deg++) {
    float rad = deg * PI / 180.0;
    int x = CX + (int)(ICON_ORBIT_R * cos(rad));
    int y = CY + (int)(ICON_ORBIT_R * sin(rad));
    M5Dial.Display.drawPixel(x, y, COL_DIM_LINE);
  }
  
  // メニューアイコン描画
  drawMenuIcons();
  
  // ページコンテンツ描画
  switch (selectedIndex) {
    case 0: drawStatusPage();  break;
    case 1: drawGpsPage();     break;
    case 2: drawObdPage();     break;
    case 3: drawDriverPage();  break;
    case 4: drawConfigPage();  break;
  }
  
  // 下部ヒント
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
  M5Dial.Display.setTextColor(COL_DIM_LINE);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString("< タッチで切替 >", CX, 225);
}

void drawMenuIcons() {
  for (int i = 0; i < MENU_COUNT; i++) {
    float rad = iconAngles[i] * PI / 180.0;
    int ix = CX + (int)(ICON_ORBIT_R * cos(rad));
    int iy = CY + (int)(ICON_ORBIT_R * sin(rad));
    
    bool active = (i == selectedIndex);
    int iconHalf = active ? 20 : 14;
    
    if (active) {
      // アクティブ: 塗りつぶし背景 + 枠線
      M5Dial.Display.fillCircle(ix, iy, iconHalf + 2, M5Dial.Display.color565(10, 20, 10));
      M5Dial.Display.drawCircle(ix, iy, iconHalf + 2, menuItems[i].color);
    } else {
      // 非アクティブ: 薄い背景のみ
      M5Dial.Display.fillCircle(ix, iy, iconHalf + 1, M5Dial.Display.color565(4, 8, 4));
    }
    
    // PNGアイコン描画
    const uint8_t* iconData = active ? icons[i].active : icons[i].inactive;
    size_t iconLen = active ? icons[i].activeLen : icons[i].inactiveLen;
    M5Dial.Display.drawPng(iconData, iconLen, ix - iconHalf, iy - iconHalf);
  }
}

// ============================================================
// ページコンテンツ
// ============================================================

void drawStatusPage() {
  // 現在時刻（大きく上部に）
  auto dt = M5Dial.Rtc.getDateTime();
  char timeBuf[6];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", dt.time.hours, dt.time.minutes);
  M5Dial.Display.setFont(&fonts::Font4);
  M5Dial.Display.setTextColor(COL_GREEN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString(timeBuf, CX, CY - 30);
  
  // 日付（小さく時刻の下に）
  char dateBuf[11];
  snprintf(dateBuf, sizeof(dateBuf), "%04d/%02d/%02d", dt.date.year, dt.date.month, dt.date.date);
  M5Dial.Display.setFont(&fonts::Font4);
  M5Dial.Display.setTextColor(COL_GREEN_DIM);
  M5Dial.Display.drawString(dateBuf, CX, CY);
  
  // ステータス情報
  String driverStr = (currentDriverName.length() > 0 && currentDriverName != "unregistered") 
    ? currentDriverName : "未登録";
  M5Dial.Display.drawString("運転者: " + driverStr, CX, CY + 24);
  M5Dial.Display.drawString(tripActive ? "運行中" : "待機中", CX, CY + 40);
}

void drawGpsPage() {
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
  
  M5Dial.Display.setTextColor(COL_CYAN);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString("GPS情報", CX, CY - 50);
  
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
  
  // 衛星数
  if (gps.satellites.isValid()) {
    M5Dial.Display.setTextColor(COL_GREEN);
    char satBuf[20];
    snprintf(satBuf, sizeof(satBuf), "衛星: %d基", (int)gps.satellites.value());
    M5Dial.Display.drawString(satBuf, CX, CY - 28);
  } else {
    M5Dial.Display.setTextColor(0x7800);
    M5Dial.Display.drawString("衛星: 受信待ち", CX, CY - 28);
  }
  
  // 運行状態
  M5Dial.Display.setTextColor(tripActive ? COL_GREEN : COL_WHITE_DIM);
  M5Dial.Display.drawString(tripActive ? "運行中" : "待機中", CX, CY - 10);
  
  // 現在座標
  if (gps.location.isValid()) {
    M5Dial.Display.setTextColor(COL_WHITE_DIM);
    char posBuf[28];
    snprintf(posBuf, sizeof(posBuf), "%.5f, %.5f", gps.location.lat(), gps.location.lng());
    M5Dial.Display.drawString(posBuf, CX, CY + 8);
  } else {
    M5Dial.Display.setTextColor(0x3186);
    M5Dial.Display.drawString("測位中...", CX, CY + 8);
  }
  
  // 最遠地点距離
  M5Dial.Display.setTextColor(COL_CYAN);
  if (farthestDist > 0) {
    char distBuf[24];
    if (farthestDist >= 1000) {
      snprintf(distBuf, sizeof(distBuf), "最遠: %.1f km", farthestDist / 1000.0);
    } else {
      snprintf(distBuf, sizeof(distBuf), "最遠: %.0f m", farthestDist);
    }
    M5Dial.Display.drawString(distBuf, CX, CY + 28);
  } else {
    M5Dial.Display.drawString("最遠: ---", CX, CY + 28);
  }
  
  // 基準点からの現在距離
  if (gps.location.isValid()) {
    double currentDist = calcDistance(baseLat, baseLng, gps.location.lat(), gps.location.lng());
    M5Dial.Display.setTextColor(0x3186);
    char curDistBuf[24];
    snprintf(curDistBuf, sizeof(curDistBuf), "現在: %.0f m", currentDist);
    M5Dial.Display.drawString(curDistBuf, CX, CY + 46);
  }
  if (currentAddress.length() > 0) {
    M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
    M5Dial.Display.setTextColor(COL_GREEN);
    M5Dial.Display.drawString(currentAddress, CX, CY + 58);
  }
}

void drawObdPage() {
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
  M5Dial.Display.setTextColor(COL_ORANGE);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString("OBD2", CX, CY - 10);
  
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
  M5Dial.Display.setTextColor(0x7A00);
  M5Dial.Display.drawString("接続: 未接続", CX, CY + 10);
  M5Dial.Display.drawString("走行距離: ---", CX, CY + 26);
  M5Dial.Display.drawString("PID 0xA6: 未確認", CX, CY + 42);
}

void drawDriverPage() {
  M5Dial.Display.setTextDatum(middle_center);
  
  if (currentDriverUID.length() > 0) {
    // --- カード読み取り済み ---
    M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_20);
    M5Dial.Display.setTextColor(COL_PURPLE);
    M5Dial.Display.drawString("ドライバー", CX, CY - 36);
    
    // ドライバー名（照合結果）
    if (currentDriverName.length() > 0 && currentDriverName != "unregistered") {
      M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_20);
      M5Dial.Display.setTextColor(COL_GREEN);
      M5Dial.Display.drawString(currentDriverName, CX, CY);
    } else {
      M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
      M5Dial.Display.setTextColor(0x7800);  // 赤系
      M5Dial.Display.drawString("未登録カード", CX, CY);
    }
    
    // UID表示（小さく下部に）
    M5Dial.Display.setFont(&fonts::Font0);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(COL_WHITE_DIM);
    M5Dial.Display.drawString(displayUID, CX, CY + 30);
  } else {
    // --- 待機中 ---
    M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
    M5Dial.Display.setTextColor(COL_PURPLE);
    M5Dial.Display.drawString("ドライバー", CX, CY - 20);
    
    M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
    M5Dial.Display.setTextColor(0x380F);
    M5Dial.Display.drawString("カード待機中", CX, CY + 4);
    
    // NFCタッチ促進の点滅風表示
    uint16_t col = (millis() / 800 % 2 == 0) ? COL_PURPLE : 0x380F;
    M5Dial.Display.setTextColor(col);
    M5Dial.Display.drawString("NFCカードをタッチ", CX, CY + 28);
    
    if (!nfcReady) {
      M5Dial.Display.setTextColor(0x7800);  // 赤系
      M5Dial.Display.drawString("NFCモジュール未検出", CX, CY + 48);
    }
  }
}

void drawOBD2Page() {
  M5Dial.Display.fillScreen(COL_BG);
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
  M5Dial.Display.setTextColor(COL_GRAY);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString("OBD2", CX, CY - 10);

  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
  M5Dial.Display.setTextColor(0x3186);
  M5Dial.Display.drawString("未接続", CX, CY + 10);
}

void drawConfigPage() {
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
  M5Dial.Display.setTextColor(COL_GRAY);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString("設定", CX, CY - 42);

  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_12);
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  M5Dial.Display.setTextColor(wifiOk ? TFT_GREEN : TFT_RED);
  M5Dial.Display.drawString(wifiOk ? "WiFi: 接続中" : "WiFi: 未接続", CX, CY - 22);

  // S3同期（選択中は白、非選択はグレー）
  M5Dial.Display.setTextColor(configSelectedIndex == 0 ? TFT_WHITE : 0x3186);
  M5Dial.Display.drawString("S3同期", CX, CY + 2);

  // WiFi設定
  M5Dial.Display.setTextColor(configSelectedIndex == 1 ? TFT_WHITE : 0x3186);
  M5Dial.Display.drawString("WiFi設定", CX, CY + 22);

  // デバッグ: 手動入力テスト ← 追加
  M5Dial.Display.setTextColor(configSelectedIndex == 2 ? TFT_WHITE : 0x3186);
  M5Dial.Display.drawString("[DBG]手動入力", CX, CY + 42);
  
  // 基準地点設定
  M5Dial.Display.setTextColor(configSelectedIndex == 3 ? TFT_WHITE : 0x3186);
  M5Dial.Display.drawString("現在地を基準に設定", CX, CY + 58);  // ← ここはCY+42から-4ずつずらした方が良いかも要調整

  // 選択インジケーター（テキストの左横に小さい丸）
  // 選択インジケーター：両方を一旦黒で消してから選択中だけ白で描く
  M5Dial.Display.fillCircle(CX - 45, CY + 2,  3, BLACK);
  M5Dial.Display.fillCircle(CX - 45, CY + 22, 3, BLACK);
  M5Dial.Display.fillCircle(CX - 45, CY + 42, 3, BLACK);
  M5Dial.Display.fillCircle(CX - 45, CY + 58, 3, BLACK); 
  int indicatorY = (configSelectedIndex == 0) ? CY + 2  :
                   (configSelectedIndex == 1) ? CY + 22 :
                   (configSelectedIndex == 2) ? CY + 42 : CY + 58;  // ← 修正
  M5Dial.Display.fillCircle(CX - 45, indicatorY, 3, TFT_WHITE);

  M5Dial.Display.setFont(&fonts::Font2);
  M5Dial.Display.setTextColor(TFT_DARKGREY);
  M5Dial.Display.drawString("long press: back", CX, CY + 68);
}

// ============================================================
// 入力処理
// ============================================================
void handleInput() {

  if (currentScreen == SCREEN_MANUAL_DIST) {
    long newPos = M5Dial.Encoder.read();
    long diff = newPos - lastEncoderPos;
    if (diff >= 2) {
      manualDistKm++;
      drawManualDistInput();
      lastEncoderPos = newPos;
    } else if (diff <= -2) {
      if (manualDistKm > 0) manualDistKm--;
      drawManualDistInput();
      lastEncoderPos = newPos;
    }
    if (M5Dial.BtnA.wasPressed()) {
      Serial.printf("[Trip] 手動入力確定: %u km\n", manualDistKm);
      saveTripLocal(manualDistKm);
      resetTrip();
      currentScreen = SCREEN_MENU;
      needsRedraw = true;
    }
    return;
  }

  auto t = M5Dial.Touch.getDetail();
  
  // タッチ: 左半分=前、右半分=次
  if (t.wasPressed()) {
    if (t.x < CX) {
      if (selectedIndex > 0) {
        selectedIndex--;
        needsRedraw = true;
      }
    } else {
      if (selectedIndex < MENU_COUNT - 1) {
        selectedIndex++;
        needsRedraw = true;
      }
    }
  }
  
  // ダイヤル回転: メニュー切替
  long newPos = M5Dial.Encoder.read();
  long diff = newPos - lastEncoderPos;
  if (diff >= 2) {
    if (currentScreen == SCREEN_CONFIG) {
      configSelectedIndex = (configSelectedIndex + 1) % CONFIG_ITEMS;
      needsRedraw = true;
    } else if (selectedIndex < MENU_COUNT - 1) {
      selectedIndex++;
      needsRedraw = true;
    }
    lastEncoderPos = newPos;
  } else if (diff <= -2) {
    if (currentScreen == SCREEN_CONFIG) {
      configSelectedIndex = (configSelectedIndex - 1 + CONFIG_ITEMS) % CONFIG_ITEMS;
      needsRedraw = true;
    } else if (selectedIndex > 0) {
      selectedIndex--;
      needsRedraw = true;
    }
    lastEncoderPos = newPos;
  }  
  // ボタン押下: 画面遷移
  if (M5Dial.BtnA.wasPressed()) {
    Serial.printf("[BTN] pressed. currentScreen=%d, selectedIndex=%d\n",
    currentScreen, selectedIndex);
    if (currentScreen == SCREEN_MENU) {
      if (selectedIndex == 1) {
        currentScreen = SCREEN_GPS;
        needsRedraw = true;
      } else if (selectedIndex == 4) {
        // ↓ S3同期から変更: SCREEN_CONFIG に遷移
        configSelectedIndex = 0;
        currentScreen = SCREEN_CONFIG;
        needsRedraw = true;
      } else {
        M5Dial.Display.fillCircle(CX, CY, 30, menuItems[selectedIndex].color);
        delay(100);
        needsRedraw = true;
      }
    } else if (currentScreen == SCREEN_CONFIG) {
      // ↓ 追加: 設定画面内での項目実行
      if (configSelectedIndex == 0) {
        syncWithS3();
        needsRedraw = true;
      } else if (configSelectedIndex == 1) {
        startAPMode();  // 内部で drawAPModePage() も呼ぶ
      } else if (configSelectedIndex == 2) {  // ← 追加
        tripDistAccum = 42.5;
        startManualDistInput();
      } else if (configSelectedIndex == 3) {
        if (gps.location.isValid()) {
          baseLat = gps.location.lat();
          baseLng = gps.location.lng();
          saveBaseConfig();
          clearTripState();  // ← 追加：基準地点変更時に運行中ステータスをリセット
          tripActive = false;
          tripStartTime = 0;
          tripDistAccum = 0.0;
          Serial.printf("[Base] updated: %.6f, %.6f\n", baseLat, baseLng);
          M5Dial.Speaker.tone(1000, 200);
        } else {
          Serial.println("[Base] GPS not valid, skipped");
          M5Dial.Speaker.tone(300, 500);
        }
        needsRedraw = true;
      }
    } else {
      currentScreen = SCREEN_MENU;
      needsRedraw = true;
    }
  }

  // ↓ 追加: 長押しで設定画面からメニューに戻る
  if (M5Dial.BtnA.wasHold()) {
    if (currentScreen == SCREEN_CONFIG) {
      currentScreen = SCREEN_MENU;
      needsRedraw = true;
    }
  }
}

void drawGPSInfo() {
  auto& lcd = M5Dial.Display;
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);
  lcd.setFont(&fonts::Font2);

  lcd.setCursor(60, 30);
  lcd.print("=== GPS Info ===");

  if (gps.location.isValid()) {
    lcd.setCursor(60, 60);
    lcd.printf("Lat: %.6f", gps.location.lat());
    lcd.setCursor(60, 80);
    lcd.printf("Lng: %.6f", gps.location.lng());
  } else {
    lcd.setCursor(60, 60);
    lcd.print("Lat: ---");
    lcd.setCursor(60, 80);
    lcd.print("Lng: ---");
  }

  lcd.setCursor(60, 110);
  lcd.printf("Sats: %d", gps.satellites.value());

  lcd.setCursor(60, 140);
  if (gps.speed.isValid()) {
    lcd.printf("Speed: %.1f km/h", gps.speed.kmph());
  } else {
    lcd.print("Speed: ---");
  }

  lcd.setCursor(60, 160);
  lcd.printf("Chars: %lu", gps.charsProcessed());

  lcd.setCursor(60, 170);
  lcd.printf("Trip: %s", tripActive ? "Active" : "Idle");

  lcd.setCursor(60, 190);
  lcd.printf("Far: %.0f m", farthestDist);
  lcd.setCursor(60, 200);
  lcd.print("[DIAL] Back");
}

// 2点間の距離（メートル）を計算（ハーバーサイン公式）
double calcDistance(double lat1, double lon1, double lat2, double lon2) {
  double R = 6371000.0;
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radians(lat1)) * cos(radians(lat2)) *
             sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void updateFarthestPoint() {
  if (!gps.location.isValid()) return;
  if (currentScreen == SCREEN_MANUAL_DIST) return;

  double lat = gps.location.lat();
  double lng = gps.location.lng();
  double distFromBase = calcDistance(baseLat, baseLng, lat, lng);

  // 基準点から離れたら運行開始
  if (!tripActive && distFromBase > RETURN_RADIUS) {
    if (millis() < tripCooldownUntil) return; 
    tripActive = true;
    tripStartTime = millis();
    farthestLat = lat;
    farthestLng = lng;
    farthestDist = distFromBase;
    tripDistAccum = 0.0;
    prevPosValid = false;
    tripStartOdometer = getOBD2Odometer();
    Serial.printf("[Trip] 開始 odometer=%u km\n", tripStartOdometer);
  }

  // 運行中: 最遠地点更新 + GPS積算
  if (tripActive) {
    if (distFromBase > farthestDist) {
      farthestLat = lat;
      farthestLng = lng;
      farthestDist = distFromBase;
    }

    // GPS積算（5km/h以上の時のみ）
    if (gps.speed.isValid() && gps.speed.kmph() >= 5.0) {
      if (prevPosValid) {
        float d = calcDistance(prevLat, prevLng, lat, lng);
        tripDistAccum += d;
      }
      prevLat = lat;
      prevLng = lng;
      prevPosValid = true;
    } else {
      prevPosValid = false;  // 停車中はリセットして再発進時の誤差防止
    }
  }

  // 帰社判定
  if (tripActive && distFromBase <= RETURN_RADIUS) {
    if (millis() - tripStartTime < 60000) return;
    uint32_t endOdometer = getOBD2Odometer();
    uint32_t obd2Dist = (endOdometer > 0 && tripStartOdometer > 0 && endOdometer >= tripStartOdometer)
                        ? endOdometer - tripStartOdometer
                        : 0;

    if (obd2Dist > 0) {
      // OBD2成功 → そのまま保存
      Serial.printf("[Trip] OBD2距離: %u km\n", obd2Dist);
      saveTripLocal(obd2Dist);
      resetTrip();
    } else {
      // OBD2失敗 → 手動入力モードへ
      Serial.printf("[Trip] GPS積算距離: %.1f km\n", tripDistAccum);
      startManualDistInput();
    }
  }
}

void resetTrip() {
  clearTripState(); // ← 追加：帰社保存完了後にファイル削除
  tripActive = false;
  farthestDist = 0.0;
  tripStartOdometer = 0;
  tripDistAccum = 0.0;
  prevPosValid = false;
  tripCooldownUntil = millis() + 30000;  // ← 追加（30秒クールダウン）
}

void saveTripLocal(uint32_t distKm) {
  JsonDocument doc;
  
  File f = SPIFFS.open("/trips.json", "r");
  if (f) {
    deserializeJson(doc, f);
    f.close();
  }
  if (!doc.is<JsonArray>()) {
    doc.to<JsonArray>();
  }

  JsonObject trip = doc.as<JsonArray>().add<JsonObject>();
  trip["date"]        = getRTCDateTime();
  trip["driver_uid"]  = currentDriverUID;          // "driver" から変更
  trip["driver_name"] = lookupDriver(currentDriverUID); // 新規追加
  trip["address"] = getAddressAt(farthestLat, farthestLng);
  //trip["dist"]        = farthestDist;
  //trip["dist"]        = obd2DistKm;
  //trip["dist"]        = distKm;
  doc["odometer"] = distKm;
  lastOdometer    = distKm;  // ← 追加：次回起動時のために保持
  saveTripState();            // ← 追加：last_odometerをSPIFFSに反映
  trip["lat"]         = farthestLat;
  trip["lng"]         = farthestLng;

  File out = SPIFFS.open("/trips.json", "w");
  if (!out) {
    Serial.println("[SPIFFS] trips.json open failed");
    return;
  }
  serializeJson(doc, out);
  out.close();
}
// trip_state.json をSPIFFSに保存
void saveTripState() {
  JsonDocument doc;
  doc["status"]          = (tripStartTime != 0) ? "active" : "idle";
  doc["last_odometer"]   = lastOdometer;
  doc["driver_uid"]      = currentDriverUID;
  doc["driver_name"]     = currentDriverName;
  doc["farthest_lat"]    = farthestLat;
  doc["farthest_lng"]    = farthestLng;
  doc["trip_dist_accum"] = tripDistAccum;
  auto dt = M5Dial.Rtc.getDateTime();
  char dateStr[11];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
           dt.date.year, dt.date.month, dt.date.date);
  doc["trip_start_date"] = dateStr;

  File f = SPIFFS.open("/trip_state.json", FILE_WRITE);
  if (f) {
    serializeJson(doc, f);
    f.close();
    Serial.println("[TripState] saved");
  } else {
    Serial.println("[TripState] save FAILED");
  }
}

bool loadTripState() {
  if (!SPIFFS.exists("/trip_state.json")) return false;
  File f = SPIFFS.open("/trip_state.json", FILE_READ);
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.println("[TripState] parse error");
    return false;
  }

  // last_odometerは常に読み込む（idle時も必要）
  lastOdometer = doc["last_odometer"] | 0;

  String status = doc["status"] | String("idle");
  if (status != "active") {
    Serial.printf("[TripState] status=idle, last_odometer=%u\n", lastOdometer);
    return false;
  }

  currentDriverUID  = doc["driver_uid"]      | String("");
  currentDriverName = doc["driver_name"]     | String("");
  farthestLat       = doc["farthest_lat"]    | 0.0;
  farthestLng       = doc["farthest_lng"]    | 0.0;
  tripDistAccum     = doc["trip_dist_accum"] | 0.0f;
  tripStartTime     = millis() - 120000UL;
  tripActive        = true;
  Serial.printf("[TripState] restored: driver=%s lat=%.4f lng=%.4f dist=%.1fkm odometer=%u\n",
                currentDriverName.c_str(), farthestLat, farthestLng, tripDistAccum, lastOdometer);
  return true;
}

void clearTripState() {
  tripActive        = false;
  tripStartTime     = 0;
  tripDistAccum     = 0.0;
  currentDriverUID  = "";
  currentDriverName = "";
  farthestLat       = 0.0;
  farthestLng       = 0.0;
  saveTripState();  // status="idle"・last_odometerは保持したまま保存
  Serial.println("[TripState] cleared to idle");
}

// ドライバーマスタからUID照合
String lookupDriver(String uid) {
  File f = SPIFFS.open("/drivers.json", "r");
  if (!f) return "unknown";

  JsonDocument doc;
  deserializeJson(doc, f);
  f.close();
  Serial.println("[SPIFFS] trips.json saved OK");  

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject d : arr) {
    if (d["uid"].as<String>() == uid) {
      return d["name"].as<String>();
    }
  }
  return "unregistered";
}

// drivers.json が無ければサンプルデータを作成
void initDriversJson() {
  if (SPIFFS.exists("/drivers.json")) return;
  
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  
  // サンプルデータ（後でS3同期で上書きされる）
  JsonObject d1 = arr.add<JsonObject>();
  d1["uid"] = "08:8E:A9:A3";
  d1["name"] = "高野個人携帯";
  
  JsonObject d2 = arr.add<JsonObject>();
  d2["uid"] = "04:66:30:4A:4B:4D:80";
  d2["name"] = "高野　無我";
  
  JsonObject d3 = arr.add<JsonObject>();
  d3["uid"] = "01:02:03:04";
  d3["name"] = "高野会社携帯";
  
  File f = SPIFFS.open("/drivers.json", "w");
  serializeJson(doc, f);
  f.close();
  Serial.println("drivers.json created with sample data");
}

bool rtcSynced = false;

void syncRTCFromGPS() {
  if (rtcSynced) return;
  if (!gps.date.isValid() || !gps.time.isValid()) return;
  if (gps.date.year() < 2024) return;  // 無効な日付を除外
  
  m5::rtc_datetime_t dt;
  dt.date.year = gps.date.year();
  dt.date.month = gps.date.month();
  dt.date.date = gps.date.day();
  dt.time.hours = gps.time.hour() + 9;  // UTC → JST
  dt.time.minutes = gps.time.minute();
  dt.time.seconds = gps.time.second();
  
  // 日付繰り上がり処理
  if (dt.time.hours >= 24) {
    dt.time.hours -= 24;
    dt.date.date += 1;  // 簡易処理（月末は厳密でないが実用上問題なし）
  }
  
  M5Dial.Rtc.setDateTime(dt);
  rtcSynced = true;
  Serial.println("RTC synced from GPS");
}

String getRTCDateTime() {
  auto dt = M5Dial.Rtc.getDateTime();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
    dt.date.year, dt.date.month, dt.date.date,
    dt.time.hours, dt.time.minutes, dt.time.seconds);
  return String(buf);
}

void startWiFiConnect() {
  if (apModeActive) return;
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
  wifiConnecting = true;
  wifiConnectStart = millis();
  Serial.println("WiFi connecting...");
}

void updateWiFiStatus() {
  if (apModeActive) return;
  if (!wifiConnecting) return;
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnecting = false;
    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  } else if (millis() - wifiConnectStart > WIFI_TIMEOUT) {
    wifiConnecting = false;
    WiFi.disconnect();
    Serial.println("WiFi timeout");
  }
}

String getAddressAt(double lat, double lng) {
  if (WiFi.status() != WL_CONNECTED) return "住所取得失敗";
  
  HTTPClient http;
  String url = "https://geoapi.heartrails.com/api/json?method=searchByGeoLocation&x="
    + String(lng, 6) + "&y=" + String(lat, 6);
  
  http.begin(url);
  http.setTimeout(5000);
  int httpCode = http.GET();
  String result = "住所取得失敗";
  
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    deserializeJson(doc, payload);
    String city = doc["response"]["location"][0]["city"].as<String>();
    String town = doc["response"]["location"][0]["town"].as<String>();
    if (city.length() > 0) result = city + town;
  }
  
  http.end();
  return result;
}

void updateAddress() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!gps.location.isValid()) return;
  if (millis() - lastAddressUpdate < ADDRESS_INTERVAL && currentAddress.length() > 0) return;
  
  HTTPClient http;
  String url = "https://geoapi.heartrails.com/api/json?method=searchByGeoLocation&x="
    + String(gps.location.lng(), 6) + "&y=" + String(gps.location.lat(), 6);  // x=経度, y=緯度
  
  http.begin(url);
  http.setTimeout(5000);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    deserializeJson(doc, payload);
    
    String city = doc["response"]["location"][0]["city"].as<String>();  // 例: 北見市
    String town = doc["response"]["location"][0]["town"].as<String>();  // 例: 東相内町
    
    if (city.length() > 0) {
      currentAddress = city + town;
    } else {
      currentAddress = "住所取得失敗";
    }
    Serial.printf("Address: %s\n", currentAddress.c_str());
  } else {
    Serial.printf("HeartRails API error: %d\n", httpCode);
  }
  
  http.end();
  lastAddressUpdate = millis();
}

// バイト列 → 16進文字列
String toHexStr(const uint8_t* buf, size_t len) {
  String result = "";
  for (size_t i = 0; i < len; i++) {
    char hex[3];
    sprintf(hex, "%02x", buf[i]);
    result += hex;
  }
  return result;
}

// SHA256 → 16進文字列
String sha256Hex(const String& data) {
  uint8_t hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, (const uint8_t*)data.c_str(), data.length());
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
  return toHexStr(hash, 32);
}

// HMAC-SHA256
void hmacSHA256(const uint8_t* key, size_t keyLen,
                const uint8_t* data, size_t dataLen,
                uint8_t* out) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, key, keyLen);
  mbedtls_md_hmac_update(&ctx, data, dataLen);
  mbedtls_md_hmac_finish(&ctx, out);
  mbedtls_md_free(&ctx);
}

// AWS Signature V4 署名キー生成
void getSigningKey(const String& dateStamp, uint8_t* signingKey) {
  String secretPrefix = "AWS4" + String(AWS_SECRET_ACCESS_KEY);
  uint8_t kDate[32], kRegion[32], kService[32], kSigning[32];
  hmacSHA256((uint8_t*)secretPrefix.c_str(), secretPrefix.length(),
             (uint8_t*)dateStamp.c_str(), dateStamp.length(), kDate);
  hmacSHA256(kDate, 32, (uint8_t*)AWS_REGION, strlen(AWS_REGION), kRegion);
  hmacSHA256(kRegion, 32, (uint8_t*)"s3", 2, kService);
  hmacSHA256(kService, 32, (uint8_t*)"aws4_request", 12, kSigning);
  memcpy(signingKey, kSigning, 32);
}

String buildS3AuthHeader(const String& method, const String& objectKey,
                         const String& payloadHash, const String& dateTime,
                         const String& dateStamp, const String& contentType) {
  String canonicalHeaders;
  String signedHeaders;

  if (contentType.length() > 0) {
    canonicalHeaders = "content-type:" + contentType + "\n"
                     + "host:" + String(S3_HOST) + "\n"
                     + "x-amz-content-sha256:" + payloadHash + "\n"
                     + "x-amz-date:" + dateTime + "\n";
    signedHeaders = "content-type;host;x-amz-content-sha256;x-amz-date";
  } else {
    canonicalHeaders = "host:" + String(S3_HOST) + "\n"
                     + "x-amz-content-sha256:" + payloadHash + "\n"
                     + "x-amz-date:" + dateTime + "\n";
    signedHeaders = "host;x-amz-content-sha256;x-amz-date";
  }

  String canonicalRequest = method + "\n"
    + "/" + objectKey + "\n"
    + "\n"
    + canonicalHeaders + "\n"
    + signedHeaders + "\n"
    + payloadHash;

  String credentialScope = dateStamp + "/" + AWS_REGION + "/s3/aws4_request";
  String stringToSign = "AWS4-HMAC-SHA256\n"
    + dateTime + "\n"
    + credentialScope + "\n"
    + sha256Hex(canonicalRequest);

  uint8_t signingKey[32];
  getSigningKey(dateStamp, signingKey);
  uint8_t signatureBytes[32];
  hmacSHA256(signingKey, 32,
             (uint8_t*)stringToSign.c_str(), stringToSign.length(),
             signatureBytes);

  return "AWS4-HMAC-SHA256 Credential=" + String(AWS_ACCESS_KEY_ID)
    + "/" + credentialScope
    + ", SignedHeaders=" + signedHeaders
    + ", Signature=" + toHexStr(signatureBytes, 32);
}

void getAwsDateTime(String& dateStamp, String& dateTime) {
  String dt = getRTCDateTime();  // "2026-03-09 12:00:00" (JST)

  // JSTをUTCに変換（-9時間）
  int yy  = dt.substring(0, 4).toInt();
  int mo  = dt.substring(5, 7).toInt();
  int dd  = dt.substring(8, 10).toInt();
  int hh  = dt.substring(11, 13).toInt();
  int mi  = dt.substring(14, 16).toInt();
  int ss  = dt.substring(17, 19).toInt();

  hh -= 9;
  if (hh < 0) {
    hh += 24;
    dd -= 1;
    if (dd < 1) {
      mo -= 1;
      if (mo < 1) { mo = 12; yy -= 1; }
      // 月末日テーブル
      int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
      if (mo == 2 && (yy % 4 == 0)) daysInMonth[1] = 29;
      dd = daysInMonth[mo - 1];
    }
  }

  char ds[9], dts[17];
  sprintf(ds,  "%04d%02d%02d",          yy, mo, dd);
  sprintf(dts, "%04d%02d%02dT%02d%02d%02dZ", yy, mo, dd, hh, mi, ss);
  dateStamp = String(ds);
  dateTime  = String(dts);
}

bool uploadTripsToS3() {
  if (WiFi.status() != WL_CONNECTED) return false;

  // --- JSON 読み込み ---
  File f = SPIFFS.open("/trips.json", "r");
  if (!f) {
    Serial.println("[S3] trips.json open failed");
    return false;
  }
  String payload = f.readString();
  f.close();

  if (payload.length() == 0) {
    Serial.println("[S3] trips.json empty");
    return false;
  }

  // --- AWS 署名用データ ---
  String dateStamp, dateTime;
  getAwsDateTime(dateStamp, dateTime);

  String payloadHash = sha256Hex(payload);
  String contentType = "application/json";
  String objectKey   = "trips.json";

  String authHeader = buildS3AuthHeader(
      "PUT",
      objectKey,
      payloadHash,
      dateTime,
      dateStamp,
      contentType
  );

  // --- Secure クライアント作成 ---
  WiFiClientSecure client;
  client.setInsecure();   // 初期テスト用。本番は CA 設定推奨

  // --- HTTPClient 開始 ---
  HTTPClient http;
  String url = "https://" + String(S3_HOST) + "/" + objectKey;

  if (!http.begin(client, url)) {
    Serial.println("[S3] http.begin failed");
    return false;
  }

  http.setTimeout(10000);

  // --- 必要ヘッダー ---
  http.addHeader("Host", S3_HOST);
  http.addHeader("Content-Type", contentType);
  http.addHeader("x-amz-content-sha256", payloadHash);
  http.addHeader("x-amz-date", dateTime);
  http.addHeader("Authorization", authHeader);

  // --- PUT 実行 ---
  int code = http.PUT(payload);
  http.end();

  if (code == 200 || code == 204) {
    Serial.printf("[S3] PUT trips.json OK (%d)\n", code);
    return true;
  }

  Serial.printf("[S3] PUT error: %d\n", code);
  return false;
}

bool downloadDriversFromS3() {
  if (WiFi.status() != WL_CONNECTED) return false;

  // --- AWS 署名用データ ---
  String dateStamp, dateTime;
  getAwsDateTime(dateStamp, dateTime);

  // GET は空ボディ → SHA256 は固定値
  const String emptyHash =
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

  String objectKey = "drivers.json";

  String authHeader = buildS3AuthHeader(
      "GET",
      objectKey,
      emptyHash,
      dateTime,
      dateStamp,
      ""   // GET は Content-Type なし
  );

  // --- Secure クライアント ---
  WiFiClientSecure client;
  client.setInsecure();  // 初期テスト用。本番は CA 設定推奨

  // --- HTTPClient 開始 ---
  HTTPClient http;
  String url = "https://" + String(S3_HOST) + "/" + objectKey;

  if (!http.begin(client, url)) {
    Serial.println("[S3] http.begin failed");
    return false;
  }

  http.setTimeout(10000);

  // --- 必要ヘッダー ---
  http.addHeader("Host", S3_HOST);
  http.addHeader("x-amz-content-sha256", emptyHash);
  http.addHeader("x-amz-date", dateTime);
  http.addHeader("Authorization", authHeader);

  // --- GET 実行 ---
  int code = http.GET();

  if (code == 200) {
    String body = http.getString();
    http.end();

    File f = SPIFFS.open("/drivers.json", "w");
    if (!f) {
      Serial.println("[S3] drivers.json write failed");
      return false;
    }

    f.print(body);
    f.close();

    Serial.println("[S3] GET drivers.json OK");
    return true;
  }

  String errBody = http.getString();  // エラー内容取得
  Serial.printf("[S3] GET error: %d\n", code);
  Serial.println("[S3] error body: " + errBody);  // XML形式でエラー詳細が出る
  http.end();
  return false;
}

void syncWithS3() {
  Serial.printf("[S3] WiFi status=%d\n", WiFi.status());
    // WiFi未接続なら最大10秒待機
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[S3] WiFi待機中...");
    startWiFiConnect();
    unsigned long wt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wt < 10000) {
      delay(200);
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[S3] WiFi接続失敗、sync中止");
      return;
    }
  }
    // シークレットキーの長さと先頭4文字だけ確認（セキュリティ上これだけでOK）
  String secret = String(AWS_SECRET_ACCESS_KEY);
  Serial.printf("[S3] secret len=%d, first4=%s\n", 
    secret.length(), secret.substring(0, 4).c_str());
  Serial.printf("[S3] access key=%s\n", AWS_ACCESS_KEY_ID);
  Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("[S3] sync start...");
  String dateStamp, dateTime;
  getAwsDateTime(dateStamp, dateTime);
  Serial.printf("[S3] dateStamp=%s\n", dateStamp.c_str());
  Serial.printf("[S3] dateTime=%s\n", dateTime.c_str());

  Serial.printf("[MEM] Before upload: %d bytes\n", ESP.getFreeHeap());
  bool up   = uploadTripsToS3();
  Serial.printf("[MEM] After upload: %d bytes\n", ESP.getFreeHeap());

  bool down = downloadDriversFromS3();
  Serial.printf("[S3] sync done: upload=%s / download=%s\n",
    up ? "OK" : "NG", down ? "OK" : "NG");
}

// ───────────────────────────────
// WiFi設定をSPIFFSから読み込む
// なければ secrets.h の値をそのまま使用
// ───────────────────────────────
void loadWiFiConfig() {
  if (!SPIFFS.exists("/wifi_config.json")) return;
  File f = SPIFFS.open("/wifi_config.json", "r");
  if (!f) return;
  JsonDocument doc;
  if (deserializeJson(doc, f) == DeserializationError::Ok) {
    if (doc["ssid"].is<const char*>()) wifiSSID = doc["ssid"].as<String>();
    if (doc["pass"].is<const char*>()) wifiPass = doc["pass"].as<String>();
  }
  f.close();
}

// ───────────────────────────────
// APモード時のM5Dial画面表示
// ───────────────────────────────
void drawAPModePage() {
  M5Dial.Display.fillScreen(BLACK);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextColor(CYAN);
  M5Dial.Display.setFont(&fonts::Font4);
  M5Dial.Display.drawString("WiFi Setup", 120, 55);

  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.setFont(&fonts::Font2);
  M5Dial.Display.drawString("SSID:", 120, 90);
  M5Dial.Display.setTextColor(YELLOW);
  M5Dial.Display.drawString(AP_SSID, 120, 108);

  M5Dial.Display.setTextColor(WHITE);
  M5Dial.Display.drawString("URL:", 120, 130);
  M5Dial.Display.setTextColor(YELLOW);
  M5Dial.Display.drawString("192.168.4.1", 120, 148);

  M5Dial.Display.setTextColor(TFT_DARKGREY);
  M5Dial.Display.drawString("press to cancel", 120, 185);
}

// ───────────────────────────────
// APモード用HTMLの生成
// ───────────────────────────────
String buildAPHTML(bool saved = false) {
  if (saved) {
    return R"rawhtml(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>body{background:#181c22;color:#e2e8f0;font-family:sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0;}
.card{text-align:center;padding:40px 24px;}.icon{width:56px;height:56px;background:#14532d;border-radius:50%;display:flex;align-items:center;justify-content:center;margin:0 auto 16px;font-size:24px;color:#22c55e;}
h2{color:#22c55e;font-size:18px;margin-bottom:8px;}p{color:#64748b;font-size:13px;line-height:1.7;}</style></head>
<body><div class="card"><div class="icon">&#10003;</div>
<h2>保存しました</h2><p>設定を保存しました。<br>デバイスが再起動します...<br><br>再起動後はこのAPに<br>接続できなくなります。</p></div></body></html>)rawhtml";
  }

  String currentSSID = "";
  if (SPIFFS.exists("/wifi_config.json")) {
    File f = SPIFFS.open("/wifi_config.json", "r");
    JsonDocument doc;
    if (f && deserializeJson(doc, f) == DeserializationError::Ok) {
      currentSSID = doc["ssid"].as<String>();
    }
    f.close();
  }

  String html = R"rawhtml(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
*{box-sizing:border-box;}body{background:#181c22;color:#e2e8f0;font-family:sans-serif;margin:0;}
.header{background:#1e2530;padding:16px 20px;border-bottom:1px solid #2a3545;}
.logo{display:flex;align-items:center;gap:10px;margin-bottom:4px;}
.logo-icon{width:28px;height:28px;background:#2563eb;border-radius:6px;display:flex;align-items:center;justify-content:center;font-size:14px;}
.logo-text{font-size:15px;font-weight:500;}.logo-sub{font-size:11px;color:#64748b;}
.body{padding:20px;}
.info-box{background:#1e2a3a;border:1px solid #2a3f5a;border-radius:10px;padding:12px 14px;margin-bottom:16px;font-size:12px;color:#94a3b8;line-height:1.6;}
.current{background:#1a2535;border:1px solid #243040;border-radius:8px;padding:10px 12px;margin-bottom:16px;display:flex;justify-content:space-between;align-items:center;}
.current-label{font-size:11px;color:#64748b;}.current-value{font-size:13px;color:#38bdf8;font-family:monospace;}
label{font-size:11px;color:#64748b;display:block;margin-bottom:6px;text-transform:uppercase;letter-spacing:.05em;}
input{width:100%;background:#1e2530;border:1px solid #2a3545;border-radius:8px;padding:10px 12px;font-size:14px;color:#e2e8f0;outline:none;margin-bottom:14px;}
input:focus{border-color:#2563eb;}
button{width:100%;background:#2563eb;color:#fff;border:none;border-radius:8px;padding:12px;font-size:14px;font-weight:500;cursor:pointer;margin-top:4px;}
</style></head><body>
<div class="header"><div class="logo"><div class="logo-icon">&#9729;</div><span class="logo-text">Drive Log Cloud</span></div><div class="logo-sub">WiFi&#35373;&#23450;</div></div>
<div class="body">
<div class="info-box">&#25509;&#32154;&#12377;&#12427;WiFi&#12398;SSID&#12392;&#12497;&#12473;&#12527;&#12540;&#12489;&#12434;&#20837;&#21147;&#12375;&#12390;&#12367;&#12384;&#12373;&#12356;&#12290;&#20445;&#23384;&#24460;&#12289;&#12487;&#12496;&#12452;&#12473;&#12364;&#33258;&#21160;&#30340;&#12395;&#36215;&#21205;&#12375;&#12414;&#12377;&#12290;</div>)rawhtml";

  if (currentSSID.length() > 0) {
    html += "<div class='current'><div><div class='current-label'>&#29694;&#22312;&#12398;&#35373;&#23450;</div>";
    html += "<div class='current-value'>" + currentSSID + "</div></div>";
    html += "<span style='color:#22c55e;'>&#10003;</span></div>";
  }

  html += R"rawhtml(
<form method="POST" action="/save">
<label>SSID&#65288;&#12493;&#12483;&#12488;&#12527;&#12540;&#12463;&#21517;&#65289;</label>
<input type="text" name="ssid" placeholder="&#20363;: MyWiFiNetwork" required>
<label>&#12497;&#12473;&#12527;&#12540;&#12489;</label>
<input type="password" name="pass" placeholder="WiFi&#12497;&#12473;&#12527;&#12540;&#12489;&#12434;&#20837;&#21147;">
<button type="submit">&#20445;&#23384;&#12375;&#12390;&#20877;&#36215;&#21205;</button>
</form></div></body></html>)rawhtml";

  return html;
}

// ───────────────────────────────
// APモード 開始
// ───────────────────────────────
void startAPMode() {
  apModeActive = true;
  apModeStartTime = millis();
  wifiConnecting = false;

  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  WiFi.disconnect(false);
  delay(300);
  WiFi.mode(WIFI_AP);  // AP_STA → AP に戻す
  delay(500);

  bool result = WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("[AP] softAP result: %s\n", result ? "OK" : "FAILED");
  if (!result) {
    apModeActive = false;
    return;
  }
  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[AP] IP: %d.%d.%d.%d\n", apIP[0], apIP[1], apIP[2], apIP[3]);
  apTcpServer.stop(); 
  apTcpServer.begin();
  delay(500);
  Serial.println("[AP] TCP server started");
  drawAPModePage();
}
// ───────────────────────────────
// APモード 停止
// ───────────────────────────────
void stopAPMode() {
  apTcpServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);   // ← 追加：スタックを完全にリセット
  delay(200);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  apModeActive = false;
}

uint32_t getOBD2Odometer() {
  Serial.println("[OBD2] WiFi切り替え開始");

  // 現在のWiFiから切断
  WiFi.disconnect(true);
  delay(500);

  // OBD2アダプタのAPに接続
  WiFi.begin(OBD2_SSID, OBD2_PASS);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
    delay(200);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OBD2] AP接続失敗");
    startWiFiConnect();
    return 0;
  }
  Serial.println("[OBD2] AP接続OK");

  // ELM327にTCP接続
  WiFiClient obd;
  if (!obd.connect(OBD2_IP, OBD2_PORT)) {
    Serial.println("[OBD2] TCP接続失敗");
    WiFi.disconnect(true);
    startWiFiConnect();
    return 0;
  }

  // ELM327初期化
  obd.print("ATZ\r");
  delay(1000);
  obd.readString();  // 応答を読み捨て

  obd.print("ATE0\r");  // エコーオフ
  delay(300);
  obd.readString();

  obd.print("ATSP6\r");  // ISO 15765-4 CAN 11bit 500kbaud（国産車の大多数）
  delay(500);
  obd.readString();

  // PID 01A6（総走行距離）クエリ
  //obd.print("01A6\r");
  obd.print("010D\r");
  delay(1000);
  String resp = "";
  unsigned long rt = millis();
  while (millis() - rt < 3000) {
    if (obd.available()) resp += (char)obd.read();
    if (resp.indexOf('>') >= 0) break;
    delay(10);
  }
  Serial.println("[OBD2] 応答: " + resp);
  obd.stop();

  // 元のWiFiに戻す
  WiFi.disconnect(true);
  delay(500);
  startWiFiConnect();

  // 応答パース: "41 A6 XX XX XX XX"
  resp.trim();
  int idx = resp.indexOf("41 A6");
  if (idx < 0) {
    Serial.println("[OBD2] パース失敗");
    return 0;
  }
  String hex = resp.substring(idx + 6);
  hex.trim();
  hex.replace(" ", "");
  if (hex.length() < 8) {
    Serial.println("[OBD2] データ不足");
    return 0;
  }
  uint32_t km = strtoul(hex.substring(0, 8).c_str(), nullptr, 16);
  Serial.printf("[OBD2] オドメーター: %u km\n", km);
  return km;
}

void startManualDistInput() {
  manualDistKm = lastOdometer;  // ← tripDistAccum から変更
  currentScreen = SCREEN_MANUAL_DIST;
  needsRedraw = true;
  M5Dial.Encoder.write(0);
  lastEncoderPos = 0;
}

void drawManualDistInput() {
  M5Dial.Display.setFont(&fonts::lgfxJapanGothicP_16);
  M5Dial.Display.fillScreen(0x181c22);

  M5Dial.Display.setTextColor(0x38bdf8);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextSize(1.2);
  M5Dial.Display.drawString("終了時メーター値を入力", 120, 60);

  M5Dial.Display.setTextColor(0xe2e8f0);
  M5Dial.Display.setTextSize(0.8);
  M5Dial.Display.drawString("(GPS参考値)", 120, 90);

  M5Dial.Display.setTextColor(0xffffff);
  M5Dial.Display.setTextSize(2.5);
  M5Dial.Display.drawString(String(manualDistKm) + " km", 120, 140);

  M5Dial.Display.setTextColor(0x94a3b8);
  M5Dial.Display.setTextSize(0.8);
  M5Dial.Display.drawString("ダイヤル: ±1km", 120, 185);
  M5Dial.Display.drawString("ボタン: 確定", 120, 205);
}

void saveBaseConfig() {
  JsonDocument doc;
  doc["lat"] = baseLat;
  doc["lng"] = baseLng;
  File f = SPIFFS.open("/base_config.json", FILE_WRITE);
  if (f) {
    serializeJson(doc, f);
    f.close();
    Serial.printf("[Base] saved: %.6f, %.6f\n", baseLat, baseLng);
  }
}

void loadBaseConfig() {
  if (!SPIFFS.exists("/base_config.json")) return;
  File f = SPIFFS.open("/base_config.json", FILE_READ);
  if (!f) return;
  JsonDocument doc;
  if (deserializeJson(doc, f) == DeserializationError::Ok) {
    baseLat = doc["lat"] | baseLat;
    baseLng = doc["lng"] | baseLng;
    Serial.printf("[Base] loaded: %.6f, %.6f\n", baseLat, baseLng);
  }
  f.close();
}


// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  WiFi.persistent(false);  // ← 追加：NVSへのWiFi認証情報キャッシュを無効化
  //while (!Serial) delay(10);  // USB CDC接続待ち（これが重要）
  Serial.println("=== BOOT ===");

  M5Dial.begin(true, true);  // enableEncoder=true, enableRFID=true
  gpsSerial.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);

  // SPIFFS初期化
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  }
  //SPIFFS.remove("/trips.json");
  initDriversJson();  // ← 追加

  // WiFi設定読込
  loadWiFiConfig();
  // 確認用：wifi_config.json の存在とSSIDをログ出力
  if (SPIFFS.exists("/wifi_config.json")) {
    Serial.println("[WiFi] config file found: " + wifiSSID);
  } else {
    Serial.println("[WiFi] config file not found, using secrets.h");
  }
  // WiFi接続開始
  startWiFiConnect();

  M5Dial.Display.setRotation(0);
  M5Dial.Display.setBrightness(80);
  M5Dial.Speaker.setVolume(255);  // 0〜255（デフォルトは低め）

  // ========================================
  // ブートスクリーン
  // ========================================
  drawBootScreen();
  delay(500);
  
  // 各モジュール初期化（後で実際の初期化処理に置き換え）
  delay(400);
  setModuleReady(MOD_GPS);
  delay(400);
  setModuleReady(MOD_OBD);
  delay(400);
  // NFC初期化（実際のハードウェア初期化）
  nfcReady = initNFC();
  if (nfcReady) {
    setModuleReady(MOD_NFC);
  }
  delay(400);
  setModuleReady(MOD_WIFI);
  
  // メーター起動演出
  delay(300);
  drawGaugeActive();
  
  // 3秒待機（ブート画面表示時間確保）
  delay(700);  // 上記の処理時間を合わせて約3秒
  
  // メイン画面へ遷移
  appState = STATE_MAIN;
  needsRedraw = true;

  getOBD2Odometer();

  tripCooldownUntil = millis() + 30000;

  // 運行中ステータスの復元（電源断またぎ対応）
  if (loadTripState()) {
    Serial.println("[TripState] trip resumed from previous session");
  }
}

// ============================================================
// Main Loop
// ============================================================
void loop() {
  M5Dial.update();
  // trip_state.json 定期保存（30秒間隔）
  if (tripStartTime != 0 &&
      millis() - lastTripStateSave > TRIP_STATE_SAVE_INTERVAL) {
    saveTripState();
    lastTripStateSave = millis();
  }
  if (apModeActive) {
    static unsigned long lastApLog = 0;
    if (millis() - lastApLog > 3000) {
      Serial.printf("[AP] loop running, stations=%d\n", WiFi.softAPgetStationNum());
      lastApLog = millis();
    }

    for (int retry = 0; retry < 5; retry++) {
      WiFiClient client = apTcpServer.available();
      if (!client) { delay(10); continue; }

      Serial.println("[AP] client connected");
      Serial.printf("[AP] free heap: %d\n", ESP.getFreeHeap());

      char reqBuf[1024] = {0};
      int reqLen = 0;
      unsigned long t = millis();
      while (client.connected() && millis() - t < 5000) {
        if (client.available()) {
          char c = client.read();
          if (reqLen < 1023) reqBuf[reqLen++] = c;
          if (reqLen >= 4 &&
              reqBuf[reqLen-4] == '\r' && reqBuf[reqLen-3] == '\n' &&
              reqBuf[reqLen-2] == '\r' && reqBuf[reqLen-1] == '\n') break;
        } else { delay(5); }
      }
      String req = String(reqBuf);
      Serial.println("[AP] request: " + req.substring(0, 60));

      if (req.startsWith("POST /save")) {
        int clIdx = req.indexOf("Content-Length: ");
        int contentLength = 0;
        if (clIdx >= 0) contentLength = req.substring(clIdx + 16).toInt();
        Serial.printf("[AP] Content-Length: %d\n", contentLength);
        delay(200);
        String body = "";
        unsigned long bt = millis();
        while ((int)body.length() < contentLength && millis() - bt < 3000) {
          if (client.available()) body += (char)client.read();
          else delay(5);
        }
        Serial.println("[AP] body: " + body);

        auto urlDecode = [](String s) {
          String r = "";
          for (int i = 0; i < (int)s.length(); i++) {
            if (s[i] == '+') { r += ' '; }
            else if (s[i] == '%' && i + 2 < (int)s.length()) {
              char c = (char)strtol(s.substring(i+1, i+3).c_str(), nullptr, 16);
              r += c; i += 2;
            } else { r += s[i]; }
          }
          return r;
        };
        auto parseParam = [](String body, String key) {
          int i = body.indexOf(key + "=");
          if (i < 0) return String("");
          int j = body.indexOf("&", i);
          return (j < 0) ? body.substring(i + key.length() + 1)
                         : body.substring(i + key.length() + 1, j);
        };

        String ssid = urlDecode(parseParam(body, "ssid"));
        String pass = urlDecode(parseParam(body, "pass"));
        Serial.println("[AP] ssid=" + ssid + " pass=" + pass);

        if (ssid.length() > 0) {
          File f = SPIFFS.open("/wifi_config.json", "w");
          if (f) {
            JsonDocument doc;
            doc["ssid"] = ssid;
            doc["pass"] = pass;
            serializeJson(doc, f);
            f.close();
            Serial.println("[AP] wifi_config.json saved");
            const char* html =
              "<html><head><meta charset='utf-8'></head>"
              "<body style='background:#181c22;color:#e2e8f0;font-family:sans-serif;padding:40px;text-align:center'>"
              "<h2 style='color:#22c55e'>&#20445;&#23384;&#12375;&#12414;&#12375;&#12383;</h2>"
              "<p>&#20877;&#36215;&#21205;&#12375;&#12414;&#12377;...</p></body></html>";
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();
            client.print(html);
            client.flush();
            delay(100);
            client.stop();
            delay(2000);
            ESP.restart();
          } else {
            Serial.println("[AP] SPIFFS open failed");
            client.println("HTTP/1.1 500 Internal Server Error");
            client.println("Connection: close");
            client.println();
            client.stop();
          }
        } else {
          Serial.println("[AP] ssid empty, redirecting");
          client.println("HTTP/1.1 302 Found");
          client.println("Location: http://192.168.4.1:8080/");
          client.println("Connection: close");
          client.println();
          client.stop();
        }
      } else {
        // 空リクエストは無視
        if (req.length() < 4) {
          client.stop();
          break;
        }
        const char* body =
          "<html><head><meta charset='utf-8'>"
          "<meta name='viewport' content='width=device-width,initial-scale=1'>"
          "</head><body style='background:#181c22;color:#e2e8f0;font-family:sans-serif;padding:20px'>"
          "<h2 style='color:#38bdf8'>Drive Log Cloud - WiFi&#35373;&#23450;</h2>"
          "<form method='POST' action='/save'>"
          "<p>SSID<br><input name='ssid' style='width:100%;padding:8px;margin-bottom:12px'></p>"
          "<p>&#12497;&#12473;&#12527;&#12540;&#12489;<br><input name='pass' type='password' style='width:100%;padding:8px;margin-bottom:12px'></p>"
          "<input type='submit' value='&#20445;&#23384;&#12375;&#12390;&#20877;&#36215;&#21205;' "
          "style='width:100%;padding:12px;background:#2563eb;color:#fff;border:none;border-radius:8px'>"
          "</form></body></html>";
        Serial.printf("[AP] sending %d bytes\n", (int)strlen(body));
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html; charset=utf-8");
        client.println("Connection: close");
        client.println();
        client.print(body);
        client.flush();
        delay(100);
        client.stop();
        Serial.println("[AP] response sent");
      }
      break;
    }

    if (millis() - apModeStartTime > 3000 && M5Dial.BtnA.wasPressed()) {
      stopAPMode();
      startWiFiConnect();
      currentScreen = SCREEN_CONFIG;
      drawConfigPage();
    }
    return;
  }
    
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  syncRTCFromGPS();

  updateWiFiStatus();
  updateAddress();

  updateFarthestPoint();

  if (appState == STATE_MAIN) {
    pollNFC();
    handleInput();
    static unsigned long lastGPSRedraw = 0;
    if (currentScreen == SCREEN_GPS && millis() - lastGPSRedraw > 1000) {
      drawGPSInfo();
      lastGPSRedraw = millis();
    }
    if (needsRedraw) {
      switch (currentScreen) {
        case SCREEN_MENU:   drawMainScreen();  break;
        case SCREEN_GPS:    drawGPSInfo();     break;
        case SCREEN_OBD2:   drawOBD2Page();    break;
        case SCREEN_DRIVER: drawDriverPage();  break;
        case SCREEN_CONFIG: drawConfigPage();  break;
        case SCREEN_MANUAL_DIST: drawManualDistInput(); break;
      }
      needsRedraw = false;
    }
  }
}