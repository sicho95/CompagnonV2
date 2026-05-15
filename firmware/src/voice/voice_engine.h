#pragma once
#include <Arduino.h>
#include <functional>

namespace voice {

using WakeCallback = std::function<void(int word_id)>;
using SttCallback  = std::function<void(const String& text)>;

bool  voice_init(WakeCallback on_wake, SttCallback on_stt);
void  voice_start_task();
void  voice_stop_task();
void  voice_trigger_stt();
void  tts_speak(const String& text);
void  voice_set_silent(bool s);
bool  voice_is_silent();

} // namespace voice
