#ifndef _PROJECT_LIGHTBULB_TYPES
#define _PROJECT_LIGHTBULB_TYPES

#include <time.h>

typedef enum
{
  SETTING_TARGET_WIFI_SSID,
  SETTING_TARGET_WIFI_PASSWORD,
  SETTING_TARGET_GPS_LATITUDE,
  SETTING_TARGET_GPS_LONGITUDE,
  SETTING_TARGET_TIMEZONE_OFFSET,
  SETTING_TARGET_RELAY_OFF_MIN_OFFSET,
  SETTING_TARGET_RELAY_ON_MIN_OFFSET,
} setting_target;

typedef enum
{
  NTP_DISCONNECTED,
  NTP_CONNECTING,
  NTP_CONNECTED,
} ntp_connect_status;

typedef enum
{
  SCREEN_LIGHT_BRIGHT,
  SCREEN_LIGHT_DIM,
  SCREEN_LIGHT_OFF,
} screen_light_level;

typedef struct
{
  double gpsLatitude;
  double gpsLongitude;
  int timezoneOffset;
  double relayOffDelayMin;
  double relayOnDelayMin;
} light_control_data;

typedef struct
{
  bool currentLightState;
  time_t nextAutoTime;
  bool nextLightState;
} light_control_response;

#endif