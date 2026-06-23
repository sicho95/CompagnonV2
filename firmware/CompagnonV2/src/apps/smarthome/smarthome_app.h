#pragma once
#include "../app_base.h"

class AppSmarthome : public AppBase {
public:
    void        init() override;
    void        update() override;
    void        onResume() override;
    void        onPause() override;
    const char* getName() override { return "Domotique"; }
};
