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
#include "modem/Sms.h"

/**
 * @brief The AbstractModem class
 */
class AbstractModem : public ICommunicationInterface {
public:
        AbstractModem (Usart &u, Gpio &pwrKey, Gpio &status, Callback *c = nullptr)
            : ICommunicationInterface (c), usart (u), pwrKeyPin (pwrKey), statusPin (status)
        {
        }

        ~AbstractModem () override = default;
        virtual void power (bool on) = 0;
        SmsCollection &getSmsCollection () { return smsCollection; }

protected:
        Usart &usart;
        Gpio &pwrKeyPin;
        Gpio &statusPin;
        SmsCollection smsCollection;
};

#endif // ABSTRACTMODEM_H
