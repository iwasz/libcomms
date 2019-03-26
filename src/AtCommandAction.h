#ifndef GSMCOMMANDACTION_H
#define GSMCOMMANDACTION_H

#include "Action.h"
#include <cstdint>

#ifdef UNIT_TEST
#include <string>
#include <vector>

extern std::vector<std::string> gsmModemCommandsIssued;
#endif

/**
 * @brief Wysyłajakieś polecenie AT do modemu.
 */
class AtCommandAction : public Action<> {
public:
        /**
         * @brief Akcja wysyłająca komendę do modemu GSM.
         * @param c Komenda.
         */
        AtCommandAction (const char *c) : command (c), len (0) {}
        AtCommandAction (uint8_t const *c, uint16_t len) : command (reinterpret_cast<const char *> (c)), len (len) {}
        virtual ~AtCommandAction () {}
        virtual bool run (EventType const &event);

protected:
        const char *command;
        uint16_t len;
};

extern AtCommandAction *at (const char *command);
extern AtCommandAction *atBin (uint8_t const *command, uint16_t len);

#endif // GSMCOMMANDACTION_H
