/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef STATUSPINCONDITION_H
#define STATUSPINCONDITION_H

#include "BinaryEvent.h"
#include "Condition.h"
#include "Gpio.h"

/**
 * @brief Warunek porównujący wejście z napisem który podajemy jako arg. konstruktora.
 */
class StatusPinCondition : public Condition<BinaryEvent> {
public:
        StatusPinCondition (bool b, Gpio &s) : b (b), statusPin (s) {}
        virtual ~StatusPinCondition () = default;

private:
        bool checkImpl (EventType const &event) const;
        bool b;
        Gpio &statusPin;
};

#endif // STATUSPINCONDITION_H
