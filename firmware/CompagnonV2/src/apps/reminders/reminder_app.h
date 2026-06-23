#pragma once
#include <Arduino.h>
#include <stdint.h>

struct Reminder {
    char     id[37];
    char     title[64];
    char     description[128];
    uint32_t epoch_trigger;
    uint32_t advance_s;
    bool     done;
    bool     active;
};

namespace apps::reminders {

void    app_init();
void    app_start();
void    app_stop();
void    app_tick();

bool    reminder_add   (const Reminder& r);
bool    reminder_delete(const char* id);
bool    reminder_done  (const char* id);
uint8_t reminder_list  (Reminder* out, uint8_t max_count);
Reminder reminder_next ();
void    reminder_trigger(const char* id);

String  reminder_to_json_all();
bool    reminder_from_json  (const String& json);

} // namespace apps::reminders
