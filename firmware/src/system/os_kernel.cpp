// ============================================================
// CompagnonV2 — system/os_kernel.cpp
// Fix deadlock WDT : plus de dispatch_post_sync.
// app_launch() poste UNE seule lambda fire-and-forget qui
// exécute init() (première fois) + onResume() dans task_ui_lvgl.
// task_os_main ne bloque plus jamais sur la queue UI.
// Fix : _initialized[id] positionné DANS la lambda, après init()
// réussi, pour éviter un flag "déjà init" si init() échoue.
// ============================================================
#include "os_kernel.h"
#include "../hal/display.h"
#include "../hal/rtc.h"
#include "../hal/pmu.h"
#include "../../include/pins.h"
#include "../apps/app_nestor.h"
#include "../apps/app_radars.h"
#include "../apps/app_bourse.h"
#include "../apps/app_meteo.h"
#include "../apps/app_rappels.h"
#include "../storage/reminder_store.h"
#include "../storage/nvs_store.h"
#include "../system/scheduler.h"
#include "../ui/notification_mgr.h"
#include "../ui/launcher.h"
#include "../ui/ui_dispatch.h"
#include "../voice/voice_engine.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Arduino.h>
#include <lvgl.h>

namespace voice {
    inline void speak(const char* text) { voice_engine_speak(text); }
    inline void set_mute(bool mute)     { voice_engine_set_silent(mute); }
}

