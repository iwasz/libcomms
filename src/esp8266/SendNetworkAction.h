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

class CircularBuffer;

/**
 * @brief Określa jaki jest rozmiar danych do wysłania i deklaruje ile chce wysłać.
 */
class SendNetworkAction : public Action<> {
public:
        enum Stage { STAGE_PREPARE, STAGE_SEND, STAGE_DECLARE };
        static constexpr size_t MAX_BYTES_SEND = 2048;

        SendNetworkAction (CircularBuffer *g, Stage s, uint32_t *b) : outputBuffer (g), stage (s), bytesToSendInSendStage (b) {}

        SendNetworkAction (SendNetworkAction const &) = delete;
        SendNetworkAction &operator= (SendNetworkAction const &) = delete;
        SendNetworkAction (SendNetworkAction const &&) = delete;
        SendNetworkAction &operator= (SendNetworkAction const &&) = delete;

        virtual ~SendNetworkAction () {}
        bool run (string const &);

#ifndef UNIT_TEST
private:
#endif

        static uint32_t parseCipsendRespoinse (const char *input);

private:
        CircularBuffer *outputBuffer; // To są bajty do wysłania.
        Stage stage;
        uint32_t *bytesToSendInSendStage;
};

#endif // SendNetworkAction_H
