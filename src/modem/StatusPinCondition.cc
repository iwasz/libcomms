/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "StatusPinCondition.h"
#include <cstdio>

bool StatusPinCondition::checkImpl (const EventType &event) const
{
        /*
         * TODO on to powinien czytać z kolejki, żeby było koszernie, a potem ją wyczuści po przejściu.
         * Ale na początku kolejka i tak będzie pusta, więc nie ma tutaj znaczenia.
         */

#if 0 && !defined(UNIT_TEST)
        printf ("status pin : [%d]\n", b);
#endif

        return b == statusPin;
}
