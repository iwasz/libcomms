/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef BG96_MODEM_H
#define BG96_MODEM_H

#include "AbstractModem.h"
#include "BinaryEvent.h"
#include "Gpio.h"
#include "ICommunicationInterface.h"
#include "StateMachine.h"
#include "Usart.h"
#include "character/BufferedCharacterSink.h"
#include "character/FixedLineSink.h"
#include "collection/CircularBuffer.h"

/**
 * Quectel Mc60 implementation.
 *
 * TODO
 * - Rremove modem buffer size query : AT+QISEND=? because like most Quectel modems it returns "<length>"
 * instead of actual (numeric) length.
 */
class Mc60Modem : public AbstractModem {
public:
        Mc60Modem (Usart &u, Gpio &pwrKeyPin, Gpio &statusPin, Callback *c = nullptr, bool gpsOn = true);
        virtual ~Mc60Modem () override = default;

        /*-ICommunicationInterface---------------------------------------------------*/

        bool connect (const char *address, uint16_t port) override;
        void disconnect (int connectionId) override;
        int send (int connectionNumber, uint8_t *data, size_t len) override;

        /*-AbstractModem-------------------------------------------------------------*/

        void power (bool on) override;

        /*-Bg96Modem-----------------------------------------------------------------*/

        void run ()
        {
                bufferedSink.run ();
                machine.run ();
        }

        /*---------------------------------------------------------------------------*/

        /// Po jakim czasie anulujemy wysyłanie TCP jeśli się zwiesiło.
        static constexpr size_t TCP_SEND_DATA_DELAY_MS = 9000;

        /// Po jakim czasie resetujemy modem / OBD(?) na miękko jeśli stan się nie zmienił.
        static constexpr size_t SOFT_RESET_DELAY_MS = 15000;

        /// Po jakim czasie resetujemy modem / OBD(?) na TWARDO jeśli stan się nie zmienił.
        static constexpr size_t HARD_RESET_DELAY_MS = 14000;

        // Temporary buffer len.
        static constexpr size_t QIOPEN_BUF_LEN = 64;

        /// Ile razy (co sekundę) sprawdzamy czy dostaliśmy ACK podczas wysyłania danych telemetrycznych.
        static constexpr size_t QUERY_ACK_COUNT_LIMIT = 8;

        /// According to BG96 datasheet : Integer type. The socket service index. The range is 0-11
        static constexpr size_t MAX_CONNECTIONS = 12;

        enum ConnectionState : uint8_t { NOT_CONNECTED, TCP_CONNECTED };

protected:
        StateMachine<BinaryEvent>::EventQueue &getEventQueue () { return machine.getEventQueue (); }

private:
        // Bufor na dane do wysłania przez TCP/IP za pośrednictwem modemu
        CircularBuffer dataToSendBuffer;
        // TimeCounter gsmTimeCounter;  /// Odmierza czas między zmianami stanów GPS i GSM aby wykryć zwiechę.
        StateMachine<BinaryEvent> machine;

        FixedLineSink<StateMachine<BinaryEvent>::EventQueue, BinaryEvent> modemResponseSink;
        BufferedCharacterSink<128> bufferedSink;
        ConnectionState connectionState[MAX_CONNECTIONS] = { NOT_CONNECTED };
};

#endif // BG96_H
