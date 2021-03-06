/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef QUERY_RECV_NETWORK_ACTION_H
#define QUERY_RECV_NETWORK_ACTION_H

#include "Action.h"
#include "BinaryEvent.h"
#include "Condition.h"
#include "Debug.h"
#include <cstdio>
#include <cstring>

/**
 *
 */
template <typename T> class ParseRecvLengthAction : public Action<BinaryEvent> {
public:
        using IntType = T;

        ParseRecvLengthAction (/*IntType *ci,*/ IntType *br) : /*connectionId (ci),*/ bytesReceived (br) {}
        virtual ~ParseRecvLengthAction () {}
        bool run (EventType const &event);

        static bool func (IntType a, IntType b) { return a == b; }

private:
        //        IntType *connectionId;
        IntType *bytesReceived;
};

/*****************************************************************************/

template <typename T> bool ParseRecvLengthAction<T>::run (const EventType &event)
{
        *bytesReceived = 0;

        EventType copy = event;
        // TODO Very unsafe
        const char *input = reinterpret_cast<const char *> (copy.data ());
        char *b = strstr (input, "+QIRD: ");

        if (!b) {
                return true;
        }

        b += 7;

        //        char *e = strchr (b, ',');

        //        if (!e) {
        //                return true;
        //        }

        //        *e = '\0';
        //        *connectionId = strtoul (b, nullptr, 10);
        //        b = e + 1;

        *bytesReceived = strtoul (b, nullptr, 10);
#if 1
        //        debug->print (", connectionId: ");
        //        debug->print (*connectionId);
        debug->print (", bytesReceived: ");
        debug->println (*bytesReceived);
#endif
        return true;
}

// template <typename T> bool ParseRecvLengthAction<T>::run (const EventType &event)
//{
//        *bytesReceived = 0;

//        EventType copy = event;
//        // TODO Very unsafe
//        const char *input = reinterpret_cast<const char *> (copy.data ());
//        char *b = strstr (input, "+QIURC: \"recv\",");

//        if (!b) {
//                return true;
//        }

//        b += 15;

//        char *e = strchr (b, ',');

//        if (!e) {
//                return true;
//        }

//        *e = '\0';
//        *connectionId = strtoul (b, nullptr, 10);
//        b = e + 1;

//        *bytesReceived = strtoul (b, nullptr, 10);
//#if 0
//        debug->print (", connectionId: ");
//        debug->print (*connectionId);
//        debug->print (", bytesReceived: ");
//        debug->println (*bytesReceived);
//#endif
//        return true;
//}

/**
 * Parses QIRC response and checks if it is non 0.
 */
template <typename IntT, typename EventT> class ParseRecvLengthCondition : public Condition<EventT> {
public:
        using IntType = IntT;
        using EventType = EventT;

        ParseRecvLengthCondition (IntType *br) : Condition<EventT> (InputRetention::IGNORE_INPUT), bytesReceived (br) {}

#ifndef UNIT_TEST
protected:
#endif
        virtual bool checkImpl (EventType const &event) const;

private:
        IntType *bytesReceived;
};

/*****************************************************************************/

template <typename IntT, typename EventT> bool ParseRecvLengthCondition<IntT, EventT>::checkImpl (EventType const &event) const
{
        if (event.size () < 7) {
                return false;
        }

        if (event.at (0) == '+' && event.at (1) == 'Q' && event.at (2) == 'I' && event.at (3) == 'R' && event.at (4) == 'D'
            && event.at (5) == ':' && event.at (6) == ' ') {
                const char *input = reinterpret_cast<const char *> (event.data ()) + 7;
                char buf[10];
                int size = event.size () - 7;
                memcpy (buf, input, size);
                buf[size] = '\0';
                *bytesReceived = strtoul (buf, nullptr, 10);

#if 0
                debug->print (", bytesReceived: ");
                debug->println (*bytesReceived);
#endif

                return *bytesReceived > 0;
        }

        return false;
}

#endif // QueryAckAction_H
