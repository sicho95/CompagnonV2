#pragma once

void power_mgr_init();
void power_mgr_tick();
void power_mgr_request_sleep();
void power_mgr_activity();   // appeler à chaque interaction UI/touch
