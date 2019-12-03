/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Condition.h"
#include "Debug.h"
#include "CommsError.h"
#include "ErrorHandler.h"
#include <algorithm>
/**
 * Parses QIRC response and checks if it is non 0.
 */
template <typename IntT, typename EventT> class QirdiParseCondition : public Condition<EventT> {
public:
        using IntType = IntT;
        using EventType = EventT;

        QirdiParseCondition (IntType *br) : Condition<EventT> (InputRetention::IGNORE_INPUT), bytesReceived (br) {}

#ifndef UNIT_TEST
protected:
#endif
        virtual bool checkImpl (EventType const &event) const;

private:
        IntType *bytesReceived;
};

/*****************************************************************************/

// Typical response : +QIRDI: 0,1,0,1,1332,1332
template <typename IntT, typename EventT> bool QirdiParseCondition<IntT, EventT>::checkImpl (EventType const &event) const
{
        if (event.size () < 8) {
                return false;
        }

        if (EventType pattern{ '+', 'Q', 'I', 'R', 'D', 'I', ':', ' ' };
            std::search (event.cbegin (), event.cend (), pattern.cbegin (), pattern.cend ()) != event.cbegin ()) {
                return false;
        }

        typename EventT::const_reverse_iterator i;
        if ((i = std::find (event.crbegin (), event.crend (), ',')) == event.crend ()) {
                return false;
        }

        typename EventT::const_iterator j = i.base ();
        constexpr size_t MAX_BUF = 10;
        std::array<char, MAX_BUF> buf{};
        size_t size = std::distance (j, event.cend ());

        if (size >= MAX_BUF) {
                Error_Handler (QUIRDI_PARSE_SIZE_GT);
        }

        std::copy (j, event.cend (), buf.begin ());
        buf.at (size) = '\0';
        *bytesReceived = strtoul (buf.data (), nullptr, 10);

#if 1
        debug->print ("### QIRDI: ");
        debug->println (*bytesReceived);
#endif

        return *bytesReceived > 0;
}
