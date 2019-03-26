/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Esp8266.h"
#include "AtCommandAction.h"
#include "Credentials.h"
#include "SendNetworkAction.h"

// TODO !!!! wywalić!
Usart *modemUsart;

enum ModemState : size_t {
        RESET_STAGE_DECIDE,
        RESET_STAGE_POWER_OFF,
        RESET_STAGE_POWER_ON,
        INIT,
        LIST_ACCESS_POINTS,
        CONNECT_TO_ACCESS_POINT,
        SIGNAL_QUALITY_CHECK,
        VERIFY_CONNECTED_ACCESS_POINT,
        CHECK_MY_IP,
        SET_MULTI_CONNECTION_MODE,
        ACTIVATE_PDP_CONTEXT,
        APN_USER_PASSWD_INPUT,
        GPRS_CONNECTION_UP,
        DNS_CONFIG,
        CONNECT_TO_SERVER,
        CLOSE_AND_RECONNECT,
        SHUT_DOWN_STAGE_START,
        SHUT_DOWN_STAGE_POWER_OFF,
        SHUT_DOWN,
        GPRS_RESET,
        CANCEL_SEND,
        GPS_USART_ECHO_ON,
        NETWORK_SEND,
        NETWORK_BEGIN_SEND,
        NETWORK_PREPARE_SEND,
        NETWORK_QUERY_MODEM_OUTPUT_BUFFER_MAX_LEN,
        NETWORK_GPS_USART_ECHO_OFF,
        NETWORK_DECLARE_READ,
        NETWORK_ACK_CHECK,
        NETWORK_ACK_CHECK_PARSE,
        RESET_BOARD,
        SET_OPERATING_MODE,
        CHECK_OPERATING_MODE,
        CFG_CLOSE_AND_RECONNECT,
        CFG_CONNECT_TO_SERVER,
        CFG_PREPARE_SEND,
        CFG_SEND_QUERY,
        CFG_RECEIVE,
        NETWORK_ECHO_OFF,
        NETWORK_ECHO_ON,
        MQTT_PUB,
        CFG_CANCEL_SEND
};

/*****************************************************************************/

