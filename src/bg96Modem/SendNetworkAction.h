/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef PREPARE_SEND_NETWORK_ACTION_H
#define PREPARE_SEND_NETWORK_ACTION_H

#include "Action.h"
#include "BinaryEvent.h"

class CircularBuffer;

/**
 * @brief Określa jaki jest rozmiar danych do wysłania i deklaruje ile chce wysłać.
 */
class SendNetworkAction : public Action<BinaryEvent> {
public:
        enum Stage { STAGE_PREPARE, STAGE_SEND, STAGE_DECLARE };

        SendNetworkAction (CircularBuffer *g, Stage s, int *b) : gsmBuffer (g), stage (s), bytesToSendInSendStage (b) {}

        virtual ~SendNetworkAction () {}
        bool run (EventType const &event);

        /**
         * Ile bajtów maksymalnie wysyłać za jednym razem (jeden CIPSEND). Jeśli modem ma mniejszy bufor,
         * to wyśle mniej, a jeśli bufor w µC ma mniej danych to też będzie uwzględnione.
         */
        static constexpr size_t GSM_MAX_BYTES_SEND = 1460;

        // Temporary buffer size.
        static constexpr size_t CIPSEND_BUF_LEN = 26;

#ifndef UNIT_TEST
private:
#endif
        static uint32_t parseCipsendRespoinse (EventType const &event);

private:
        CircularBuffer *gsmBuffer; // To są bajty do wysłania.
        Stage stage;
        int *bytesToSendInSendStage;
};

#endif // SendNetworkAction_H
