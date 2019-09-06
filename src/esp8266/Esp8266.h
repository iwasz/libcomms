/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Gpio.h"
#include "ICommunicationInterface.h"
#include "StateMachine.h"
#include "Timer.h"
#include "Types.h"
#include "Usart.h"
#include "character/LineSink.h"
#include "collection/CircularBuffer.h"

class WifiCard : public ICommunicationInterface {
public:
        explicit WifiCard (Usart &u) : usart (u) {}
        virtual ~WifiCard () = default;

protected:
        Usart &usart;
};

/**
 *
 */
template <typename QueueT, typename EventT> class IOBufferLineSink : public LineSink<QueueT, EventT> {
public:
        IOBufferLineSink (QueueT &g, buffer &receiveBuffer) : LineSink<QueueT, EventT> (g), receiveBuffer (receiveBuffer) {}
        void onData (uint8_t c) override;

        void setUseRawData (bool b) { useRawData = b; }
        bool isUseRawData () const { return useRawData; }

private:
        buffer &receiveBuffer;
        bool useRawData = false;
};

template <typename QueueT, typename EventT> void IOBufferLineSink<QueueT, EventT>::onData (uint8_t c)
{
        if (useRawData) {
                if (!receiveBuffer.full ()) {
                        receiveBuffer.push_back (c);
                }
        }
        else {
                LineSink<QueueT, EventT>::onData (c);
        }
}

/**
 * @brief The ESP8266 class
 * TODO Do not CWLAP if already connected. ESP connects on its own when known AP is in range.
 * TODO Do not connect to AP as well if already connected (CWJAP disconnects and connects again in this situation).
 * TODO The addres we are connecting to is hardcoded. Api should have connect (returns ID/socket whatever) and disconnect like all normal APIS
 * does.
 * TODO Manage AP access somehow. Maybe some list of defined APs by the user?
 */
class Esp8266 : public WifiCard {
public:
        Esp8266 (Usart &u);
        ~Esp8266 () override = default;

        int send (gsl::span<uint8_t> const &data) override;
        bool connect (const char *address, uint16_t port) override;
        void disconnect () override;

        bool isApConnected () const;
        bool isTcpConnected () const;
        bool isSending () const;

        buffer &getReceiveBuffer () { return receiveBuffer; }
        buffer const &getReceiveBuffer () const { return receiveBuffer; }

        virtual void run () { machine.run (); }

        /*---------------------------------------------------------------------------*/

        /// Po jakim czasie anulujemy wysyłanie TCP jeśli się zwiesiło.
        static constexpr size_t TCP_SEND_DATA_DELAY_MS = 9000;

        /// Po jakim czasie resetujemy modem / OBD(?) na miękko jeśli stan się nie zmienił.
        static constexpr size_t SOFT_RESET_DELAY_MS = 10000;

        /// Po jakim czasie resetujemy modem / OBD(?) na TWARDO jeśli stan się nie zmienił.
        static constexpr size_t HARD_RESET_DELAY_MS = 9000;

protected:
        StateMachine<>::EventQueue &getEventQueue () { return machine.getEventQueue (); }

private:
        buffer sendBuffer;    // Data to send over TCP
        buffer receiveBuffer; // Data to receive over TCP

        StateMachine<> machine;
        IOBufferLineSink<StateMachine<>::EventQueue, string> usartSink;
};
