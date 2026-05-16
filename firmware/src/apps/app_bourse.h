#pragma once
#include "app_base.h"   // Fix 3 — AppBase déclaré avant AppBourse

class AppBourse : public AppBase {
public:
    void        init()     override;
    void        update()   override;
    void        onResume() override;
    void        onPause()  override;
    const char* getName()  override { return "Bourse"; }
};
