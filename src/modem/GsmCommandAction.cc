#include "GsmCommandAction.h"
#include "Debug.h"
#include "Usart.h"
#include <cstdio>

//#ifndef UNIT_TEST
//#include "System.h"
//#endif

#ifdef UNIT_TEST
#include <iostream>
#endif

extern Usart *modemUsart;

#ifdef UNIT_TEST
std::vector<std::string> gsmModemCommandsIssued;

bool AtCommandAction::run (EventType const &)
{
        gsmModemCommandsIssued.push_back (command);

#ifdef UNIT_TEST
        std::cout << "OUT : " << command << std::flush;
#endif

        return true;
}

#else

bool AtCommandAction::run (EventType const &event)
{
        // Tu wysyłamy coś do modemu
        if (!len) {
                modemUsart->transmit (command);
#if 1
                debug->print ("OUT : ");
                debug->println (command);
#endif
        }
        else {
                // Brzydki kast
                modemUsart->transmit ((uint8_t *)(command), len);
#if 1
                debug->println ("OUT : <bin>");
#endif
        }

        return true;
}

#endif

/*****************************************************************************/

AtCommandAction *at (const char *command) { return new AtCommandAction (command); }

/*****************************************************************************/

extern AtCommandAction *atBin (uint8_t const *command, uint16_t len) { return new AtCommandAction (command, len); }