namespace os {

static AppNestor  _app_nestor;
static AppRadars  _app_radars;
static AppBourse  _app_bourse;
static AppMeteo   _app_meteo;
static AppRappels _app_rappels;

static AppDesc   _apps[(int)AppId::COUNT];
static AppId     _current_app = AppId::NONE;
static bool      _silent      = false;
static bool      _time_valid  = false;
static bool      _initialized[(int)AppId::COUNT] = {};

static QueueHandle_t _intent_queue = nullptr;

struct PendingAlarm { char label[96]; };
static QueueHandle_t _alarm_queue = nullptr;

static constexpr const char* NVS_NS     = "os_cfg";
static constexpr const char* NVS_SILENT = "silent";

static void _run_ui_task_async_cb(void* user_data) {
    ui::UiTask* fn = static_cast<ui::UiTask*>(user_data);
    if (fn) {
        (*fn)();
        delete fn;
    }
}

static bool _schedule_ui_transition(ui::UiTask fn) {
    if (ui::dispatch_is_ui_thread()) {
        ui::UiTask* heap_fn = new ui::UiTask(std::move(fn));
        if (lv_async_call(_run_ui_task_async_cb, heap_fn) != LV_RESULT_OK) {
            delete heap_fn;
            return false;
        }
        return true;
    }
    return ui::dispatch_post(std::move(fn));
}

void kernel_init() {
    _intent_queue = xQueueCreate(8, sizeof(VoiceIntent));
    _alarm_queue  = xQueueCreate(4, sizeof(PendingAlarm));
    _silent       = NvsStore::getBool(NVS_NS, NVS_SILENT, false);
    _time_valid   = hal::rtc_is_valid();
    voice_engine_init();
    voice_engine_set_silent(_silent);
    Serial.printf("[KERNEL] init — silent=%d time_valid=%d\n", _silent, _time_valid);
}

void apps_register_all() {
    _apps[(int)AppId::NESTOR]  = { AppId::NESTOR,  "\U0001F916", &_app_nestor,
        [](const char* i, const char* p){ _app_nestor.handleIntent(i, p); } };
    _apps[(int)AppId::RADARS]  = { AppId::RADARS,  "\U0001F4E1", &_app_radars,  nullptr };
    _apps[(int)AppId::BOURSE]  = { AppId::BOURSE,  "\U0001F4C8", &_app_bourse,  nullptr };
    _apps[(int)AppId::METEO]   = { AppId::METEO,   "\U0001F324", &_app_meteo,   nullptr };
    _apps[(int)AppId::RAPPELS] = { AppId::RAPPELS, "\u23F0",     &_app_rappels,
        [](const char* i, const char* p){ _app_rappels.handleIntent(i, p); } };
    Serial.println("[KERNEL] 5 apps registered");
}

bool app_launch(AppId id) {
    if (id == AppId::NONE || (int)id >= (int)AppId::COUNT) return false;
    AppDesc& desc = _apps[(int)id];
    if (!desc.instance) return false;

    // _initialized capturé par valeur pour que la lambda sache si c'est
    // le premier lancement. On le passe à true DANS la lambda, APRES init(),
    // pour éviter qu'un crash dans init() marque l'app comme initialisée.
    bool already_init = _initialized[(int)id];
    AppBase* inst     = desc.instance;
    bool*    flag     = &_initialized[(int)id];
    AppId    prev_id  = _current_app;
    AppBase* prev     = nullptr;
    if (prev_id != AppId::NONE && prev_id != id) {
        prev = _apps[(int)prev_id].instance;
    }

    if (!_schedule_ui_transition([inst, prev, prev_id, id, already_init, flag]() {
        if (prev && prev_id == _current_app) {
            prev->onPause();
        }
        if (!already_init) {
            inst->init();
            *flag = true;   // marqué seulement après init() réussi
        }
        _current_app = id;
        inst->onResume();
        lv_obj_invalidate(lv_screen_active());
        lv_obj_invalidate(lv_layer_top());
        hal::display_force_refresh();
    })) {
        Serial.printf("[KERNEL] app_launch %d -> dispatch FAILED\n", (int)id);
        return false;
    }

    Serial.printf("[KERNEL] app_launch %d → dispatched (init=%s)\n",
                  (int)id, already_init ? "skip" : "yes");
    return true;
}

void app_close_current() {
    if (_current_app == AppId::NONE) return;
    AppId closing_id = _current_app;
    AppBase* inst = _apps[(int)_current_app].instance;
    if (!_schedule_ui_transition([inst, closing_id]() {
        if (_current_app != closing_id) return;
        Serial.printf("[KERNEL] app_close_current %d\n", (int)closing_id);
        if (inst) inst->onPause();
        _current_app = AppId::NONE;
        ui_launcher_show();
        lv_obj_invalidate(lv_screen_active());
        lv_obj_invalidate(lv_layer_top());
        hal::display_force_refresh();
    })) {
        Serial.printf("[KERNEL] app_close_current %d -> dispatch FAILED\n",
                      (int)closing_id);
    }
}

AppId    app_current()              { return _current_app; }
AppBase* app_get_instance(AppId id) {
    if ((int)id >= (int)AppId::COUNT) return nullptr;
    return _apps[(int)id].instance;
}

void kernel_post_intent(const VoiceIntent& intent) {
    if (_intent_queue) xQueueSend(_intent_queue, &intent, 0);
}

void kernel_set_silent(bool s) {
    _silent = s;
    NvsStore::setBool(NVS_NS, NVS_SILENT, s);
    voice::set_mute(s);
}
bool kernel_is_silent()            { return _silent; }
void kernel_set_time_valid(bool v)  { _time_valid = v; }
bool kernel_time_is_valid()         { return _time_valid; }

void kernel_schedule_next_reminder() {
    time_t next = _app_rappels.nextEpoch();
    if (next > 0) {
        hal::rtc_set_alarm(next);
        Serial.printf("[KERNEL] Next reminder alarm at epoch %lu\n", (unsigned long)next);
    } else {
        hal::rtc_clear_alarm();
    }
}

void kernel_post_alarm(const char* label) {
    if (!_alarm_queue) return;
    PendingAlarm a;
    strncpy(a.label, label, sizeof(a.label) - 1);
    a.label[sizeof(a.label) - 1] = '\0';
    xQueueSend(_alarm_queue, &a, 0);
}

void kernel_tick() {
    if (_current_app != AppId::NONE && _apps[(int)_current_app].instance)
        _apps[(int)_current_app].instance->update();

    static uint32_t _last_alarm_poll = 0;
    uint32_t now_ms = millis();
    if (now_ms - _last_alarm_poll >= 1000) {
        _last_alarm_poll = now_ms;
        time_t now = time(nullptr);
        int to_cancel = -1;
        Reminder fired_copy;
        for (const auto& a : Scheduler::getScheduled()) {
            if (a.trigger_epoch > 0 && a.trigger_epoch <= now + 2) {
                const Reminder* r = ReminderStore::getById(a.reminder_id);
                if (r && r->enabled) {
                    fired_copy = *r;
                    to_cancel  = a.reminder_id;
                    break;
                }
            }
        }
        if (to_cancel >= 0) {
            Serial.printf("[KERNEL] Alarm fired: %s\n", fired_copy.label.c_str());
            ui::notification_post(fired_copy.label, 8000);
            if (!_silent) voice::speak(fired_copy.label.c_str());
            Scheduler::cancelAlarm(to_cancel);
            kernel_schedule_next_reminder();
        }
    }

    PendingAlarm pending;
    while (xQueueReceive(_alarm_queue, &pending, 0) == pdTRUE) {
        ui::notification_post(String(pending.label), 8000);
        if (!_silent) voice::speak(pending.label);
    }

    VoiceIntent intent;
    while (xQueueReceive(_intent_queue, &intent, 0) == pdTRUE) {
        if (intent.target_app != AppId::NONE && intent.target_app != _current_app)
            app_launch(intent.target_app);
        AppId target = (intent.target_app != AppId::NONE) ? intent.target_app : _current_app;
        if (target != AppId::NONE && _apps[(int)target].handle_intent)
            _apps[(int)target].handle_intent(intent.intent, intent.param);
    }
}

} // namespace os
