#include "display.h"

#if LVGL_GUI
TFT_eSPI tft = TFT_eSPI();
FT62XXTouchScreen touchScreen = FT62XXTouchScreen(TFT_HEIGHT, PIN_SDA, PIN_SCL);
lv_disp_draw_buf_t dispBuffer;
lv_color_t *screenBuffer;
#endif

uint32_t screenLightTs;
screen_light_level screenLightLevel = SCREEN_LIGHT_BRIGHT;

#if LVGL_GUI
uint32_t wifiBlinkTs;
bool isWiFiBlinkOn = false;
#else
const char HOLLOW_CHR_DATA[] PROGMEM = {
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001,
    B11111,
};

LiquidCrystal_I2C lcd(0x27, 16, 2);
PCF8574 pcf8574(0x20);

char screenBuffer[2][17];

keypad_pressed_state keypad_state;
keypad_pressed_state prev_keypad_state;
keypad_pressed_state pushup_keypad_state;
keypad_pressed_state pushdown_keypad_state;

screen_flags screenFlags;
uint32_t lcdUpdateTs;

screen_page currentScreenPage = SCREEN_HOME;
extern bool lightState;
extern bool controlOverride;

const char LCD_LINE_TEMPLATE[] PROGMEM = "%-16.16s";
const char TIME_TEMPLATE[] PROGMEM = "%02d/%02d/%04d %02d:%02d";
const char SETTING_LIST_WIFI_SSID[] PROGMEM = "SET WIFI SSID";
const char SETTING_LIST_WIFI_PWD[] PROGMEM = "SET WIFI PWD";
const char SETTING_LIST_GPS_LAT[] PROGMEM = "SET GPS LAT";
const char SETTING_LIST_GPS_LNG[] PROGMEM = "SET GPS LNG";
const char SETTING_LIST_DELAY_ON[] PROGMEM = "SET DELAY ON";
const char SETTING_LIST_DELAY_OFF[] PROGMEM = "SET DELAY OFF";
const char SETTING_LIST_SAVE[] PROGMEM = "SAVE";
const char SETTING_LIST_SAVED[] PROGMEM = "SAVED";
const char SETTING_LIST_RESET[] PROGMEM = "RESET";
const char SETTING_LIST_RESETED[] PROGMEM = "RESETED";
const char SETTING_LIST_EXIT[] PROGMEM = "EXIT";
const char *SETTING_LIST[] PROGMEM = {
    SETTING_LIST_WIFI_SSID,
    SETTING_LIST_WIFI_PWD,
    SETTING_LIST_GPS_LAT,
    SETTING_LIST_GPS_LNG,
    SETTING_LIST_DELAY_ON,
    SETTING_LIST_DELAY_OFF,
    SETTING_LIST_RESET,
    SETTING_LIST_SAVE,
    SETTING_LIST_EXIT,
};

int settingMenuId;
extern light_control_data settingLightControlData;
extern char settingWifiSSID[MAX_SSID_LEN + 1];
extern char settingWifiPassword[MAX_PASSPHRASE_LEN + 1];
char inputBuffer[65];
int inputStrIndex;
int inputCursorIndex;

const char NUMBER_CHAR_SEQ[] = "1234567890.-";
const char LETTER_CHAR_SEQ[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz 1234567890.-_!?%@*#$&^+=,;:'\"()[]{}<>/\\|";
const char PHONE_1_CHAR_SEQ[] = "1 '\"()[]{}<>/\\|";
const char PHONE_2_CHAR_SEQ[] = "ABCabc2";
const char PHONE_3_CHAR_SEQ[] = "DEFdef3";
const char PHONE_4_CHAR_SEQ[] = "GHIghi4";
const char PHONE_5_CHAR_SEQ[] = "JKLjkl5";
const char PHONE_6_CHAR_SEQ[] = "MNOmno6";
const char PHONE_7_CHAR_SEQ[] = "PQRSpqrs7";
const char PHONE_8_CHAR_SEQ[] = "TUVtuv8";
const char PHONE_9_CHAR_SEQ[] = "WXYZwxyz9";
const char PHONE_0_CHAR_SEQ[] = "0.-_!?%@*#$&^+=,;:";

uint32_t debounceInputTs;
int debonceInputCnt;
bool cursorBlinked;
uint32_t cursorBlinkUpdateTs;
#endif

#if !LVGL_GUI
extern "C"
{
  void backup_new_settings();
  void apply_new_settings();

  void update_relay_state_to(bool state);
}
#endif

#if LVGL_GUI
static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}
static void touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  TouchPoint touchPos = touchScreen.read();
  if (touchPos.touched)
  {
    screenLightTs = millis();
    if (screenLightLevel != SCREEN_LIGHT_OFF)
    {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touchPos.xPos;
      data->point.y = touchPos.yPos;
    }
    else
    {
      data->state = LV_INDEV_STATE_REL;
    }
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}
#else
static void update_lcd_form_buffer()
{
  lcd.setCursor(0, 0);
  lcd.printf(LCD_LINE_TEMPLATE, screenBuffer[0]);
  lcd.setCursor(0, 1);
  lcd.printf(LCD_LINE_TEMPLATE, screenBuffer[1]);
}
static void replace_chars(char *target, char find, char replace)
{
  int len = strlen(target);
  for (int i = 0; i < len; i++)
  {
    if (target[i] == find)
    {
      target[i] = replace;
    }
  }
}
#endif

