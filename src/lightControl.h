#ifndef _LIGHT_CONTROL_H
#define _LIGHT_CONTROL_H

#include <time.h>
#include <sunset.h>
#include "types.h"

class LightControl
{
public:
  LightControl();
  light_control_data setting;
  light_control_response response;
  void apply_check_at(time_t currentTime);

private:
  SunSet _sunsetCalculator;
  tm _timeStruct;
};

#endif