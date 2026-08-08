/**
 * @file    config_manager.cpp
 * @brief   运行时配置管理器 实现 — NVS 持久化
 *
 * 使用 ESP32 Preferences (NVS), 命名空间 "clock"。
 * 用 getBytes/putBytes 读写, 零 String 开销。
 */

#include "config_manager.h"
#include "config.h"
#include <Preferences.h>
#include <string.h>

#define CFG_NS  "clock"
#define KEY_SSID    "w_ssid"
#define KEY_PASS    "w_pass"
#define KEY_APIKEY  "w_apik"

void Config_Load(ConfigManager* cm)
{
    if (!cm) return;
    memset(cm, 0, sizeof(ConfigManager));

    Preferences prefs;
    if (prefs.begin(CFG_NS, true)) {  /* 只读打开 */
        prefs.getBytes(KEY_SSID,   cm->ssid,     CFG_SSID_MAX);
        prefs.getBytes(KEY_PASS,   cm->pass,     CFG_PASS_MAX);
        prefs.getBytes(KEY_APIKEY, cm->api_key,  CFG_APIKEY_MAX);
        prefs.end();
    }

    /* 确保 null 终止 */
    cm->ssid[CFG_SSID_MAX - 1]       = '\0';
    cm->pass[CFG_PASS_MAX - 1]       = '\0';
    cm->api_key[CFG_APIKEY_MAX - 1]  = '\0';

    /* 用 config.h 默认值填充空字段 */
    if (cm->api_key[0] == '\0') {
        strncpy(cm->api_key, OWM_API_KEY, CFG_APIKEY_MAX - 1);
        cm->api_key[CFG_APIKEY_MAX - 1] = '\0';
    }
    /* ssid/pass 为空 → 首次启动, 保持空 */
}

static void prefPutBytes(const char* key, const char* val)
{
    if (!val || val[0] == '\0') return;
    Preferences prefs;
    prefs.begin(CFG_NS, false);
    prefs.putBytes(key, val, strlen(val) + 1);
    prefs.end();
}

void Config_SaveWifi(const ConfigManager* cm)
{
    if (!cm) return;
    prefPutBytes(KEY_SSID, cm->ssid);
    prefPutBytes(KEY_PASS, cm->pass);
}

void Config_SaveApiKey(const ConfigManager* cm)
{
    if (!cm) return;
    prefPutBytes(KEY_APIKEY, cm->api_key);
}

int Config_HasWifi(const ConfigManager* cm)
{
    return cm && cm->ssid[0] != '\0';
}