static void set_screen_brightness(uint16_t percent)
{
#if LVGL_GUI
  ledcWrite(TFT_BL_CHANNEL, map(percent, 0, 100, 0, 255));
#else
  lcd.setBacklight(percent > 0);
#endif
}
static void update_screen_light(screen_light_level level)
{
  if (level == screenLightLevel)
  {
    return;
  }
  screenLightLevel = level;
  switch (screenLightLevel)
  {
  case SCREEN_LIGHT_DIM:
    set_screen_brightness(25);
    break;
  case SCREEN_LIGHT_OFF:
    set_screen_brightness(0);
    break;
  case SCREEN_LIGHT_BRIGHT:
    set_screen_brightness(100);
    break;
  }
}

#if LVGL_GUI
void lv_set_visibility(lv_obj_t *target, bool visibility)
{
  if (visibility)
  {
    lv_obj_clear_flag(target, LV_OBJ_FLAG_HIDDEN);
  }
  else
  {
    lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
  }
}
void lv_set_state(lv_obj_t *target, bool state)
{
  if (state)
  {
    lv_obj_add_state(target, LV_STATE_CHECKED);
  }
  else
  {
    lv_obj_clear_state(target, LV_STATE_CHECKED);
  }
}
void change_screen_to(lv_obj_t *target)
{
  lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}
#endif

bool is_home_screen()
{
#if LVGL_GUI
  return lv_scr_act() == ui_HomeScreen;
#else
  return currentScreenPage == SCREEN_HOME;
#endif
}
bool is_setting_screen()
{
#if LVGL_GUI
  return lv_scr_act() == ui_SettingScreen;
#else
  return currentScreenPage == SCREEN_SETTING;
#endif
}
bool is_input_screen()
{
#if LVGL_GUI
  return lv_scr_act() == ui_KeyboardInputScreen;
#else
  return currentScreenPage == SCREEN_INPUT;
#endif
}

void init_screen(init_screen_prop prop)
{
#if LVGL_GUI
  lv_init(); // Init LVGL

  // Enable TFT
  tft.begin();
  tft.setRotation(1);

  // Blacklight PWM
  ledcSetup(TFT_BL_CHANNEL, 5000, 8);
  ledcAttachPin(TFT_BL, TFT_BL_CHANNEL);
  set_screen_brightness(100);

  // Start TouchScreen
  touchScreen.begin();

  // Display Buffer
  screenBuffer = (lv_color_t *)ps_malloc(BUFFER_SIZE * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&dispBuffer, screenBuffer, NULL, BUFFER_SIZE);

  // Initialize the display
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = TFT_WIDTH;
  disp_drv.ver_res = TFT_HEIGHT;
  disp_drv.flush_cb = disp_flush;
  disp_drv.draw_buf = &dispBuffer;
  lv_disp_drv_register(&disp_drv);

  // Init Touchscreen
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Init generated ui component (and display it)
  ui_init();
  update_ui_manual_switch_state(prop.lightState);

  wifiBlinkTs = millis();
  screenLightTs = millis();
#else
  pcf8574.pinMode(P7, OUTPUT, HIGH); // col1
  pcf8574.pinMode(P6, OUTPUT, HIGH); // col2
  pcf8574.pinMode(P5, OUTPUT, HIGH); // col3
  pcf8574.pinMode(P4, OUTPUT, HIGH); // col4

  pcf8574.pinMode(P3, INPUT_PULLUP); // row1
  pcf8574.pinMode(P2, INPUT_PULLUP); // row2
  pcf8574.pinMode(P1, INPUT_PULLUP); // row3
  pcf8574.pinMode(P0, INPUT_PULLUP); // row4

  Serial.print(F("pcf8574.begin():"));
  Serial.println(pcf8574.begin() ? 'T' : 'F');

  lcd.init();
  lcd.backlight();
  lcd.clear();

  screenFlags.forceLcdUpdate = true;
  lcdUpdateTs = millis();
#endif
}

