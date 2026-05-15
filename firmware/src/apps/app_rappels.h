#pragma once
#include <time.h>
bool   app_rappels_start();
void   app_rappels_stop();
void   app_rappels_intent(const char* intent, const char* param);
time_t app_rappels_next_epoch(); // prochain rappel non déclenché
