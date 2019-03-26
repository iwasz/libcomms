/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef USARTACTION_H
#define USARTACTION_H

#include "Action.h"
#include "BinaryEvent.h"
#include "Debug.h"
#include "Usart.h"

/**
 * @brief The InitUsartAction class
 */
template <typename EventT> class UsartAction : public Action<EventT> {
public:
        using EventType = EventT;

        enum Type { INTERRUPT_ON, INTERRUPT_OFF };
        UsartAction (Usart &u, Type a) : usart (u), action (a) {}

        virtual ~UsartAction () {}
        bool run (EventType const &event);

private:
        Usart &usart;
        Type action;
};

/*****************************************************************************/

template <typename EventT> bool UsartAction<EventT>::run (const EventType &)
{
        if (action == INTERRUPT_ON) {
#if 1
                debug->println ("usartStart");
#endif
                usart.startReceive ();
        }
        else {
#if 1
                debug->println ("usartStop");
#endif
                usart.stopReceive ();
        }
        return true;
}

#endif // USARTACTION_H
