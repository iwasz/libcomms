/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef ESP_8266_H
#define ESP_8266_H

#include "Gpio.h"
#include "ICommunicationInterface.h"
#include "StateMachine.h"
#include "Timer.h"
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
 * @brief The Bg96 class
 * TODO Do not CWLAP if already connected. ESP connects on its own when known AP is in range.
 * TODO Do not connect to AP as well if already connected (CWJAP disconnects and connects again in this situation).
 * TODO The addres we are connecting to is hardcoded. Api should have connect (returns ID/socket whatever) and disconnect like all normal APIS
 * does.
 * TODO Manage AP access somehow. Maybe some list of defined APs by the user?
 */
class Esp8266 : public WifiCard {
public:
        Esp8266 (Usart &u);
        virtual ~Esp8266 () override = default;

        int send (int connectionNumber, uint8_t *data, size_t len) override;
        bool connect (const char *address, uint16_t port);
        void disconnect (int connectionId);
        void run () override { machine.run (); }

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
        CircularBuffer dataToSendBuffer;
        StateMachine<> machine;
        LineSink<StateMachine<>::EventQueue, string> responseSink;
};

#endif // BG96_H