#if !LVGL_GUI
static void to_screen(screen_page screen)
{
  if (currentScreenPage == screen)
  {
    return;
  }
  currentScreenPage = screen;
  screenFlags.forceLcdUpdate = true;
}
static void reset_input_cursor(bool numberOnly)
{
  inputStrIndex = 0;
  inputCursorIndex = 0;
  cursorBlinkUpdateTs = millis();
  cursorBlinked = true;
  screenFlags.inputNumberOnly = numberOnly;
}
static void reset_cursor_blink(bool resetCounter)
{
  debounceInputTs = millis();
  cursorBlinked = false;
  screenFlags.forceLcdUpdate = true;

  if (resetCounter)
  {
    debonceInputCnt = 0;
  }
  if (debonceInputCnt < DEBONCE_FAST_COUNT)
  {
    debonceInputCnt += 1;
  }
}
#endif

#if !LVGL_GUI
static void get_keyboard_input()
{
  prev_keypad_state = keypad_state;
  keypad_state = {};
  // col1
  pcf8574.digitalWrite(P7, LOW);
  PCF8574::DigitalInput input = pcf8574.digitalReadAll();
  keypad_state.keyLeft = input.p3 == LOW;
  keypad_state.key7 = input.p2 == LOW;
  keypad_state.key4 = input.p1 == LOW;
  keypad_state.key1 = input.p0 == LOW;
  // row 5 keypad_state.keyF1 = ?
  pcf8574.digitalWrite(P7, HIGH);

  // col2
  pcf8574.digitalWrite(P6, LOW);
  input = pcf8574.digitalReadAll();
  keypad_state.key0 = input.p3 == LOW;
  keypad_state.key8 = input.p2 == LOW;
  keypad_state.key5 = input.p1 == LOW;
  keypad_state.key2 = input.p0 == LOW;
  // row 5 keypad_state.keyF2 = ?
  pcf8574.digitalWrite(P6, HIGH);

  // col3
  pcf8574.digitalWrite(P5, LOW);
  input = pcf8574.digitalReadAll();
  keypad_state.keyRight = input.p3 == LOW;
  keypad_state.key9 = input.p2 == LOW;
  keypad_state.key6 = input.p1 == LOW;
  keypad_state.key3 = input.p0 == LOW;
  // row 5 keypad_state.keySharp = ?
  pcf8574.digitalWrite(P5, HIGH);

  // col4
  pcf8574.digitalWrite(P4, LOW);
  input = pcf8574.digitalReadAll();
  keypad_state.keyEnter = input.p3 == LOW;
  keypad_state.keyEsc = input.p2 == LOW;
  keypad_state.keyDown = input.p1 == LOW;
  keypad_state.keyUp = input.p0 == LOW;
  // row 5 keypad_state.keyAsterisk = ?
  pcf8574.digitalWrite(P4, HIGH);
}
static void compute_keyboard_push()
{
  pushdown_keypad_state = {
      .key1 = !prev_keypad_state.key1 && keypad_state.key1,
      .key2 = !prev_keypad_state.key2 && keypad_state.key2,
      .key3 = !prev_keypad_state.key3 && keypad_state.key3,
      .key4 = !prev_keypad_state.key4 && keypad_state.key4,
      .key5 = !prev_keypad_state.key5 && keypad_state.key5,
      .key6 = !prev_keypad_state.key6 && keypad_state.key6,
      .key7 = !prev_keypad_state.key7 && keypad_state.key7,
      .key8 = !prev_keypad_state.key8 && keypad_state.key8,
      .key9 = !prev_keypad_state.key9 && keypad_state.key9,
      .key0 = !prev_keypad_state.key0 && keypad_state.key0,
      .keyLeft = !prev_keypad_state.keyLeft && keypad_state.keyLeft,
      .keyRight = !prev_keypad_state.keyRight && keypad_state.keyRight,
      .keyUp = !prev_keypad_state.keyUp && keypad_state.keyUp,
      .keyDown = !prev_keypad_state.keyDown && keypad_state.keyDown,
      .keyEsc = !prev_keypad_state.keyEsc && keypad_state.keyEsc,
      .keyEnter = !prev_keypad_state.keyEnter && keypad_state.keyEnter,
      .keyF1 = !prev_keypad_state.keyF1 && keypad_state.keyF1,
      .keyF2 = !prev_keypad_state.keyF2 && keypad_state.keyF2,
      .keySharp = !prev_keypad_state.keySharp && keypad_state.keySharp,
      .keyAsterisk = !prev_keypad_state.keyAsterisk && keypad_state.keyAsterisk,
  };

  pushup_keypad_state = {
      .key1 = prev_keypad_state.key1 && !keypad_state.key1,
      .key2 = prev_keypad_state.key2 && !keypad_state.key2,
      .key3 = prev_keypad_state.key3 && !keypad_state.key3,
      .key4 = prev_keypad_state.key4 && !keypad_state.key4,
      .key5 = prev_keypad_state.key5 && !keypad_state.key5,
      .key6 = prev_keypad_state.key6 && !keypad_state.key6,
      .key7 = prev_keypad_state.key7 && !keypad_state.key7,
      .key8 = prev_keypad_state.key8 && !keypad_state.key8,
      .key9 = prev_keypad_state.key9 && !keypad_state.key9,
      .key0 = prev_keypad_state.key0 && !keypad_state.key0,
      .keyLeft = prev_keypad_state.keyLeft && !keypad_state.keyLeft,
      .keyRight = prev_keypad_state.keyRight && !keypad_state.keyRight,
      .keyUp = prev_keypad_state.keyUp && !keypad_state.keyUp,
      .keyDown = prev_keypad_state.keyDown && !keypad_state.keyDown,
      .keyEsc = prev_keypad_state.keyEsc && !keypad_state.keyEsc,
      .keyEnter = prev_keypad_state.keyEnter && !keypad_state.keyEnter,
      .keyF1 = prev_keypad_state.keyF1 && !keypad_state.keyF1,
      .keyF2 = prev_keypad_state.keyF2 && !keypad_state.keyF2,
      .keySharp = prev_keypad_state.keySharp && !keypad_state.keySharp,
      .keyAsterisk = prev_keypad_state.keyAsterisk && !keypad_state.keyAsterisk,
  };
}
#endif

