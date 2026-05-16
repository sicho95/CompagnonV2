// ============================================================
// CompagnonV2 — storage/reminder_store.h
// Persistance des rappels en JSON sur FATFS (/reminders.json)
// ============================================================
#pragma once
#include <Arduino.h>
#include <vector>

namespace ReminderStore {

struct Reminder {
    int     id;               // auto-incrémenté
    String  label;            // texte du rappel
    time_t  datetime;         // epoch UTC de déclenchement
    int     advance_minutes;  // délai avant (0 = au moment pile)
    bool    enabled;
};

// Init — appeler APRÈS FFat.begin()
bool init();

// CRUD
bool load();                          // (re)lit /reminders.json
bool save();                          // écrit /reminders.json
int  add(const Reminder& r);          // retourne le nouvel id
bool remove(int id);
bool update(const Reminder& r);       // identifié par r.id

// Lecture
const std::vector<Reminder>& getAll();
const Reminder* getById(int id);

// Prochain rappel actif (pour le scheduler)
time_t nextTriggerEpoch();   // trigger = datetime - advance_minutes*60

} // namespace ReminderStore
