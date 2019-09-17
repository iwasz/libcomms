/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "AbstractModem.h"
#include "BinaryEvent.h"
#include "Gpio.h"
#include "ICommunicationInterface.h"
#include "StateMachine.h"
#include "Types.h"
#include "Usart.h"
#include "character/BufferedCharacterSink.h"
#include "character/FixedLineSink.h"

/**
 * Quectel Mc60 implementation.
 *
 * TODO
 * - Rremove modem buffer size query : AT+QISEND=? because like most Quectel modems it returns "<length>"
 * instead of actual (numeric) length.
 */
class Mc60Modem : public AbstractModem {
public:
        static constexpr size_t TCP_SEND_DATA_DELAY_MS = 9000; /// Po jakim czasie anulujemy wysyłanie TCP jeśli się zwiesiło.
        static constexpr size_t SOFT_RESET_DELAY_MS = 15000;   /// Po jakim czasie resetujemy modem na miękko jeśli stan się nie zmienił.
        static constexpr size_t HARD_RESET_DELAY_MS = 14000;   /// Po jakim czasie resetujemy modem na TWARDO jeśli stan się nie zmienił.
        static constexpr size_t QIOPEN_BUF_LEN = 64;           /// Temporary buffer len.
        static constexpr size_t QUERY_ACK_COUNT_LIMIT = 8;     /// Ile razy (na s) sprawdzamy czy dostaliśmy ACK podczas wysyłania.
        static constexpr size_t BUFFERED_SINK_SIZE = 2048;     /// Pierwszy cache danych z modemu do µC.

        enum ConnectionState : uint8_t { NOT_CONNECTED, TCP_CONNECTED };

        /*---------------------------------------------------------------------------*/

        Mc60Modem (Usart &u, Gpio &pwrKeyPin, Gpio &statusPin, Callback *c = nullptr);
        ~Mc60Modem () override = default;

        /*-ICommunicationInterface---------------------------------------------------*/

        bool connect (const char *address, uint16_t port) override;
        void disconnect (int connectionId) override;
        int send (gsl::span<uint8_t> const &data) override;

        /*-AbstractModem-------------------------------------------------------------*/

        void power (bool on) override;

        /*-Mc60Modem-----------------------------------------------------------------*/

        void run () override
        {
                bufferedSink.run ();
                machine.run ();
        }

protected:
        StateMachine<BinaryEvent>::EventQueue &getEventQueue () { return machine.getEventQueue (); }

private:
        Buffer dataToSendBuffer; // Bufor na dane do wysłania przez TCP/IP za pośrednictwem modemu
        StateMachine<BinaryEvent> machine;
        LineSink2<StateMachine<BinaryEvent>::EventQueue, BinaryEvent> modemResponseSink;
        BufferedCharacterSink<BUFFERED_SINK_SIZE> bufferedSink; // Bufor na dane TCP/IP przychodzące z serwera. MC60 może zwrócić na raz 1500B.
        string address;                                         // Cache adresu, żeby reconnect.
        uint16_t port = 0;                                      // Cache portu, żeby reconnect.
};
