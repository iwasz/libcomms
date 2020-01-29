/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <etl/cstring.h>
#include <etl/deque.h>

struct Sms {
        template <typename Nc, typename Bc> Sms (Nc const &n, Bc const &b) : number (std::move (n)), body (std::move (b)) {}

        etl::string<16> number;
        etl::string<160> body;
};

using SmsCollection = etl::deque<Sms, 4>;
