/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "ErrorHandler.h"
#include "Gpio.h"
#include "MqttClient.h"
#include "Usart.h"
#include "catch.hpp"
#include "collection/Queue.h"
#include "mc60Modem/Mc60Modem.h"
#include "x86-test/Engine.h"
#include <iostream>

/**
 * @brief Storing some (little amounts of) data to freshly cleared file.
 */
#if 0
TEST_CASE ("Instantiation", "[mc60]")
{
        Gpio modemUartGpios (/*GPIOA, GPIO_PIN_2 | GPIO_PIN_3, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, GPIO_AF7_USART2*/);
        // HAL_NVIC_SetPriority (USART2_IRQn, 6, 0);
        // HAL_NVIC_EnableIRQ (USART2_IRQn);
        Usart modemUart (115200);

        using Event = std::string;
        using EventQueue = Queue<Event>;
        EventQueue eventQueue (16);
        LineSink<EventQueue, Event> lineSink (eventQueue);
        modemUart.setSink (&lineSink);

        // Do nóżki modemu PWRKEY
        Gpio pwrKeyGpio{ /*GPIOA, GPIO_PIN_12, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP*/ };
        // Do nóżki modemu status / vdd_ext
        Gpio statusGpio{ /*GPIOC, GPIO_PIN_6, GPIO_MODE_INPUT*/ };

        // Mc60Modem modem (modemUart, pwrKeyGpio, statusGpio, nullptr, false);

        Timer t{ 1000 };

        Gpio g;
        g.setOnToggle ([] { std::cerr << "Gpio::onToggle" << std::endl; });

        using namespace std::chrono;

        lm::Engine engine (std::array<lm::Event, 8>{ {
                { 100ms, std::unique_ptr<lm::ICommand>{ new lm::GpioState<true>{ g } } },
                { 200ms, std::unique_ptr<lm::ICommand>{ new lm::GpioState<true>{ g } } },
                { 300ms, std::unique_ptr<lm::ICommand>{ new lm::UsartChar{ modemUart, 'R' } } },
                { 301ms, std::unique_ptr<lm::ICommand>{ new lm::UsartChar{ modemUart, 'D' } } },
                { 302ms, std::unique_ptr<lm::ICommand>{ new lm::UsartChar{ modemUart, 'Y' } } },
                { 303ms, std::unique_ptr<lm::ICommand>{ new lm::UsartChar{ modemUart, '\r' } } },
                { 304ms, std::unique_ptr<lm::ICommand>{ new lm::UsartChar{ modemUart, '\n' } } },
                { 305ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "CONNECTED\r\n" } } },
        } });

        while (!t.isExpired ()) {
                engine.run ();
        }

        REQUIRE (eventQueue.size () == 2);
        REQUIRE (eventQueue.front () == "RDY");
        eventQueue.pop_front ();
        REQUIRE (eventQueue.front () == "CONNECTED");
}
#endif

class MqttCallback : public MqttClient::Callback {
public:
        virtual ~MqttCallback () override = default;

        void onConnected (MqttClient &owner) override
        {
                std::cout << "MqttCallback::onConnected" << std::endl;
                owner.subscribe ("config");
        }

        void onDisconnected (MqttClient &) override { std::cout << "MqttCallback::onDisconnected" << std::endl; }

        void onData (MqttClient &owner, const char *topic, uint8_t const *data, size_t len) override
        {
                std::cout << "MqttCallback::onData" << std::endl;
        }

        void onError (MqttClient &owner, MqttClient::Callback::Error error) override { std::cout << "MqttCallback::onError" << std::endl; }
};

TEST_CASE ("Modem", "[mc60]")
{
        Debug debug (nullptr);
        Debug::singleton () = &debug;
        ::debug = Debug::singleton ();
        ::debug->println ("IUM-light");

        Gpio modemUartGpios (/*GPIOA, GPIO_PIN_2 | GPIO_PIN_3, GPIO_MODE_AF_OD, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH, GPIO_AF7_USART2*/);
        // HAL_NVIC_SetPriority (USART2_IRQn, 6, 0);
        // HAL_NVIC_EnableIRQ (USART2_IRQn);
        Usart modemUart (115200);

        // Do nóżki modemu PWRKEY
        Gpio pwrKeyGpio{ /*GPIOA, GPIO_PIN_12, GPIO_MODE_OUTPUT_OD, GPIO_PULLUP*/ };
        // Do nóżki modemu status / vdd_ext
        Gpio statusGpio{ /*GPIOC, GPIO_PIN_6, GPIO_MODE_INPUT*/ };
        statusGpio = false;

        Mc60Modem modem (modemUart, pwrKeyGpio, statusGpio, nullptr, false);
        MqttClient mqtt (modem);
        modem.setCallback (&mqtt);
        MqttCallback mqttCallback;
        mqtt.setCallback (&mqttCallback);
        mqtt.connect ("trackmatevm.cloudapp.net", 1883);

        Timer t{ 5000 };

        Gpio g;
        g.setOnToggle ([] { std::cerr << "Gpio::onToggle" << std::endl; });

        std::string sss;

        using namespace std::chrono;

        lm::Engine engine (std::array<lm::Event, 38>{ {
                { 1000ms, std::unique_ptr<lm::ICommand>{ new lm::GpioState<true>{ statusGpio } } },

                { 10000ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, std::string ("AT\r\n") } } },
                { 10000ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10100ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+CPIN: READY\r\n" } } },
                { 10100ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10200ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+CSQ: 24,0\r\n" } } },
                { 10200ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10300ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+CREG: 1,1\r\n" } } },
                { 10300ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10400ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+CGATT: 1\r\n" } } },
                { 10400ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10500ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QIFGCNT=0\r\n" } } },
                { 10500ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10600ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QICSGP=xxx\r\n" } } },
                { 10600ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10700ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QIMUX=yyy\r\n" } } },
                { 10700ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10800ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QIMODE=0\r\n" } } },
                { 10800ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 10900ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QINDI=2\r\n" } } },
                { 10900ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                // Connected and first data reception

                { 11010ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "STATE: CONNECT OK\r\n" } } },

                { 11200ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QISDE=0\r\n" } } },
                { 11200ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 11300ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+QIRD: 40.114.228.99:1883,TCP,4\r\n" } } },
                { 11300ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, std::vector<uint8_t>{ 32, 2, 0, 0 } } } },

                // Sending 3 bytes of data

                { 11400ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+QISEND: <length>\r\n" } } },
                { 11400ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 11500ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QISEND=xxx\r\n" } } },
                { 11500ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 11600ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "SEND OK\r\n" } } },

                { 11700ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+QISACK: 3, 3, 0\r\n" } } },
                { 11700ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                // Second reception

                { 12700ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "STATE: CONNECT OK\r\n" } } },

                { 12800ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "AT+QISDE=0\r\n" } } },
                { 12800ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "OK\r\n" } } },

                { 12900ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, "+QIRD: 40.114.228.99:1883,TCP,2\r\n" } } },
                { 12900ms, std::unique_ptr<lm::ICommand>{ new lm::UsartString{ modemUart, std::vector<uint8_t>{ 43, 0 } } } },

        } });

        modem.send (0, std::array<uint8_t, 3>{ 0, 1, 2 }.data (), 3);

        while (true) {
                engine.run ();
                mqtt.run ();
        }
}
