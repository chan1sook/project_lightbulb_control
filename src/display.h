#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "types.h"

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN (32)
#endif
#ifndef MAX_PASSPHRASE_LEN
#define MAX_PASSPHRASE_LEN (64)
#endif

#if LVGL_GUI
#if ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <SPI.h>
#include <TFT_eSPI.h>
#include "FT62XXTouchScreen.h"
#include "ui/ui.h"
#else
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "PCF8574.h"
#endif

#if LVGL_GUI
extern TFT_eSPI tft;
extern FT62XXTouchScreen touchScreen;

#define BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT / 10)
extern lv_disp_draw_buf_t dispBuffer;
extern lv_color_t *screenBuffer;

#define TFT_BL_CHANNEL (0)
#define UI_SCREEN_EMIT_DURATION (30000)
#define UI_SCREEN_DIM_DURATION (60000)
extern uint32_t screenLightTs;
#endif

extern screen_light_level screenLightLevel;

#if LVGL_GUI
#define UI_WIFI_BLINK_DURATION (2500)
extern uint32_t wifiBlinkTs;
extern bool isWiFiBlinkOn;
#else
#define LCD_BLOCK_CHR ('\xFF')
extern const char HOLLOW_CHR_DATA[] PROGMEM;
#define HOLLOW_CHR ('\x1')

extern LiquidCrystal_I2C lcd;
extern PCF8574 pcf8574;

extern char screenBuffer[2][17];

typedef struct
{
  bool key1 : 1;
  bool key2 : 1;
  bool key3 : 1;
  bool key4 : 1;
  bool key5 : 1;
  bool key6 : 1;
  bool key7 : 1;
  bool key8 : 1;
  bool key9 : 1;
  bool key0 : 1;
  bool keyLeft : 1;
  bool keyRight : 1;
  bool keyUp : 1;
  bool keyDown : 1;
  bool keyEsc : 1;
  bool keyEnter : 1;
  bool keyF1 : 1;
  bool keyF2 : 1;
  bool keySharp : 1;
  bool keyAsterisk : 1;
} keypad_pressed_state;
extern keypad_pressed_state keypad_state;
extern keypad_pressed_state prev_keypad_state;
extern keypad_pressed_state pushup_keypad_state;
extern keypad_pressed_state pushdown_keypad_state;

typedef struct
{
  bool forceLcdUpdate;
  bool homeShowAlternate : 1;
  bool savedFlag : 1;
  bool inputNumberOnly : 1;
} screen_flags;
extern screen_flags screenFlags;

#define LCD_UPDATE_DELAY (100)
extern uint32_t lcdUpdateTs;

typedef enum
{
  SCREEN_HOME,
  SCREEN_SETTING,
  SCREEN_INPUT,
} screen_page;
extern screen_page currentScreenPage;
extern bool lightState;
extern bool controlOverride;

extern const char LCD_LINE_TEMPLATE[] PROGMEM;
extern const char TIME_TEMPLATE[] PROGMEM;
extern const char SETTING_LIST_WIFI_SSID[] PROGMEM;
extern const char SETTING_LIST_WIFI_PWD[] PROGMEM;
extern const char SETTING_LIST_GPS_LAT[] PROGMEM;
extern const char SETTING_LIST_GPS_LNG[] PROGMEM;
extern const char SETTING_LIST_DELAY_ON[] PROGMEM;
extern const char SETTING_LIST_DELAY_OFF[] PROGMEM;
extern const char SETTING_LIST_SAVE[] PROGMEM;
extern const char SETTING_LIST_SAVED[] PROGMEM;
extern const char SETTING_LIST_RESET[] PROGMEM;
extern const char SETTING_LIST_RESETED[] PROGMEM;
extern const char SETTING_LIST_EXIT[] PROGMEM;
extern const char *SETTING_LIST[] PROGMEM;

extern int settingMenuId;

extern light_control_data settingLightControlData;
extern setting_target currentSettingTarget;
extern char settingWifiSSID[MAX_SSID_LEN + 1];
extern char settingWifiPassword[MAX_PASSPHRASE_LEN + 1];
extern char inputBuffer[65];
extern int inputStrIndex;
extern int inputCursorIndex;

extern const char NUMBER_CHAR_SEQ[];
extern const char LETTER_CHAR_SEQ[];
extern const char PHONE_1_CHAR_SEQ[];
extern const char PHONE_2_CHAR_SEQ[];
extern const char PHONE_3_CHAR_SEQ[];
extern const char PHONE_4_CHAR_SEQ[];
extern const char PHONE_5_CHAR_SEQ[];
extern const char PHONE_6_CHAR_SEQ[];
extern const char PHONE_7_CHAR_SEQ[];
extern const char PHONE_8_CHAR_SEQ[];
extern const char PHONE_9_CHAR_SEQ[];
extern const char PHONE_0_CHAR_SEQ[];

#define DEBONCE_INPUT_DELAY (1000)
#define DEBONCE_FAST_INPUT_DELAY (400)
extern uint32_t debounceInputTs;
#define DEBONCE_FAST_COUNT (5)
extern int debonceInputCnt;
#define CURSOR_BLINK_DELAY (500)
extern bool cursorBlinked;
extern uint32_t cursorBlinkUpdateTs;
#endif

typedef struct
{
  bool &lightState;
} init_screen_prop;

typedef struct
{
  bool &lightState;
  bool &controlOverride;
  ntp_connect_status &ntpStatus;
  time_t &currentTime;
  time_t &nextAutoTime;
  bool &nextLightState;
} home_screen_prop;

typedef struct
{
  char *wifiSSID;
  char *wifiPassword;
  char *settingWifiSSID;
  char *settingWifiPassword;
  light_control_data &lightControlData;
  light_control_data &settingLightControlData;
} setting_screen_prop;

extern setting_target currentSettingTarget;

#if !LVGL_GUI
extern "C"
{
  void backup_new_settings();
  void apply_new_settings();
  void update_relay_state_to(bool state);
}
#endif

#if LVGL_GUI
static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
#endif
static void set_screen_brightness(uint16_t percent);
static void update_screen_light(screen_light_level level);

#if !LVGL_GUI
static void update_lcd_form_buffer();
static void replace_chars(char *target, char find, char replace);
#endif

#if LVGL_GUI
extern "C"
{
  void lv_set_visibility(lv_obj_t *target, bool visibility);
  void lv_set_state(lv_obj_t *target, bool state);
  void change_screen_to(lv_obj_t *target);
}
#endif

bool is_home_screen();
bool is_setting_screen();
bool is_input_screen();

void init_screen(init_screen_prop prop);

#if !LVGL_GUI
static void to_screen(screen_page screen);
static void reset_input_cursor(bool numberOnly);
static void reset_cursor_blink(bool resetCounter);

static void get_keyboard_input();
static void compute_keyboard_push();

static void rotate_charset_next(char &target, const char *chrSeq);
static void rotate_charset_reverse(char &target, const char *chrSeq);

static void home_screen_keys_action();
static void setting_screen_keys_action();
static void input_screen_keys_action();
static void input_screen_input_key_action();

#endif
void tick_ui_loop();

#if LVGL_GUI
extern "C" void update_ui_manual_switch_state(bool lightState);

static void update_ui_weekday(int weekdate);
#endif

void update_ui_home_screen(home_screen_prop prop);
void update_ui_setting_screen(setting_screen_prop prop);
void update_ui_input_screen();
#endif