#include "lightControl.h"

LightControl::LightControl()
{
  setting = {
      .gpsLatitude = 14.0f,
      .gpsLongitude = 100.0f,
      .timezoneOffset = 7,
      .relayOffDelayMin = 0.0f,
      .relayOnDelayMin = 0.0f,
  };
}

void LightControl::apply_check_at(time_t currentTime)
{
  localtime_r(&currentTime, &_timeStruct);
  _sunsetCalculator.setPosition(setting.gpsLatitude, setting.gpsLongitude, setting.timezoneOffset);
  _sunsetCalculator.setCurrentDate(_timeStruct.tm_year + 1900, _timeStruct.tm_mon + 1, _timeStruct.tm_mday); // today
  double sunriseMinAfterMidnight = _sunsetCalculator.calcCivilSunrise() + setting.relayOffDelayMin;
  double sunsetMinAfterMidnight = _sunsetCalculator.calcCivilSunset() + setting.relayOnDelayMin;
  _sunsetCalculator.setCurrentDate(_timeStruct.tm_year + 1900, _timeStruct.tm_mon + 1, _timeStruct.tm_mday + 1); // tomorrow
  double nextSunriseMinAfterMidnight = _sunsetCalculator.calcCivilSunrise() + setting.relayOffDelayMin;

  _timeStruct.tm_hour = 0;
  _timeStruct.tm_min = sunriseMinAfterMidnight;
  _timeStruct.tm_sec = (sunriseMinAfterMidnight - (int)sunriseMinAfterMidnight) * 60;
  time_t sunriseTime = mktime(&_timeStruct);

  _timeStruct.tm_hour = 0;
  _timeStruct.tm_min = sunsetMinAfterMidnight;
  _timeStruct.tm_sec = (sunsetMinAfterMidnight - (int)sunsetMinAfterMidnight) * 60;
  time_t sunsetTime = mktime(&_timeStruct);

  _timeStruct.tm_hour = 0;
  _timeStruct.tm_min = nextSunriseMinAfterMidnight;
  _timeStruct.tm_sec = (nextSunriseMinAfterMidnight - (int)nextSunriseMinAfterMidnight) * 60;
  time_t nextSunriseTime = mktime(&_timeStruct);

  if (currentTime < sunriseTime)
  {
    response.nextAutoTime = sunriseTime;
    response.currentLightState = true;
    response.nextLightState = false;
  }
  else if (currentTime < sunsetTime)
  {
    response.nextAutoTime = sunsetTime;
    response.currentLightState = false;
    response.nextLightState = true;
  }
  else
  {
    response.nextAutoTime = nextSunriseTime;
    response.currentLightState = true;
    response.nextLightState = false;
  }
}