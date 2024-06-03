#include <Arduino.h>
#include <Wire.h>
#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN (32)
#endif
#ifndef MAX_PASSPHRASE_LEN
#define MAX_PASSPHRASE_LEN (64)
#endif

#endif
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "eepromHelper.h"
#include "types.h"
#include "lightControl.h"

#include "display.h"

/// Your function Section
#ifndef RELAY_IO_PIN
#define RELAY_IO_PIN (26)
#endif
#define RELAY_LOW_ACTIVE (1)
#define RELAY_CLOSE_DEFAULT (1)

#define EEPROM_WIFI_SSID_ADDR (EEPROM_USEABLE_ADDR)
#define EEPROM_WIFI_PASSWORD_ADDR (EEPROM_WIFI_SSID_ADDR + MAX_SSID_LEN)
#define EEPROM_SETTING_ADDR (EEPROM_WIFI_PASSWORD_ADDR + MAX_PASSPHRASE_LEN)
#define TOTAL_EEPROM_WRITE_BYTES (EEPROM_SETTING_ADDR + sizeof(light_control_data))

char wifiSSID[MAX_SSID_LEN + 1] = "WIFISSID";
char wifiPassword[MAX_PASSPHRASE_LEN + 1] = "WIFIPWD";

LightControl lightControl;

char settingWifiSSID[MAX_SSID_LEN + 1];
char settingWifiPassword[MAX_PASSPHRASE_LEN + 1];
light_control_data settingLightControlData;
setting_target currentSettingTarget;

bool lightState = true;
bool controlOverride = false;

#define NTP_TIME_OFFSET (60 * 60 * 7)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", NTP_TIME_OFFSET);
ntp_connect_status ntpStatus = NTP_DISCONNECTED;
time_t currentTime = 1704067200;
tm timeStruct;

#define SERIAL_DEBUG_DELAY (1000)
uint32_t serialDebugTs;

static void compute_time(time_t time);

static void print_date_time(const time_t &dt);
static void print_settings();

static void set_relay_signal_to(bool state);

static void save_to_eeprom();

extern "C"
{
  void backup_new_settings();
  void apply_new_settings();

  void connect_wifi();
  void update_relay_state_to(bool state);
}

void setup()
{
#if ESP32
  Serial.begin(115200);
#else
  Serial.begin(74880);
#endif

  EEPROM.begin(TOTAL_EEPROM_WRITE_BYTES);
  if (is_eeprom_valid(TOTAL_EEPROM_WRITE_BYTES))
  {
    Serial.println(F("EEPROM Valid: Load from EEPROM"));
    read_eeprom_raw(EEPROM_WIFI_SSID_ADDR, wifiSSID, sizeof(wifiSSID));
    read_eeprom_raw(EEPROM_WIFI_PASSWORD_ADDR, wifiPassword, sizeof(wifiPassword));
    read_eeprom_raw(EEPROM_SETTING_ADDR, &lightControl.setting, sizeof(light_control_data));

    print_settings();
  }
  else
  {
    Serial.println(F("EEPROM Invalid"));
    Serial.print(F("Storage:"));
    Serial.println(read_eeprom_uint64(EEPROM_CRC_ADDR), 16);
    Serial.print(F("Actual:"));
    Serial.println(eeprom_crc(TOTAL_EEPROM_WRITE_BYTES), 16);
    save_to_eeprom();
  }

  pinMode(RELAY_IO_PIN, OUTPUT);
  lightControl.apply_check_at(currentTime);
  lightState = lightControl.response.currentLightState;
  set_relay_signal_to(lightState);

  connect_wifi();

  init_screen({
      .lightState = lightState,
  });

  serialDebugTs = millis();
}
void loop()
{
  wl_status_t wifiStatus = WiFi.status();
  if (wifiStatus == WL_CONNECTION_LOST || wifiStatus == WL_CONNECT_FAILED)
  {
    WiFi.begin(wifiSSID, wifiPassword);
  }

  if (wifiStatus == WL_CONNECTED && ntpStatus == NTP_DISCONNECTED)
  {
    timeClient.begin();
    ntpStatus = NTP_CONNECTED;
  }

  if (ntpStatus == NTP_CONNECTED)
  {
    if (timeClient.update())
    {
      Serial.println(F("Sync NTP to RTC time"));
    }
  }

  compute_time(timeClient.getEpochTime());

  if (!controlOverride)
  {
    update_relay_state_to(lightControl.response.currentLightState);
  }

  tick_ui_loop();

  update_ui_home_screen({
      .lightState = lightState,
      .controlOverride = controlOverride,
      .ntpStatus = ntpStatus,
      .currentTime = currentTime,
      .nextAutoTime = lightControl.response.nextAutoTime,
      .nextLightState = lightControl.response.nextLightState,
  });
  update_ui_setting_screen({
      .wifiSSID = wifiSSID,
      .wifiPassword = wifiPassword,
      .settingWifiSSID = settingWifiSSID,
      .settingWifiPassword = settingWifiPassword,
      .lightControlData = lightControl.setting,
      .settingLightControlData = settingLightControlData,
  });
  update_ui_input_screen();

  if (millis() - serialDebugTs >= SERIAL_DEBUG_DELAY)
  {
    serialDebugTs = millis();

    Serial.print("WIFI status:");
    Serial.println(WiFi.status());
    Serial.print(F("currentTime time:"));
    print_date_time(currentTime);
    Serial.println();

#if LVGL_GUI
    Serial.print(F("screenLightTs:"));
    Serial.println(millis() - screenLightTs);
#endif
    Serial.print(F("Light Level:"));
    switch (screenLightLevel)
    {
    case SCREEN_LIGHT_DIM:
      Serial.println(F("DIM"));
      break;
    case SCREEN_LIGHT_OFF:
      Serial.println(F("OFF"));
      break;
    case SCREEN_LIGHT_BRIGHT:
      Serial.println(F("BRIGHT"));
      break;
    default:
      Serial.println(F("?"));
      break;
    }
  }

#if LVGL_GUI
  lv_task_handler();
#endif

  delay(1);
}