#if !LVGL_GUI
static void rotate_charset_next(char &target, const char *chrSeq)
{
  int chrSeqLen = strlen(chrSeq);
  if (target == '\0')
  {
    target = chrSeq[0];
  }
  else if (target == chrSeq[chrSeqLen - 1])
  {
    target = '\0';
  }
  else
  {
    for (int i = 0; i < chrSeqLen - 1; i++)
    {
      if (chrSeq[i] == target)
      {
        target = chrSeq[i + 1];
        return;
      }
    }
    target = chrSeq[0];
  }
}
static void rotate_charset_reverse(char &target, const char *chrSeq)
{
  int chrSeqLen = strlen(chrSeq);
  if (target == '\0')
  {
    target = chrSeq[chrSeqLen - 1];
  }
  else if (target == chrSeq[0])
  {
    target = '\0';
  }
  else
  {
    for (int i = 1; i < chrSeqLen; i++)
    {
      if (chrSeq[i] == target)
      {
        target = chrSeq[i - 1];
        return;
      }
    }
    target = chrSeq[chrSeqLen - 1];
  }
}
#endif

#if !LVGL_GUI
static void home_screen_keys_action()
{
  if (pushup_keypad_state.keyEnter)
  {
    to_screen(SCREEN_SETTING);
    settingMenuId = 0;
    backup_new_settings();
  }

  if (pushup_keypad_state.keyEsc)
  {
    controlOverride = !controlOverride;
    screenFlags.forceLcdUpdate = true;
  }

  if (pushup_keypad_state.keyUp || pushup_keypad_state.keyDown)
  {
    if (controlOverride)
    {
      update_relay_state_to(!lightState);
      screenFlags.forceLcdUpdate = true;
    }
    else
    {
      screenFlags.homeShowAlternate = !screenFlags.homeShowAlternate;
      screenFlags.forceLcdUpdate = true;
    }
  }
}
static void setting_screen_keys_action()
{
  const int maxLength = sizeof(SETTING_LIST) / sizeof(SETTING_LIST[0]);
  if (pushup_keypad_state.keyEsc)
  {
    to_screen(SCREEN_HOME);
  }
  else if (pushup_keypad_state.keyEnter)
  {
    const char *pt = SETTING_LIST[settingMenuId];
    if (pt == SETTING_LIST_WIFI_SSID)
    {
      currentSettingTarget = SETTING_TARGET_WIFI_SSID;
      memset(inputBuffer, '\0', 65);
      strlcpy(inputBuffer, settingWifiSSID, MAX_SSID_LEN + 1);
      to_screen(SCREEN_INPUT);
      reset_input_cursor(false);
    }
    else if (pt == SETTING_LIST_WIFI_PWD)
    {
      currentSettingTarget = SETTING_TARGET_WIFI_PASSWORD;
      memset(inputBuffer, '\0', 65);
      strlcpy(inputBuffer, settingWifiPassword, MAX_PASSPHRASE_LEN + 1);
      to_screen(SCREEN_INPUT);
      reset_input_cursor(false);
    }
    else if (pt == SETTING_LIST_GPS_LAT)
    {
      currentSettingTarget = SETTING_TARGET_GPS_LATITUDE;
      memset(inputBuffer, '\0', 65);
      dtostrf(settingLightControlData.gpsLatitude, 0, 12, inputBuffer);
      to_screen(SCREEN_INPUT);
      reset_input_cursor(true);
    }
    else if (pt == SETTING_LIST_GPS_LNG)
    {
      currentSettingTarget = SETTING_TARGET_GPS_LONGITUDE;
      memset(inputBuffer, '\0', 65);
      dtostrf(settingLightControlData.gpsLongitude, 0, 12, inputBuffer);
      to_screen(SCREEN_INPUT);
      reset_input_cursor(true);
    }
    else if (pt == SETTING_LIST_DELAY_OFF)
    {
      currentSettingTarget = SETTING_TARGET_RELAY_OFF_MIN_OFFSET;
      memset(inputBuffer, '\0', 65);
      dtostrf(settingLightControlData.relayOffDelayMin, 0, 12, inputBuffer);
      to_screen(SCREEN_INPUT);
      reset_input_cursor(true);
    }
    else if (pt == SETTING_LIST_DELAY_ON)
    {
      currentSettingTarget = SETTING_TARGET_RELAY_ON_MIN_OFFSET;
      memset(inputBuffer, '\0', 65);
      dtostrf(settingLightControlData.relayOnDelayMin, 0, 12, inputBuffer);
      to_screen(SCREEN_INPUT);
      reset_input_cursor(true);
    }
    else if (pt == SETTING_LIST_RESET)
    {
      backup_new_settings();
      screenFlags.forceLcdUpdate = true;
      screenFlags.savedFlag = true;
    }
    else if (pt == SETTING_LIST_SAVE)
    {
      apply_new_settings();
      screenFlags.forceLcdUpdate = true;
      screenFlags.savedFlag = true;
    }
    else if (pt == SETTING_LIST_EXIT)
    {
      to_screen(SCREEN_HOME);
    }
  }
  if (pushup_keypad_state.keyUp)
  {
    settingMenuId = (settingMenuId > 0) ? (settingMenuId - 1) : (maxLength - 1);
    screenFlags.savedFlag = false;
    screenFlags.forceLcdUpdate = true;
  }
  else if (pushup_keypad_state.keyDown)
  {
    settingMenuId = (settingMenuId < (maxLength - 1)) ? (settingMenuId + 1) : 0;
    screenFlags.savedFlag = false;
    screenFlags.forceLcdUpdate = true;
  }
}
static void input_screen_keys_action()
{
  if (pushup_keypad_state.keyEsc)
  {
    to_screen(SCREEN_SETTING);
  }
  else if (pushup_keypad_state.keyEnter)
  {
    const char *pt = SETTING_LIST[settingMenuId];
    if (pt == SETTING_LIST_WIFI_SSID)
    {
      strlcpy(settingWifiSSID, inputBuffer, MAX_SSID_LEN + 1);
      to_screen(SCREEN_SETTING);
    }
    else if (pt == SETTING_LIST_WIFI_PWD)
    {
      strlcpy(settingWifiPassword, inputBuffer, MAX_PASSPHRASE_LEN + 1);
      to_screen(SCREEN_SETTING);
    }
    else if (pt == SETTING_LIST_GPS_LAT)
    {
      settingLightControlData.gpsLatitude = atof(inputBuffer);
      to_screen(SCREEN_SETTING);
    }
    else if (pt == SETTING_LIST_GPS_LNG)
    {
      settingLightControlData.gpsLongitude = atof(inputBuffer);
      to_screen(SCREEN_SETTING);
    }
    else if (pt == SETTING_LIST_DELAY_OFF)
    {
      settingLightControlData.relayOffDelayMin = atof(inputBuffer);
      to_screen(SCREEN_SETTING);
    }
    else if (pt == SETTING_LIST_DELAY_ON)
    {
      settingLightControlData.relayOnDelayMin = atof(inputBuffer);
      to_screen(SCREEN_SETTING);
    }
  }

  if (pushup_keypad_state.keyLeft)
  {
    if (inputCursorIndex > 1)
    { // shift left cursor first
      inputCursorIndex -= 1;
    }
    else if (inputStrIndex > 0)
    { // on edge : str shift second
      inputStrIndex -= 1;
    }
    else if (inputCursorIndex == 1 && inputStrIndex == 0)
    { // special case
      inputCursorIndex -= 1;
    }
  }
  else if (pushup_keypad_state.keyRight)
  {
    int inputLen = strlen(inputBuffer);
    if (inputCursorIndex + inputStrIndex < inputLen && inputLen < 64)
    {
      if (inputCursorIndex < 15)
      {
        inputCursorIndex += 1;
      }
      else if (inputLen - inputStrIndex > 15)
      {
        inputStrIndex += 1;
      }
    }
  }

  input_screen_input_key_action();
}

