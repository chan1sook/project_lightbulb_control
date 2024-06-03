// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.0
// LVGL version: 8.3.11
// Project name: ProjectLightblub

#include "ui.h"
#include "types.h"

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN (32)
#endif
#ifndef MAX_PASSPHRASE_LEN
#define MAX_PASSPHRASE_LEN (64)
#endif

extern setting_target currentSettingTarget;

extern bool lightState;
extern bool controlOverride;

extern char settingWifiSSID[MAX_SSID_LEN + 1];
extern char settingWifiPassword[MAX_PASSPHRASE_LEN + 1];
extern light_control_data settingLightControlData;

extern void change_screen_to(lv_obj_t *target);
extern void update_ui_manual_switch_state(bool lightState);

extern void backup_new_settings();
extern void apply_new_settings();

extern void update_relay_state_to(bool state);

extern char *dtostrf(double number, signed int width, unsigned int prec, char *s);

static void reset_input_by_target()
{
  char buffer[24];
  switch (currentSettingTarget)
  {
  case SETTING_TARGET_WIFI_SSID:
    lv_textarea_set_text(ui_KeyboardInputScreen_TextArea1, settingWifiSSID);
    break;
  case SETTING_TARGET_WIFI_PASSWORD:
    lv_textarea_set_text(ui_KeyboardInputScreen_TextArea1, settingWifiPassword);
    break;
  case SETTING_TARGET_GPS_LATITUDE:
    dtostrf(settingLightControlData.gpsLatitude, 0, 12, buffer);
    lv_textarea_set_text(ui_KeyboardInputScreen_TextArea1, buffer);
    break;
  case SETTING_TARGET_GPS_LONGITUDE:
    dtostrf(settingLightControlData.gpsLongitude, 0, 12, buffer);
    lv_textarea_set_text(ui_KeyboardInputScreen_TextArea1, buffer);
    break;
  case SETTING_TARGET_RELAY_OFF_MIN_OFFSET:
    dtostrf(settingLightControlData.relayOffDelayMin, 0, 12, buffer);
    lv_textarea_set_text(ui_KeyboardInputScreen_TextArea1, buffer);
    break;
  case SETTING_TARGET_RELAY_ON_MIN_OFFSET:
    dtostrf(settingLightControlData.relayOnDelayMin, 0, 12, buffer);
    lv_textarea_set_text(ui_KeyboardInputScreen_TextArea1, buffer);
    break;
  }
}

void toggleControlOverride(lv_event_t *e)
{
  controlOverride = lv_obj_has_state(ui_HomeScreen_OverrideSwitch, LV_STATE_CHECKED);
  update_ui_manual_switch_state(lightState);
}

void toggleManualControl(lv_event_t *e)
{
  update_relay_state_to(lv_obj_has_state(ui_HomeScreen_LightbulbManualSwitch, LV_STATE_CHECKED));
}

void toOptionPage(lv_event_t *e)
{
  change_screen_to(ui_SettingScreen);
  backup_new_settings();
}

void cancelNewSetting(lv_event_t *e)
{
  change_screen_to(ui_HomeScreen);
}

void resetNewSettings(lv_event_t *e)
{
  backup_new_settings();
}

void applyNewSettings(lv_event_t *e)
{
  apply_new_settings();
  change_screen_to(ui_HomeScreen);
}

void prepareWifiSSIDToInput(lv_event_t *e)
{
  lv_textarea_set_max_length(ui_KeyboardInputScreen_TextArea1, MAX_SSID_LEN + 1);
  currentSettingTarget = SETTING_TARGET_WIFI_SSID;
  reset_input_by_target();
  change_screen_to(ui_KeyboardInputScreen);
}

void prepareWifiPasswordToInput(lv_event_t *e)
{
  lv_textarea_set_max_length(ui_KeyboardInputScreen_TextArea1, MAX_PASSPHRASE_LEN + 1);
  currentSettingTarget = SETTING_TARGET_WIFI_PASSWORD;
  reset_input_by_target();
  change_screen_to(ui_KeyboardInputScreen);
}

void prepareGpsLatitudeToInput(lv_event_t *e)
{
  lv_textarea_set_max_length(ui_KeyboardInputScreen_TextArea1, 24);
  currentSettingTarget = SETTING_TARGET_GPS_LATITUDE;
  reset_input_by_target();
  change_screen_to(ui_KeyboardInputScreen);
}

void prepareGpsLongitudeToInput(lv_event_t *e)
{
  lv_textarea_set_max_length(ui_KeyboardInputScreen_TextArea1, 24);
  currentSettingTarget = SETTING_TARGET_GPS_LONGITUDE;
  reset_input_by_target();
  change_screen_to(ui_KeyboardInputScreen);
}

void prepareRelayOffDelayToInput(lv_event_t *e)
{
  lv_textarea_set_max_length(ui_KeyboardInputScreen_TextArea1, 24);
  currentSettingTarget = SETTING_TARGET_RELAY_OFF_MIN_OFFSET;
  reset_input_by_target();
  change_screen_to(ui_KeyboardInputScreen);
}

void prepareRelayOnDelayToInput(lv_event_t *e)
{
  lv_textarea_set_max_length(ui_KeyboardInputScreen_TextArea1, 24);
  currentSettingTarget = SETTING_TARGET_RELAY_ON_MIN_OFFSET;
  reset_input_by_target();
  change_screen_to(ui_KeyboardInputScreen);
}

void resetCurrentInput(lv_event_t *e)
{
  reset_input_by_target();
}

void cancelCurrentInput(lv_event_t *e)
{
  change_screen_to(ui_SettingScreen);
}

void applyCurrentInputToSetting(lv_event_t *e)
{
  const char *readText = lv_textarea_get_text(ui_KeyboardInputScreen_TextArea1);

  switch (currentSettingTarget)
  {
  case SETTING_TARGET_WIFI_SSID:
    strlcpy(settingWifiSSID, readText, MAX_SSID_LEN + 1);
    break;
  case SETTING_TARGET_WIFI_PASSWORD:
    strlcpy(settingWifiPassword, readText, MAX_PASSPHRASE_LEN + 1);
    break;
  case SETTING_TARGET_GPS_LATITUDE:
    settingLightControlData.gpsLatitude = atof(readText);
    break;
  case SETTING_TARGET_GPS_LONGITUDE:
    settingLightControlData.gpsLongitude = atof(readText);
    break;
  case SETTING_TARGET_RELAY_OFF_MIN_OFFSET:
    settingLightControlData.relayOffDelayMin = atof(readText);
    break;
  case SETTING_TARGET_RELAY_ON_MIN_OFFSET:
    settingLightControlData.relayOnDelayMin = atof(readText);
    break;
  }

  change_screen_to(ui_SettingScreen);
}