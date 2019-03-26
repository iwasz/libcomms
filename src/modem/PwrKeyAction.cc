/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "PwrKeyAction.h"
#include "Debug.h"
#include <cstdio>

bool PwrKeyAction::run (const EventType &event)
{
#if 0 && !defined(UNIT_TEST)
        printf ("pwr key set to : [%d]\n", b);
#endif

        pwrKeyPin = b;
        return true;
}