static void input_screen_input_key_action()
{
  int cursorIndex = inputCursorIndex + inputStrIndex;
  bool isInputDebounceReady = (millis() - debounceInputTs) >= (debonceInputCnt >= DEBONCE_FAST_COUNT ? DEBONCE_FAST_INPUT_DELAY : DEBONCE_INPUT_DELAY);

  if (keypad_state.key0)
  {
    if (pushup_keypad_state.key0 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key0);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "0.-" : PHONE_0_CHAR_SEQ);
    }
  }
  else if (keypad_state.key1)
  {
    if (pushup_keypad_state.key1 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key1);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "1" : PHONE_1_CHAR_SEQ);
    }
  }
  else if (keypad_state.key2)
  {
    if (pushup_keypad_state.key2 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key2);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "2" : PHONE_2_CHAR_SEQ);
    }
  }
  else if (keypad_state.key3)
  {
    if (pushup_keypad_state.key3 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key3);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "3" : PHONE_3_CHAR_SEQ);
    }
  }
  else if (keypad_state.key4)
  {
    if (pushup_keypad_state.key4 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key4);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "4" : PHONE_4_CHAR_SEQ);
    }
  }
  else if (keypad_state.key5)
  {
    if (pushup_keypad_state.key5 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key5);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "5" : PHONE_5_CHAR_SEQ);
    }
  }
  else if (keypad_state.key6)
  {
    if (pushup_keypad_state.key6 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key6);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "6" : PHONE_6_CHAR_SEQ);
    }
  }
  else if (keypad_state.key7)
  {
    if (pushup_keypad_state.key7 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key7);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "7" : PHONE_7_CHAR_SEQ);
    }
  }
  else if (keypad_state.key8)
  {
    if (pushup_keypad_state.key8 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key8);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "8" : PHONE_8_CHAR_SEQ);
    }
  }
  else if (keypad_state.key9)
  {
    if (pushup_keypad_state.key9 || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.key9);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? "9" : PHONE_9_CHAR_SEQ);
    }
  }
  else if (keypad_state.keyUp)
  {
    if (pushdown_keypad_state.keyUp || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.keyUp);
      rotate_charset_next(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? NUMBER_CHAR_SEQ : LETTER_CHAR_SEQ);
    }
  }
  else if (keypad_state.keyDown)
  {
    if (pushdown_keypad_state.keyDown || isInputDebounceReady)
    {
      reset_cursor_blink(pushdown_keypad_state.keyDown);
      rotate_charset_reverse(inputBuffer[cursorIndex], screenFlags.inputNumberOnly ? NUMBER_CHAR_SEQ : LETTER_CHAR_SEQ);
    }
  }
}
#endif
void tick_ui_loop()
{
#if LVGL_GUI
  if (millis() - wifiBlinkTs > UI_WIFI_BLINK_DURATION)
  {
    wifiBlinkTs = millis();
    isWiFiBlinkOn = !isWiFiBlinkOn;
  }

  uint32_t screenBlinkDuration = millis() - screenLightTs;
  if (screenBlinkDuration > (UI_SCREEN_EMIT_DURATION + UI_SCREEN_DIM_DURATION))
  {
    update_screen_light(SCREEN_LIGHT_OFF);
  }
  else if (screenBlinkDuration > UI_SCREEN_EMIT_DURATION)
  {
    update_screen_light(SCREEN_LIGHT_DIM);
  }
  else
  {
    update_screen_light(SCREEN_LIGHT_BRIGHT);
  }
#else
  get_keyboard_input();
  compute_keyboard_push();

  // keyboard action here
  if (keypad_state.key0 && pushup_keypad_state.keyUp)
  {
    update_screen_light(screenLightLevel == SCREEN_LIGHT_BRIGHT ? SCREEN_LIGHT_OFF : SCREEN_LIGHT_BRIGHT);
  }

  if (is_home_screen())
  {
    home_screen_keys_action();
  }
  else if (is_setting_screen())
  {
    setting_screen_keys_action();
  }
  else if (is_input_screen())
  {
    input_screen_keys_action();
  }
#endif
}