static void compute_time(time_t time)
{
  if (currentTime == time)
  {
    return;
  }

  currentTime = time;
  lightControl.apply_check_at(currentTime);
}

static void print_settings()
{
  Serial.print(F("WIFI SSID: "));
  Serial.println(wifiSSID);
  Serial.print(F("WIFI PW: "));
  Serial.println(wifiPassword);
  Serial.print(F("GPS LAT: "));
  Serial.println(lightControl.setting.gpsLatitude, 4);
  Serial.print(F("GPS LNG: "));
  Serial.println(lightControl.setting.gpsLongitude, 4);
  Serial.print(F("TZ: "));
  Serial.println(lightControl.setting.timezoneOffset);
  Serial.print(F("RELAY OFF DELAY (MIN): "));
  Serial.println(lightControl.setting.relayOffDelayMin, 4);
  Serial.print(F("RELAY ON DELAY (MIN): "));
  Serial.println(lightControl.setting.relayOnDelayMin, 4);
}
static void print_date_time(const time_t &dt)
{
  char datestring[26];
  tm timeStruct;
  localtime_r(&dt, &timeStruct);
  snprintf_P(datestring,
             26,
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             timeStruct.tm_mon + 1,
             timeStruct.tm_mday,
             timeStruct.tm_year + 1900,
             timeStruct.tm_hour,
             timeStruct.tm_min,
             timeStruct.tm_sec);
  Serial.print(datestring);
}

static void set_relay_signal_to(bool state)
{
  uint8_t on_state = RELAY_LOW_ACTIVE ? LOW : HIGH;
  uint8_t off_state = RELAY_LOW_ACTIVE ? HIGH : LOW;

  if (RELAY_CLOSE_DEFAULT) // flip state
  {
    uint8_t temp = on_state;
    on_state = off_state;
    off_state = temp;
  }

  uint8_t actual_state = state ? on_state : off_state;
  digitalWrite(RELAY_IO_PIN, actual_state);

  Serial.print(F("RELAY: "));
  Serial.println(actual_state == HIGH ? F("HIGH") : F("LOW"));
}

static void save_to_eeprom()
{
  write_eeprom_raw(EEPROM_WIFI_SSID_ADDR, wifiSSID, sizeof(wifiSSID));
  write_eeprom_raw(EEPROM_WIFI_PASSWORD_ADDR, wifiPassword, sizeof(wifiPassword));
  write_eeprom_raw(EEPROM_SETTING_ADDR, &lightControl.setting, sizeof(lightControl.setting));
  commit_eeprom_with_headers(TOTAL_EEPROM_WRITE_BYTES);

  Serial.print(F("EEPROM Saved | CRC: 0x"));
  Serial.println(read_eeprom_uint64(EEPROM_CRC_ADDR), 16);

  print_settings();
}

void backup_new_settings()
{
  strlcpy(settingWifiSSID, wifiSSID, MAX_SSID_LEN + 1);
  strlcpy(settingWifiPassword, wifiPassword, MAX_PASSPHRASE_LEN + 1);
  settingLightControlData = lightControl.setting;
}
void apply_new_settings()
{
  strlcpy(wifiSSID, settingWifiSSID, MAX_SSID_LEN + 1);
  strlcpy(wifiPassword, settingWifiPassword, MAX_PASSPHRASE_LEN + 1);
  lightControl.setting = settingLightControlData;

  save_to_eeprom();
  connect_wifi();
}

void connect_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPassword);
}
void update_relay_state_to(bool state)
{
  if (state == lightState)
  {
    return;
  }
  lightState = state;
  set_relay_signal_to(state);
}
