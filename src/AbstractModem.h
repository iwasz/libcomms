/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef ABSTRACTMODEM_H
#define ABSTRACTMODEM_H

#include "Gpio.h"
#include "ICommunicationInterface.h"
#include "Usart.h"

/**
 * @brief The AbstractModem class
 */
class AbstractModem : public ICommunicationInterface {
public:
        AbstractModem (Usart &u, Gpio &pwrKey, Gpio &status, Callback *c = nullptr)
            : ICommunicationInterface (c), usart (u), pwrKeyPin (pwrKey), statusPin (status)
        {
        }
        virtual ~AbstractModem () = default;
        virtual void power (bool on) = 0;

protected:
        Usart &usart;
        Gpio &pwrKeyPin;
        Gpio &statusPin;
};

#endif // ABSTRACTMODEM_H