Esp8266::Esp8266 (Usart &u) : WifiCard (u), dataToSendBuffer (2048 * 3), responseSink (machine.getEventQueue ())
{
        modemUsart = &u;
        u.setSink (&responseSink);

        static StringCondition ok ("OK");
        static StringCondition okA (">OK");
        static bool alwaysTrueB = true;
        //        static bool alwaysFalseB = false;
        static BoolCondition alwaysTrue (&alwaysTrueB);
        //        static WatchdogRefreshAction wdgRefreshAction (&watchdog);

        static DelayAction delay (100);
        static DelayAction longDelay (1000);
        static LikeCondition error ("%ERROR%");
        static TimeCounter gsmTimeCounter; /// Odmierza czas między zmianami stanów GPS i GSM aby wykryć zwiechę.
        machine.setTimeCounter (&gsmTimeCounter);

        StateMachine<> *m = &machine;

        static TimePassedCondition softResetDelay (SOFT_RESET_DELAY_MS, &gsmTimeCounter);
        static TimePassedCondition hardResetDelay (HARD_RESET_DELAY_MS, &gsmTimeCounter);
        static_assert (HARD_RESET_DELAY_MS < SOFT_RESET_DELAY_MS, "HARD_RESET_DELAY_MS musi być mniejsze niż SOFT_RESET_DELAY_MS");
        static_assert (TCP_SEND_DATA_DELAY_MS < SOFT_RESET_DELAY_MS, "TCP_SEND_DATA_DELAY_MS musi być mniejsze niż SOFT_RESET_DELAY_MS");

        /*---------------------------------------------------------------------------*/
        /* clang-format off */

        m->transition (INIT)->when (&softResetDelay);

        m->state (INIT, StateFlags::INITIAL)->entry (at ("ATE1\r\n"))
                ->transition (CHECK_OPERATING_MODE)->when (&ok)->then (&delay);

        m->state (CHECK_OPERATING_MODE)->entry (and_action (at ("AT+CWMODE_DEF?\r\n"), &delay))
                ->transition (VERIFY_CONNECTED_ACCESS_POINT)->when (eq ("+CWMODE_DEF:1"))->then (&delay)
                ->transition (SET_OPERATING_MODE)->when (&alwaysTrue)->then (&delay);

        m->state (SET_OPERATING_MODE)->entry (at ("AT+CWMODE_DEF=1\r\n"))
                ->transition (RESET_BOARD)->when (&ok)->then (&delay);

        m->state (RESET_BOARD)->entry (at ("AT+RST\r\n"))
                ->transition (VERIFY_CONNECTED_ACCESS_POINT)->when (eq ("ready"))->then (&delay);

        m->state (VERIFY_CONNECTED_ACCESS_POINT)->entry (and_action (at ("AT+CWJAP?\r\n"), &delay))
                ->transition (CHECK_MY_IP)->when (like ("%" SSID "%"))->then (&delay)
                ->transition (LIST_ACCESS_POINTS)->when (anded (eq ("AT+CWJAP?"), beginsWith ("busy")))->then (delayMs (5000))
                ->transition (LIST_ACCESS_POINTS)->when (&alwaysTrue);

        m->state (CHECK_MY_IP)->entry (at ("AT+CIFSR\r\n"))
                ->transition (SET_MULTI_CONNECTION_MODE)->when (like ("%.%.%.%"))->then (&delay)
                ->transition (LIST_ACCESS_POINTS)->when (&error)->then (&delay);

        m->state (LIST_ACCESS_POINTS)->entry (at ("AT+CWLAP\r\n"))
                ->transition (CONNECT_TO_ACCESS_POINT)->when (like ("%" SSID "%"))->then (&delay);

        m->state (CONNECT_TO_ACCESS_POINT)->entry (at ("AT+CWJAP=\"" SSID "\",\"" PASS "\"\r\n"))
                ->transition (VERIFY_CONNECTED_ACCESS_POINT)->when (&ok)->then (&delay);

        m->state (SET_MULTI_CONNECTION_MODE)->entry (at ("AT+CIPMUX=1\r\n"))
                ->transition (CONNECT_TO_SERVER)->when (anded (eq ("AT+CIPMUX=1"), &ok))->then (&delay);

//        m->state (CLOSE_AND_RECONNECT)->entry (and_action (at ("AT+CIPCLOSE=0\r\n"), &delay))
//                ->transition (CONNECT_TO_SERVER)->when(&alwaysTrue)->then (&delay);

        m->state (CONNECT_TO_SERVER)->entry (at ("AT+CIPSTART=0,\"TCP\",\"trackmatevm.cloudapp.net\",1883\r\n"))
                ->transition (NETWORK_ECHO_OFF)->when (anded (like ("%,CONNECT"), &ok))->then (&delay)
                ->transition (NETWORK_ECHO_OFF)->when (anded (eq ("ALREADY CONNECTED"), eq ("ERROR")))->then (&delay);

        // Wyłącz ECHO podczas wysyłania danych.
        m->state (NETWORK_ECHO_OFF)->entry (and_action (at ("ATE0\r\n"), &delay))
                ->transition (NETWORK_BEGIN_SEND)->when (&alwaysTrue);

        m->state (NETWORK_BEGIN_SEND)->entry (&delay)->exit (&delay)
                ->transition (NETWORK_BEGIN_SEND)->whenf ([this] (string const &) { return dataToSendBuffer.size() == 0 ; })
                ->transition (NETWORK_PREPARE_SEND)->when (&alwaysTrue);

        static uint32_t bytesToSendInSendStage = 0;
        // Łapie odpowiedź z poprzedniego stanu, czyli max liczbę bajtów i wysyła komendę CIPSEND=<obliczona liczba B>
        static SendNetworkAction prepareAction (&dataToSendBuffer, SendNetworkAction::STAGE_PREPARE, &bytesToSendInSendStage);
        static IntegerCondition bytesToSendZero ((int *)&bytesToSendInSendStage, IntegerCondition<string>::EQ, 0);
        m->state (NETWORK_PREPARE_SEND)->entry (and_action (&prepareAction, &delay))
                ->transition (NETWORK_BEGIN_SEND)->when (&bytesToSendZero)
                ->transition (NETWORK_SEND)->when (&ok)
                ->transition (CONNECT_TO_SERVER)->when (&alwaysTrue)->then (&longDelay);

        // Ile razy wykonaliśmy cykl NETWORK_ACK_CHECK -> NETWORK_ACK_CHECK_PARSE (oczekiwanie na ACK danych).
//        static uint8_t ackQueryRetryNo = 0;
        static SendNetworkAction sendAction (&dataToSendBuffer, SendNetworkAction::STAGE_SEND, &bytesToSendInSendStage);
//        static IntegerAction resetRetry ((int *)&ackQueryRetryNo, IntegerAction::CLEAR);
//        static IntegerAction incRetry ((int *)&ackQueryRetryNo, IntegerAction::INC);
        m->state (NETWORK_SEND)->entry (&sendAction)
                ->transition (NETWORK_DECLARE_READ)->when (/*anded (*/eq ("SEND OK")/*, eq ("CLOSED"))*/)
                ->transition (CANCEL_SEND)->when (msPassed (TCP_SEND_DATA_DELAY_MS, &gsmTimeCounter))
                ->transition (CONNECT_TO_SERVER)->when (ored (ored (&error, eq ("CLOSED")), ored (eq ("SEND FAIL"), eq ("+PDP: DEACT"))))->then (&longDelay);

        static SendNetworkAction declareAction (&dataToSendBuffer, SendNetworkAction::STAGE_DECLARE, reinterpret_cast <uint32_t *> (&bytesToSendInSendStage));
        m->state (NETWORK_DECLARE_READ)->entry (&declareAction)
                ->transition (NETWORK_ECHO_ON)->when (&alwaysTrue);

        // Wyłącz ECHO podczas wysyłania danych.
        m->state (NETWORK_ECHO_ON)->entry (at ("ATE1\r\n"))
                ->transition (CONNECT_TO_SERVER)->when (&alwaysTrue);

        /* clang-format on */
}

/*****************************************************************************/

int Esp8266::send (int connectionNumber, uint8_t *data, size_t len) { dataToSendBuffer.store (data, len); }

/*****************************************************************************/

bool Esp8266::connect (const char *address, uint16_t port) {}

/*****************************************************************************/

void Esp8266::disconnect (int connectionId) {}
