// =============================================================
// secrets.example.h
// このファイルをコピーして secrets.h を作成し、値を設定してください。
// secrets.h は .gitignore により Git 管理対象外です。
// =============================================================

#ifndef SECRETS_H
#define SECRETS_H

// AWS IAM アクセスキー（IAMコンソールのCSVからコピー）
#define AWS_ACCESS_KEY_ID     "YOUR_ACCESS_KEY_ID_HERE"
#define AWS_SECRET_ACCESS_KEY "YOUR_SECRET_ACCESS_KEY_HERE"

// WiFi 接続情報
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// 会社（基準点）の座標
// Googleマップで会社の建物を右クリック → 表示された座標をコピーして貼り付ける
// 例: 43.804443, 143.892911 → 前が緯度(LAT)、後ろが経度(LNG)
#define BASE_LAT 0.000000  // 会社の緯度
#define BASE_LNG 0.000000  // 会社の経度

#endif // SECRETS_H