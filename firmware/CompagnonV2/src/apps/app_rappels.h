#pragma once
#include "app_base.h"            // Fix 3 — AppBase déclaré avant AppRappels
#include "../storage/reminder_store.h"
#include <time.h>

class AppRappels : public AppBase {
public:
    void        init()     override;
    void        update()   override;
    void        onResume() override;
    void        onPause()  override;
    const char* getName()  override { return "Rappels"; }
    void        handleIntent(const char* intent, const char* param) override;
    time_t      nextEpoch() const;
};
