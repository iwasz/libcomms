/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef QUERY_ACK_NETWORK_ACTION_H
#define QUERY_ACK_NETWORK_ACTION_H

#include "Action.h"
#include "BinaryEvent.h"
#include "Debug.h"
#include <cstdio>
#include <cstring>

/**
 * @brief Parsuje odpowiedź z AT+QISACK, żeby wiedzieć ile bajtów faktycznie się
 * wysłało. Na tej podstawie zdejmujemy z kolejki jedynie liczbę bajtów która się
 * faktycznie wysłała.
 * Na starym modemie L66 komenda nazywała się AT+QISACK, a na BG96 używa się wariantu
 * AT+QISEND.
 */
template <typename T> class QueryAckAction : public Action<BinaryEvent> {
public:
        using IntType = T;

        QueryAckAction (IntType *s, IntType *a, IntType *n) : sent (s), acked (a), nAcked (n) {}
        virtual ~QueryAckAction () {}
        bool run (EventType const &event);

        static bool func (IntType a, IntType b) { return a == b; }

private:
        IntType *sent;
        IntType *acked;
        IntType *nAcked;
};

/*****************************************************************************/

template <typename T> bool QueryAckAction<T>::run (const EventType &event)
{
        *sent = 0;
        *acked = 0;
        *nAcked = 0;

        EventType copy = event;
        const char *input = reinterpret_cast<const char *> (copy.data ());

        char *b = strstr (input, "+QISACK:");

        if (!b) {
                return true;
        }

        b += 8;

        char *e = strchr (b, ',');

        if (!e) {
                return true;
        }

        *e = '\0';
        *sent = strtoul (b, nullptr, 10);
        b = e + 1;

        e = strchr (b, ',');

        if (!e) {
                *sent = 0;
                return true;
        }
        *e = '\0';

        *acked = strtoul (b, nullptr, 10);
        b = e + 1;

        *nAcked = strtoul (b, nullptr, 10);

#if 0
        // printf ("s = %d, a = %d, n = %d\n", *sent, *acked, *nAcked);
        debug->print ("sent : ");
        debug->print (*sent);
        debug->print (", acked: ");
        debug->print (*acked);
        debug->print (", nacked: ");
        debug->println (*nAcked);
#endif
        return true;
}

#endif // QueryAckAction_H
