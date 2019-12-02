/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Action.h"
#include "BinaryEvent.h"
#include "Sms.h"
#include "Types.h"

/**
 * @brief Określa jaki jest rozmiar danych do wysłania i deklaruje ile chce wysłać.
 */
class SmsSendSingleAction : public Action<BinaryEvent> {
public:
        enum class Stage { NUMBER, BODY, REMOVE };

        explicit SmsSendSingleAction (SmsCollection &sv, Stage s) : smsVector (sv), stage{s} {}
        bool run (EventType const &event) override;

private:
        static constexpr size_t BUF_LEN = 200;
        static constexpr char CTRL_Z = 26;
        SmsCollection &smsVector;
        Stage stage{};
};
