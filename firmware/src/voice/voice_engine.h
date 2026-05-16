#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void voice_engine_init();
void voice_engine_set_silent(bool silent);
bool voice_engine_is_listening();
#ifdef __cplusplus
}
#endif
