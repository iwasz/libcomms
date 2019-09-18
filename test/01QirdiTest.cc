/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "../src/mc60Modem/QirdiParseAction.h"
#include "catch.hpp"
#include <string>

TEST_CASE ("Instantiation", "[qirdi]")
{
        using QAction = QirdiParseAction<int, std::string>;

        {
                int i = 0;
                QAction a{ &i };
                REQUIRE (a.checkImpl ({ "+QIRDI: 0,1,0,1,1332,1332" }));
                REQUIRE (i == 1332);
        }

        {
                int i = 0;
                QAction a{ &i };
                REQUIRE (a.checkImpl ({ "+QIRDI: 0,1,0,2,1359,2691" }));
                REQUIRE (i == 2691);
        }

        {
                int i = 0;
                QAction a{ &i };
                REQUIRE (a.checkImpl ({ "+QIRDI: 0,1,0,3,2,2693" }));
                REQUIRE (i == 2693);
        }

        {
                int i = 0;
                QAction a{ &i };
                REQUIRE (!a.checkImpl ({ "" }));
                REQUIRE (i == 0);
        }

        {
                int i = 0;
                QAction a{ &i };
                REQUIRE (!a.checkImpl ({ "+QIRDI: 982348923489324" }));
                REQUIRE (i == 0);
        }
}
