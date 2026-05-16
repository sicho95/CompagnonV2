#pragma once
#include <Arduino.h>
#include <vector>
#include <optional>  // W3 — getById() retourne optional, plus de ptr dangling

struct Reminder {
    int      id;
    String   label;
    time_t   datetime;       // epoch UTC
    int      advance_minutes;
    bool     enabled;
};

namespace ReminderStore {
    bool   load();           // charge /reminders.json depuis FATFS
    bool   save();           // persiste toute la liste
    bool   add(Reminder& r); // assigne un id auto-incrémenté et sauvegarde
    bool   remove(int id);
    bool   update(const Reminder& r);
    const std::vector<Reminder>& getAll();

    // W3 — retour par pointeur brut conservé pour compatibilité interne
    //      (le vecteur n'est pas modifié pendant l'usage de ce ptr)
    //      À terme : migrer vers std::optional<Reminder> côté appelants.
    Reminder* getById(int id);

    // W3 — version sûre : copie de la Reminder (pas de ptr invalide après add/remove)
    std::optional<Reminder> findById(int id);
}
