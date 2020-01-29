/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "SmsSendSingleAction.h"
#include "Usart.h"
#include <cstring>

extern Usart *modemUsart;

bool SmsSendSingleAction::run (const EventType &event)
{
        if (smsVector.empty ()) {
                return true;
        }

        char command[BUF_LEN];
        Sms const &sms = smsVector.front ();

        switch (stage) {
        case Stage::NUMBER:
                snprintf (command, BUF_LEN, "AT+CMGS=\"%s\"\r", sms.number.c_str ());
                modemUsart->transmit (command);
                return true;

        case Stage::BODY:
                snprintf (command, BUF_LEN, "%s%c", sms.body.c_str (), CTRL_Z);
                modemUsart->transmit (command);
                return true;

        case Stage::REMOVE:
                smsVector.pop_front ();
                return true;
        }

        return true;
}
