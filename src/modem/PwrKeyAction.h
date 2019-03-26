/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef PWRKEYACTION_H
#define PWRKEYACTION_H

#include "Action.h"
#include "BinaryEvent.h"
#include "Gpio.h"
#include "Usart.h"

/**
 * @brief Wysyłajakieś polecenie AT do modemu.
 */
class PwrKeyAction : public Action<BinaryEvent> {
public:
        PwrKeyAction (bool b, Gpio &pwrKeyPin) : b (b), pwrKeyPin (pwrKeyPin) {}
        virtual ~PwrKeyAction () {}
        bool run (EventType const &event);

private:
        bool b;
        Gpio &pwrKeyPin;
};

#endif // PWRKEYACTION_H
