// ============================================================
// CompagnonV2 — apps/app_base.h
// Interface commune pour toutes les applications
// Hériter publiquement : class AppNestor : public AppBase
// ============================================================
#pragma once

class AppBase {
public:
    virtual void        init()     = 0;  // allocation LVGL, 1 fois
    virtual void        update()   = 0;  // appelé chaque tick LVGL (~5ms)
    virtual void        onResume() = 0;  // app passe au premier plan
    virtual void        onPause()  = 0;  // app passe en arrière-plan
    virtual const char* getName()  = 0;  // nom affiché dans le launcher
    virtual ~AppBase() = default;
};
