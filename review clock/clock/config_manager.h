/**
 * @file    config_manager.h
 * @brief   运行时配置管理器 — NVS 持久化存储
 *
 * WiFi 凭据和 API Key 通过 Preferences (NVS) 存储, 断电不丢失。
 * Config_Load 首次调用时自动以 config.h 中的默认值填充。
 *
 * 依赖: Preferences (ESP32 Arduino 核心自带)
 */

#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H

#include <stdint.h>

#define CFG_SSID_MAX    33
#define CFG_PASS_MAX    65
#define CFG_APIKEY_MAX  41

typedef struct {
    char ssid[CFG_SSID_MAX];
    char pass[CFG_PASS_MAX];
    char api_key[CFG_APIKEY_MAX];
} ConfigManager;

/* 从 NVS 加载, 空字段用 config.h 默认值填充 */
void Config_Load(ConfigManager* cm);

/* 持久化 WiFi 凭据到 NVS */
void Config_SaveWifi(const ConfigManager* cm);

/* 持久化 API Key 到 NVS */
void Config_SaveApiKey(const ConfigManager* cm);

/* 是否有已保存的 WiFi 凭据 */
int  Config_HasWifi(const ConfigManager* cm);

#endif