#if LVGL_GUI
void update_ui_manual_switch_state(bool lightState)
{
  lv_set_state(ui_HomeScreen_LightbulbManualSwitch, lightState);
}

static void update_ui_weekday(int weekdate)
{
  switch (weekdate)
  {
  case 0:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "SUN");
    break;
  case 1:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "MON");
    break;
  case 2:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "TUE");
    break;
  case 3:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "WED");
    break;
  case 4:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "THU");
    break;
  case 5:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "FRI");
    break;
  case 6:
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "SAT");
    break;
  }
}
#endif

void update_ui_home_screen(home_screen_prop prop)
{
  if (!is_home_screen())
  {
    return;
  }
#if LVGL_GUI
  // Set Lightblub status
  lv_obj_set_style_bg_color(ui_HomeScreen_LightbulbStatusLed, lv_color_hex(prop.lightState ? 0x12AE00 : 0xD20303), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(ui_HomeScreen_LightblubStateLabel, prop.lightState ? "ON" : "OFF");
  lv_label_set_text(ui_HomeScreen_OverrideStateLabel, prop.controlOverride ? "ON" : "OFF");

  // Set Control Override
  lv_set_visibility(ui_HomeScreen_LightbulbManualSwitch, prop.controlOverride);
  lv_set_visibility(ui_HomeScreen_LightbulbStatusLed, !prop.controlOverride);

  // Set WiFi LED
  wl_status_t wifiStatus = WiFi.status();
  bool isWifiOnImageOn;
  if (wifiStatus == WL_CONNECTED)
  {
    isWifiOnImageOn = true;
  }
  else if (wifiStatus == WL_IDLE_STATUS) // connecting
  {
    isWifiOnImageOn = isWiFiBlinkOn;
  }

  lv_set_visibility(ui_HomeScreen_WifiOnImage, isWifiOnImageOn);
  lv_set_visibility(ui_HomeScreen_WifiOffImage, !isWifiOnImageOn);

  // Set Datetime
  if (prop.ntpStatus == NTP_CONNECTED)
  {
    tm timeStruct;
    localtime_r(&prop.currentTime, &timeStruct);
    lv_label_set_text_fmt(ui_HomeScreen_TimeLabel, "%02d:%02d:%02d", timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec);
    lv_label_set_text_fmt(ui_HomeScreen_DateLabel, "%02d/%02d/%04d", timeStruct.tm_mday, timeStruct.tm_mon + 1, timeStruct.tm_year + 1900);
    update_ui_weekday(timeStruct.tm_wday);
  }
  else
  {
    lv_label_set_text(ui_HomeScreen_TimeLabel, "\?\?:\?\?:\?\?");
    lv_label_set_text(ui_HomeScreen_DateLabel, "\?\?/\?\?/\?\?\?\?");
    lv_label_set_text(ui_HomeScreen_WeekDayLabel, "\?\?\?");
  }
#else
  if (screenFlags.forceLcdUpdate || millis() - lcdUpdateTs > LCD_UPDATE_DELAY)
  {
    screenFlags.forceLcdUpdate = false;
    lcdUpdateTs = millis();
    tm timeStruct;
    localtime_r(&prop.currentTime, &timeStruct);
    if (prop.ntpStatus == NTP_CONNECTED)
    {
      sprintf(screenBuffer[0], TIME_TEMPLATE,
              timeStruct.tm_mday,
              timeStruct.tm_mon + 1,
              timeStruct.tm_year + 1900,
              timeStruct.tm_hour,
              timeStruct.tm_min);
    }
    else
    {
      sprintf(screenBuffer[0], "\?\?/\?\?/\?\?\?\? \?\?:\?\?");
    }

    // Bottom text
    if (screenFlags.homeShowAlternate && !prop.controlOverride)
    {
      if (prop.lightState)
      {
        sprintf(screenBuffer[1], "ON ");
      }
      else
      {
        sprintf(screenBuffer[1], "OFF ");
      }

      memset(screenBuffer[1] + strlen(screenBuffer[1]), LCD_BLOCK_CHR, 16 - strlen(screenBuffer[1]));

      localtime_r(&prop.nextAutoTime, &timeStruct);
      sprintf(screenBuffer[1] + 8, " < %02d:%02d", timeStruct.tm_hour, timeStruct.tm_min);
    }
    else
    {
      sprintf(screenBuffer[1], prop.controlOverride ? "SET " : "AUTO ");
      memset(screenBuffer[1] + strlen(screenBuffer[1]), LCD_BLOCK_CHR, 16 - strlen(screenBuffer[1]));
      if (prop.lightState)
      {
        sprintf(screenBuffer[1] + 13, " ON");
      }
      else
      {
        sprintf(screenBuffer[1] + 12, " OFF");
      }
    }

    update_lcd_form_buffer();
  }
#endif
}

void update_ui_setting_screen(setting_screen_prop prop)
{
  if (!is_setting_screen())
  {
    return;
  }

#if LVGL_GUI
  lv_set_visibility(ui_SettingScreen_WifiSSIDMarkImage, strcmp(prop.wifiSSID, prop.settingWifiSSID) != 0);
  lv_set_visibility(ui_SettingScreen_WifiPasswordMarkImage, strcmp(prop.wifiPassword, prop.settingWifiPassword) != 0);
  lv_set_visibility(ui_SettingScreen_GpsLatitudeMarkImage, prop.lightControlData.gpsLatitude != prop.settingLightControlData.gpsLatitude);
  lv_set_visibility(ui_SettingScreen_GpsLongitudeMarkImage, prop.lightControlData.gpsLongitude != prop.settingLightControlData.gpsLongitude);
  lv_set_visibility(ui_SettingScreen_RelayOffDelayMarkImage, prop.lightControlData.relayOffDelayMin != prop.settingLightControlData.relayOffDelayMin);
  lv_set_visibility(ui_SettingScreen_RelayOnDelayMarkImage, prop.lightControlData.relayOnDelayMin != prop.settingLightControlData.relayOnDelayMin);
#else
  if (screenFlags.forceLcdUpdate || millis() - lcdUpdateTs > LCD_UPDATE_DELAY)
  {
    screenFlags.forceLcdUpdate = false;
    lcdUpdateTs = millis();

    sprintf(screenBuffer[0], "#### OPTION ####");
    replace_chars(screenBuffer[0], '#', LCD_BLOCK_CHR);

    const char *pt = SETTING_LIST[settingMenuId];
    if (pt == SETTING_LIST_WIFI_SSID)
    {
      if (strcmp(prop.wifiSSID, prop.settingWifiSSID) != 0)
      {
        sprintf(screenBuffer[1], "%-14.14s *", SETTING_LIST[settingMenuId]);
      }
      else
      {
        sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
      }
    }
    else if (pt == SETTING_LIST_WIFI_PWD)
    {
      if (strcmp(prop.wifiPassword, prop.settingWifiPassword) != 0)
      {
        sprintf(screenBuffer[1], "%-14.14s *", SETTING_LIST[settingMenuId]);
      }
      else
      {
        sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
      }
    }
    else if (pt == SETTING_LIST_GPS_LAT)
    {
      if (prop.lightControlData.gpsLatitude != prop.settingLightControlData.gpsLatitude)
      {
        sprintf(screenBuffer[1], "%-14.14s *", SETTING_LIST[settingMenuId]);
      }
      else
      {
        sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
      }
    }
    else if (pt == SETTING_LIST_GPS_LNG)
    {
      if (prop.lightControlData.gpsLongitude != prop.settingLightControlData.gpsLongitude)
      {
        sprintf(screenBuffer[1], "%-14.14s *", SETTING_LIST[settingMenuId]);
      }
      else
      {
        sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
      }
    }
    else if (pt == SETTING_LIST_DELAY_OFF)
    {
      if (prop.lightControlData.relayOffDelayMin != prop.settingLightControlData.relayOffDelayMin)
      {
        sprintf(screenBuffer[1], "%-14.14s *", SETTING_LIST[settingMenuId]);
      }
      else
      {
        sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
      }
    }
    else if (pt == SETTING_LIST_DELAY_ON)
    {
      if (prop.lightControlData.relayOnDelayMin != prop.settingLightControlData.relayOnDelayMin)
      {
        sprintf(screenBuffer[1], "%-14.14s *", SETTING_LIST[settingMenuId]);
      }
      else
      {
        sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
      }
    }
    else if (pt == SETTING_LIST_RESET)
    {
      sprintf(screenBuffer[1], screenFlags.savedFlag ? SETTING_LIST_RESETED : SETTING_LIST_RESET);
    }
    else if (pt == SETTING_LIST_SAVE)
    {
      sprintf(screenBuffer[1], screenFlags.savedFlag ? SETTING_LIST_SAVED : SETTING_LIST_SAVE);
    }
    else
    {
      sprintf(screenBuffer[1], SETTING_LIST[settingMenuId]);
    }

    update_lcd_form_buffer();
  }
#endif
}

void update_ui_input_screen()
{
  if (!is_input_screen())
  {
    return;
  }

#if LVGL_GUI
#else
  if (millis() - cursorBlinkUpdateTs > CURSOR_BLINK_DELAY && !(keypad_state.keyDown || keypad_state.keyUp))
  {
    cursorBlinkUpdateTs = millis();
    cursorBlinked = !cursorBlinked;
    screenFlags.forceLcdUpdate = true;
  }
  if (screenFlags.forceLcdUpdate || millis() - lcdUpdateTs > LCD_UPDATE_DELAY)
  {
    screenFlags.forceLcdUpdate = false;
    lcdUpdateTs = millis();

    lcd.createChar(HOLLOW_CHR, HOLLOW_CHR_DATA);

    sprintf(screenBuffer[0], SETTING_LIST[settingMenuId]);
    char *strPt = inputBuffer + inputStrIndex;
    sprintf(screenBuffer[1], "%-16.16s", strPt);
    for (int i = 0; i < 16; i++)
    {
      if (strPt[i] == ' ')
      {
        screenBuffer[1][i] = HOLLOW_CHR;
      }
    }
    if (inputStrIndex > 0)
    {
      screenBuffer[1][16] = '>';
    }
    if (cursorBlinked)
    {
      screenBuffer[1][inputCursorIndex] = LCD_BLOCK_CHR;
    }

    update_lcd_form_buffer();
  }
#endif
}