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
#include "UsartAction.h"

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
        NETWORK_DISCONNECT,
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
        LEAVE_TRANSPARENT,
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
        IDLE
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
        static UsartAction initgsmUsart (u, UsartAction<>::INTERRUPT_ON);
        static UsartAction deinitgsmUsart (u, UsartAction<>::INTERRUPT_OFF);

        /*---------------------------------------------------------------------------*/
        /* clang-format off */

        m->transition (RESET_BOARD)->when (&softResetDelay);

        // Reset.
        m->state (RESET_BOARD, StateFlags::INITIAL)->entry (and_action (and_action (&longDelay, and_action (at ("+++"), &longDelay)), and_action (at ("AT+RST\r\n"), &deinitgsmUsart)))
                ->transition (INIT)->when (&alwaysTrue)->then (&longDelay);

        // Echo off
        m->state (INIT)->entry (and_action (&initgsmUsart, at ("ATE0\r\n")))
                ->transition (SET_OPERATING_MODE)->when (&ok)->then (&delay);

        // mode of operation. 1 Means "Station"
        m->state (SET_OPERATING_MODE)->entry (at ("AT+CWMODE_CUR=1\r\n"))
                ->transition (VERIFY_CONNECTED_ACCESS_POINT)->when (&ok)->then (&delay);

        m->state (VERIFY_CONNECTED_ACCESS_POINT)->entry (and_action (at ("AT+CWJAP?\r\n"), &delay))
                ->transition (CHECK_MY_IP)->when (anded (like ("%" SSID "%"), &ok))->then (&delay)
                ->transition (LIST_ACCESS_POINTS)->when (anded (eq ("AT+CWJAP?"), beginsWith ("busy")))->then (delayMs (5000))
                ->transition (LIST_ACCESS_POINTS)->when (anded (eq ("No AP"), &ok))
                ->transition (LIST_ACCESS_POINTS)->when (&alwaysTrue);

        m->state (CHECK_MY_IP)->entry (at ("AT+CIFSR\r\n"))
                ->transition (SET_MULTI_CONNECTION_MODE)->when (anded (like ("%.%.%.%"), &ok))->then (&delay)
                ->transition (LIST_ACCESS_POINTS)->when (&error)->then (&delay);

        m->state (LIST_ACCESS_POINTS)->entry (at ("AT+CWLAP\r\n"))
                ->transition (CONNECT_TO_ACCESS_POINT)->when (anded (like ("%" SSID "%"), &ok))->then (&delay);

        m->state (CONNECT_TO_ACCESS_POINT)->entry (at ("AT+CWJAP=\"" SSID "\",\"" PASS "\"\r\n"))
                ->transition (VERIFY_CONNECTED_ACCESS_POINT)->when (&ok)->then (&delay);

        // CIPMUX=0 means single connection, 1 means multiple.
        m->state (SET_MULTI_CONNECTION_MODE)->entry (at ("AT+CIPMUX=0\r\n"))
                ->transition (IDLE)->when (&ok)->then (&delay);

        m->state (IDLE, StateFlags::SUPPRESS_GLOBAL_TRANSITIONS)->transition (CONNECT_TO_SERVER)->when (eq ("_CONN"))->defer (0, true);

        m->state (CONNECT_TO_SERVER)->entry (at ("AT+CIPSTART=\"TCP\",\"192.168.0.31\",1883\r\n"))
                ->transition (NETWORK_BEGIN_SEND)->when (anded (eq ("CONNECT"), &ok))->then (&delay)
                ->transition (NETWORK_BEGIN_SEND)->when (anded (eq ("ALREADY CONNECTED"), eq ("ERROR")))->then (&delay);

        m->state (NETWORK_BEGIN_SEND)->entry (at ("AT+CIPMODE=1\r\n"))->exit (&delay)
                ->transition (NETWORK_PREPARE_SEND)->when (&ok);

        m->state (NETWORK_PREPARE_SEND)->entry (at ("AT+CIPSEND\r\n"))->exit (&delay)
                ->transition (NETWORK_SEND)->when (beginsWith (">"))
                ->transition (NETWORK_SEND)->when (&ok);

        static SendTransparentAction sendTransparentAction (dataToSendBuffer);
        m->state (NETWORK_SEND)->entry (&sendTransparentAction)
                ->transition(LEAVE_TRANSPARENT)->when (eq ("_CLOSE"))->defer (0, true)
                ->transition(NETWORK_SEND)->when (&alwaysTrue)->then (&delay);

        m->state (LEAVE_TRANSPARENT)->entry (and_action (and_action (&longDelay, at ("+++")), &longDelay))
                ->transition (NETWORK_DISCONNECT)->when (&alwaysTrue);

        m->state (NETWORK_DISCONNECT)->entry(at ("AT+CIPCLOSE\r\n"))->transition(IDLE)->when (&alwaysTrue);

        /* clang-format on */
}

/*****************************************************************************/

int Esp8266::send (uint8_t *data, size_t len) { return dataToSendBuffer.store (data, len); }

/*****************************************************************************/

bool Esp8266::connect (const char *address, uint16_t port)
{
        {
                InterruptLock<CortexMInterruptControl> lock;

                if (!getEventQueue ().push_back ()) {
                        return false;
                }
        }

        string &ev = getEventQueue ().back ();
        ev = "_CONN";
        return true;
}

/*****************************************************************************/

void Esp8266::disconnect ()
{
        {
                InterruptLock<CortexMInterruptControl> lock;

                if (!getEventQueue ().push_back ()) {
                        return;
                }
        }

        string &ev = getEventQueue ().back ();
        ev = "_CLOSE";
}

/*****************************************************************************/

bool Esp8266::isApConnected () const
{
        uint8_t l = machine.getCurrentStateLabel ();
        return isTcpConnected () || l == NETWORK_BEGIN_SEND || l == NETWORK_PREPARE_SEND || l == NETWORK_SEND || l == LEAVE_TRANSPARENT;
}

/*****************************************************************************/

bool Esp8266::isTcpConnected () const
{
        uint8_t l = machine.getCurrentStateLabel ();
        return l == NETWORK_BEGIN_SEND || l == NETWORK_PREPARE_SEND || l == NETWORK_SEND || l == LEAVE_TRANSPARENT;
}

/*****************************************************************************/

bool Esp8266::isSending () const { return (machine.getCurrentStateLabel () == NETWORK_SEND && dataToSendBuffer.size () > 0); }
