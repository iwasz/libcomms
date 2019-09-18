/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Mc60Modem.h"
#include "QirdiParseAction.h"
#include "QueryAckAction.h"
#include "QueryRecvAction.h"
#include "SendNetworkAction.h"
#include "UsartAction.h"
#include "modem/GsmCommandAction.h"
#include "modem/PwrKeyAction.h"
#include "modem/StatusPinCondition.h"

// TODO !!!! wywalić!
Usart *modemUsart;

// TODO !!! Mega wywalić, total hack.
static size_t bytesReceived;

// It polutes the header file and the class body, so I put it here.
enum MachineState : size_t {
        NO_TRANSITION, // TODO dirty hack, remove
        RESET_STAGE_DECIDE,
        RESET_STAGE_POWER_OFF,
        RESET_STAGE_POWER_ON,
        INIT,
        PIN_STATUS_CHECK,
        ENTER_PIN,
        GET_SIM_IMSI,
        SIGNAL_QUALITY_CHECK,
        REGISTRATION_CHECK,
        GPRS_ATTACH_CHECK,
        SET_CONTEXT,
        MULTIPLE_CONNECTIONS_OFF,
        SET_MODEM_MODE,
        APN_USER_PASSWD_INPUT,
        GPRS_CONNECTION_UP,
        DNS_CONFIG,
        USE_DNS,
        SET_RECEIVE_MODE,
        CONNECT_TO_SERVER,
        CLOSE_AND_RECONNECT,
        CHECK_CONNECTION,
        SHUT_DOWN_STAGE_START,
        SHUT_DOWN_STAGE_POWER_OFF,
        SHUT_DOWN,
        GPRS_RESET,
        CANCEL_SEND,
        GPS_USART_ECHO_ON,
        NETWORK_SEND,
        NETWORK_BEGIN_SEND,
        NETWORK_CHECK_RECEIVE,
        NETWORK_BEGIN_RECEIVE,
        NETWORK_RECEIVE,
        NETWORK_RECEIVE_NOTIFY,
        NETWORK_PREPARE_SEND,
        NETWORK_QUERY_MODEM_OUTPUT_BUFFER_MAX_LEN,
        NETWORK_GPS_USART_ECHO_OFF,
        NETWORK_DECLARE_READ,
        NETWORK_ACK_CHECK,
        NETWORK_ACK_CHECK_PARSE,
        CONTROL_WAIT_FOR_CONNECT,
        AT_QBTPWR,
        AT_QBTVISB,
        AT_QBTGATSREG,
        AT_QBTGATSL
};

/*****************************************************************************/

Mc60Modem::Mc60Modem (Usart &u, Gpio &pwrKey, Gpio &status, Callback *c)
    : AbstractModem (u, pwrKey, status, c), modemResponseSink (machine.getEventQueue ()), bufferedSink (modemResponseSink)
{
        modemUsart = &u;
        u.setSink (&bufferedSink);

        static StringCondition<BinaryEvent> ok ("OK");
        static DelayAction<BinaryEvent> delay (100);
        static DelayAction<BinaryEvent> longDelay (1000);
        static LikeCondition<BinaryEvent> error ("%ERROR%");
        auto gsmPwrCycleOn = and_action<BinaryEvent> (and_action<BinaryEvent> (new PwrKeyAction (false, pwrKeyPin), delayMs<BinaryEvent> (1200)),
                                                      new PwrKeyAction (true, pwrKeyPin));

        auto gsmPwrCycleOff = and_action<BinaryEvent> (and_action<BinaryEvent> (new PwrKeyAction (false, pwrKeyPin), delayMs<BinaryEvent> (800)),
                                                       new PwrKeyAction (true, pwrKeyPin));

        static StatusPinCondition statusLow (false, statusPin);
        static StatusPinCondition statusHigh (true, statusPin);
        static TimeCounter gsmTimeCounter; /// Odmierza czas między zmianami stanów GPS i GSM aby wykryć zwiechę.
        static TimePassedCondition<BinaryEvent> softResetDelay (SOFT_RESET_DELAY_MS, &gsmTimeCounter);
        static TimePassedCondition<BinaryEvent> hardResetDelay (HARD_RESET_DELAY_MS, &gsmTimeCounter);
        static_assert (HARD_RESET_DELAY_MS < SOFT_RESET_DELAY_MS, "HARD_RESET_DELAY_MS musi być mniejsze niż SOFT_RESET_DELAY_MS");
        static_assert (TCP_SEND_DATA_DELAY_MS < SOFT_RESET_DELAY_MS, "TCP_SEND_DATA_DELAY_MS musi być mniejsze niż SOFT_RESET_DELAY_MS");
        static UsartAction<BinaryEvent> initgsmUsart (u, UsartAction<BinaryEvent>::INTERRUPT_ON);
        static UsartAction<BinaryEvent> deinitgsmUsart (u, UsartAction<BinaryEvent>::INTERRUPT_OFF);
        static TrueCondition<BinaryEvent> alwaysTrue;

        machine.setTimeCounter (&gsmTimeCounter);
        auto m = &machine;

        /*---------------------------------------------------------------------------*/
        /*--HARD-I-SOFT-RESET--------------------------------------------------------*/
        /*---------------------------------------------------------------------------*/
        /* clang-format off */

        static QirdiParseCondition <size_t, BinaryEvent> totalReceiveQirdiAction (&totalReceivedBytes);
        m->transition (GPRS_RESET)->when (&softResetDelay);
        m->transition (SHUT_DOWN_STAGE_START)->when (eq<BinaryEvent> ("_OFF"));
        m->transition (NO_TRANSITION, TransitionPriority::RUN_FIRST)->when (&totalReceiveQirdiAction)->thenf ([this] (BinaryEvent const & /*ev*/) { totalData.reserve(totalReceivedBytes); return true; });

        m->state (RESET_STAGE_DECIDE, StateFlags::INITIAL)->entry (/*and_action (&gpsReset,*/ and_action (&deinitgsmUsart, &delay))
                ->transition (PIN_STATUS_CHECK)->when (beginsWith<BinaryEvent> ("RDY")) // To oznacza, że wcześniej nie było zasilania, czyli nowy start.
                ->transition (RESET_STAGE_POWER_OFF)->when (&statusHigh)
                ->transition (RESET_STAGE_POWER_ON)->when (&statusLow)
                /*->transition (RESET_STAGE_DECIDE)->when (&hardResetDelay)*/;

        m->state (RESET_STAGE_POWER_OFF,  StateFlags::SUPPRESS_GLOBAL_TRANSITIONS)->entry (gsmPwrCycleOff)
                ->transition (RESET_STAGE_POWER_ON)->when (&statusLow)->then (delayMs<BinaryEvent> (500))
                ->transition (RESET_STAGE_DECIDE)->when (&hardResetDelay);

        // M66_Hardware_Design strona 25. Pierwszą komendę po 4-5 sekundach od włączenia.
        m->state (RESET_STAGE_POWER_ON)->entry (gsmPwrCycleOn)
                ->transition (PIN_STATUS_CHECK)->when (beginsWith<BinaryEvent> ("RDY"))
                ->transition (INIT)->when (&statusHigh)->then (and_action (and_action<BinaryEvent> (delayMs<BinaryEvent> (12000), &initgsmUsart), delayMs<BinaryEvent> (1000)))
                /*->transition (RESET_STAGE_DECIDE)->when (&hardResetDelay)*/;

        /*---------------------------------------------------------------------------*/

        m->state (GPRS_RESET)->entry (at ("AT+CIPSHUT\r\n"))
                ->transition (GPS_USART_ECHO_ON)->when (eq<BinaryEvent> ("SHUT OK"))
                ->transition (RESET_STAGE_DECIDE)->when (ored (&hardResetDelay, &error));

        m->state (GPS_USART_ECHO_ON)->entry (at ("ATE1\r\n"))
                ->transition (INIT)->when (anded <BinaryEvent>(msPassed<BinaryEvent> (100, &gsmTimeCounter), &ok));

        m->state (INIT)->entry (at ("AT\r\n"))
                ->transition (AT_QBTPWR)->when (anded<BinaryEvent> (eq<BinaryEvent> ("AT"), &ok))->then (&delay);

        // m->state (IPR)->entry (at ("AT+IPR=?\r\n"))
        //         ->transition (AT_QBTPWR)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+IPR"), &ok))->then (&delay);

        /*---------------------------------------------------------------------------*/
        /* BLE                                                                       */
        /*---------------------------------------------------------------------------*/

        m->state (AT_QBTPWR)->entry (at ("AT+QBTPWR=1\r\n"))
                ->transition (AT_QBTVISB)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QBTPWR"), &ok))->then (&delay);

        m->state (AT_QBTVISB)->entry (at ("AT+QBTVISB=0\r\n"))
                ->transition (AT_QBTGATSREG)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QBTVISB"), &ok))->then (&delay);

        m->state (AT_QBTGATSREG)->entry (at ("AT+QBTGATSREG=1,\"ABC2\"\r\n"))
                ->transition (AT_QBTGATSL)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QBTGATSREG"), &ok))->then (&delay)
                ->transition (PIN_STATUS_CHECK)->when (beginsWith<BinaryEvent> ("+CME ERROR:"));

        m->state (AT_QBTGATSL)->entry (at ("AT+QBTGATSL=\"ABC2\",1\r\n"))
                ->transition (PIN_STATUS_CHECK)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QBTGATSL"), &ok))->then (&delay);

        /*---------------------------------------------------------------------------*/

        m->state (PIN_STATUS_CHECK)->entry (at ("AT+CPIN?\r\n"))
                ->transition (GET_SIM_IMSI)->when (anded<BinaryEvent> (eq<BinaryEvent> ("+CPIN: READY"), &ok))->then (&delay)
                ->transition (PIN_STATUS_CHECK)->when (&error)->then (delayMs<BinaryEvent> (4000))
                ->transition (ENTER_PIN)->when (anded<BinaryEvent> (eq<BinaryEvent> ("+CPIN: SIM PIN"), &ok))->then (&delay);

        m->state (ENTER_PIN)->entry (at ("AT+CPIN=1220\r\n"))
                ->transition (PIN_STATUS_CHECK)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+CPIN="), &ok))->then (&delay)
                ->transition (PIN_STATUS_CHECK)->when (&error)->then (delayMs<BinaryEvent> (4000));

        m->state (GET_SIM_IMSI)->entry (at ("AT+CIMI\r\n"))
                ->transition (SIGNAL_QUALITY_CHECK)->when (&ok)->then (&delay);

        /*
         * Check Signal Quality response:
         * +CSQ: <rssi>,<ber>
         *
         * <rssi>
         * 0 -115 dBm or less
         * 1 -111 dBm
         * 2...30 -110... -54 dBm
         * 31 -52 dBm or greater
         * 99 not known or not detectable
         *
         * <ber> (in percent):
         * 0...7 As RXQUAL values in the table in GSM 05.08 [20] subclause 7.2.4
         * 99 Not known or not detectable
         */
        // TODO Tu powinno być jakieś logowanie tej siły sygnału.
        m->state (SIGNAL_QUALITY_CHECK)->entry (at ("AT+CSQ\r\n"))
                ->transition (SIGNAL_QUALITY_CHECK)->when (anded<BinaryEvent> (like<BinaryEvent> ("+CSQ: 99,%"), &ok))->then (&longDelay)
                ->transition (REGISTRATION_CHECK)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("+CSQ:"), &ok))->then (&delay);
                //->transition (SIGNAL_QUALITY_CHECK)->when (&ok)->then (&longDelay);

        /*
         * Check Network Registration Status response:
         * +CREG: <n>,<stat>[,<lac>,<ci>]
         *
         * <n>
         * 0 Disable network registration unsolicited result code
         * 1 Enable network registration unsolicited result code +CREG: <stat>
         * 2 Enable network registration unsolicited result code with location information +CREG: <stat>[,<lac>,<ci>]
         *
         * <stat>
         * 0 Not registered, MT is not currently searching a new operator to register to
         * 1 Registered, home network
         * 2 Not registered, but MT is currently searching a new operator to register to
         * 3 Registration denied
         * 4 Unknown
         * 5 Registered, roaming
         *
         * <lac>
         * String type (string should be included in quotation marks);
         * two byte location area code in hexadecimal format
         *
         * <ci>
         * String type (string should be included in quotation marks);
         * two byte cell ID in hexadecimal format
         */
        // TODO Parsowanie drugiej liczby po przecinku. Jak 1 lub 5, to idziemy dalej.
        m->state (REGISTRATION_CHECK)->entry (at ("AT+CREG?\r\n"))
                ->transition (SIGNAL_QUALITY_CHECK)->when (anded<BinaryEvent> (ored<BinaryEvent> (like<BinaryEvent> ("+CREG: %,0%"), like<BinaryEvent> ("+CREG: %,2%")), &ok))->then (delayMs<BinaryEvent> (5000))
                ->transition (SIGNAL_QUALITY_CHECK)->when (anded<BinaryEvent> (ored<BinaryEvent> (like<BinaryEvent> ("+CREG: %,3%"), like<BinaryEvent> ("+CREG: %,4%")), &ok))->then (delayMs<BinaryEvent> (5000))
                ->transition (INIT)->when (&error)->then (&longDelay)
                ->transition (GPRS_ATTACH_CHECK)->when (anded<BinaryEvent> (ored<BinaryEvent> (like<BinaryEvent> ("+CREG: %,1%"), like<BinaryEvent> ("+CREG: %,5%")), &ok)); // Zarejestorwał się do sieci.

        m->state (GPRS_ATTACH_CHECK)->entry (at ("AT+CGATT?\r\n"))
                ->transition (GPRS_ATTACH_CHECK)->when (anded<BinaryEvent> (eq<BinaryEvent> ("+CGATT: 0"), &ok))->then (&longDelay)
                ->transition (SET_CONTEXT)->when (anded<BinaryEvent> (eq<BinaryEvent> ("+CGATT: 1"), &ok));

        m->state (SET_CONTEXT)->entry (at ("AT+QIFGCNT=0\r\n"))
                ->transition(APN_USER_PASSWD_INPUT)->when (anded (eq<BinaryEvent> ("AT+QIFGCNT=0"), &ok));

        // Start Task and Set APN, USER NAME, PASSWORD
        // TODO Uwaga! kiedy ERROR, to idzie do GPRS_RESET, który ostatnio też zwracał ERROR i wtedy twardy reset. Czemu?
        m->state (APN_USER_PASSWD_INPUT)->entry (at ("AT+QICSGP=1,\"internet\"\r\n"))
                ->transition (GPRS_RESET)->when (&error)->then (&longDelay)
                ->transition (/*PDP_CONTEXT_CHECK*/MULTIPLE_CONNECTIONS_OFF)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QICSGP="), &ok))->then (&delay);

        m->state (MULTIPLE_CONNECTIONS_OFF)->entry (at ("AT+QIMUX=0\r\n"))
                  ->transition (SET_MODEM_MODE)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QIMUX="), &ok))->then (&delay);

        m->state (SET_MODEM_MODE)->entry (at ("AT+QIMODE=0\r\n"))
                 ->transition (/*DNS_CONFIG*/SET_RECEIVE_MODE)->when (anded<BinaryEvent> (eq<BinaryEvent> ("AT+QIMODE=0"), &ok))->then (&delay);

        m->state (DNS_CONFIG)->entry (at ("AT+QIDNSCFG=1,\"8.8.8.8\",\"8.8.4.4\"\r\n"))
                ->transition (INIT)->when (&error)->then (&longDelay)
                ->transition (USE_DNS)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QIDNSCFG="), &ok))->then (&delay);

        m->state (USE_DNS)->entry (at ("AT+QIDNSIP=1\r\n"))
                ->transition (SET_RECEIVE_MODE)->when (anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QIDNSIP="), &ok))->then (&delay);

        m->state (SET_RECEIVE_MODE)->entry (at ("AT+QINDI=2\r\n"))
                ->transition (CONTROL_WAIT_FOR_CONNECT)->when (anded<BinaryEvent> (eq<BinaryEvent> ("AT+QINDI=2"), &ok))->then (&delay);

        m->state (CONTROL_WAIT_FOR_CONNECT)
                // ->transition (CHECK_CONNECTION)->when (eq<BinaryEvent> ("_CONN", StripInput::DONT_STRIP, InputRetention::RETAIN_INPUT))->defer (0, true);
                ->transition (CHECK_CONNECTION)->whenf ([this] (BinaryEvent const &/*e*/) { return !address.empty() && port > 0; });

        /*---------------------------------------------------------------------------*/
        /*--Connecting---------------------------------------------------------------*/
        /*---------------------------------------------------------------------------*/

        m->state (CLOSE_AND_RECONNECT)->entry (at ("AT+QICLOSE\r\n"))
                ->transition (CONNECT_TO_SERVER)->when(anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QICLOSE"), &ok))->then (delayMs<BinaryEvent> (1000));

        // Kiedy nie ma połączenia, to ta komenda nic nie zwraca. ROTFL.
        m->state (CHECK_CONNECTION)->entry (at ("AT+QISTATE\r\n"))
                ->transition (NETWORK_GPS_USART_ECHO_OFF)->when (anded (beginsWith<BinaryEvent> ("STATE: CONNECT OK"), &ok))
                ->transition (CONNECT_TO_SERVER)->when (anded (beginsWith<BinaryEvent> ("STATE:"), &ok));

        /*
         * Połącz się z serwerem (połączenie TCP).
         */
        m->state (CONNECT_TO_SERVER)->entry (func<BinaryEvent> ([&u, this] (BinaryEvent const &/*e*/) {
                static char qiopenCommand[QIOPEN_BUF_LEN];
                snprintf (qiopenCommand, QIOPEN_BUF_LEN, "AT+QIOPEN=\"TCP\",\"%s\",\"%d\"\r\n", address.c_str(), port);
                u.transmit (qiopenCommand);
                debug->print(qiopenCommand);
                return true;
        }))
                ->transition (NETWORK_GPS_USART_ECHO_OFF)->when (anded (&ok, ored<BinaryEvent> (eq<BinaryEvent> ("CONNECT OK"), eq<BinaryEvent> ("ALREADY CONNECT"))))->then (and_action (delayMs<BinaryEvent> (1000), func<BinaryEvent> ([this] (BinaryEvent const &) {
                    if (callback) {
                        callback->onConnected (0);
                     }
                    return true;
                })))
                ->transition (CLOSE_AND_RECONNECT)->when (ored<BinaryEvent> (eq<BinaryEvent> ("CONNECT FAIL"), &error))->then(delayMs<BinaryEvent> (1000));

        static BeginsWithCondition<BinaryEvent> disconnected ("+QIURC: \"closed\",");

        /*---------------------------------------------------------------------------*/
        /*--Data-reception-----------------------------------------------------------*/
        /*---------------------------------------------------------------------------*/

        // Wyłącz ECHO podczas wysyłania danych.
        m->state (NETWORK_GPS_USART_ECHO_OFF)->entry (at ("AT+QISDE=0\r\n"))
                ->transition (NETWORK_CHECK_RECEIVE)->when (/*anded (&configurationWasRead,*/ anded<BinaryEvent> (beginsWith<BinaryEvent> ("AT+QISDE=0"), &ok));

        static ParseRecvLengthCondition <size_t, BinaryEvent> parseRecvLenCondition (&bytesReceived);
        static TimePassedCondition<BinaryEvent> recvDelay (100, &gsmTimeCounter);

        m->state (NETWORK_CHECK_RECEIVE)->
                transition (NETWORK_RECEIVE_NOTIFY)->whenf ([this] (BinaryEvent const &/*ev*/) { return totalData.size ()>0 && totalData.size () >= totalReceivedBytes; })->
                transition (NETWORK_BEGIN_RECEIVE)->whenf ([this] (BinaryEvent const &/*ev*/) { return totalReceivedBytes > 0; })->
                transition (NETWORK_BEGIN_SEND)->when (&alwaysTrue);

        m->state (NETWORK_BEGIN_RECEIVE)->entry (at ("AT+QIRD=0,1,0," MAX_MODEM_RECEIVE_BATCH_SIZE "\r\n"))
                ->transition (NETWORK_RECEIVE)->when (&parseRecvLenCondition)->thenf ([this] (BinaryEvent const &) {
                    modemResponseSink.receiveBytes (size_t (bytesReceived));
                    return true;
                })
                ->transition (NETWORK_BEGIN_SEND)->when (anded (eq <BinaryEvent> ("AT+QIRD=0,1,0," MAX_MODEM_RECEIVE_BATCH_SIZE), &ok))
                ->transition (CLOSE_AND_RECONNECT)->when (&error);

        m->state (NETWORK_RECEIVE)->exit (&delay)
                //->transition (NETWORK_CHECK_RECEIVE)->when (notEmpty<BinaryEvent> (InputRetention::RETAIN_INPUT))->thenf ([this] (BinaryEvent const &input) {
                ->transition (NETWORK_CHECK_RECEIVE)->when (anded (len <BinaryEvent> (&bytesReceived, InputRetention::RETAIN_INPUT), &ok))->thenf ([this] (BinaryEvent const &input) {
                    std::copy (input.cbegin (), input.cend (), std::back_inserter (totalData));
                    return true;
                });

        m->state (NETWORK_RECEIVE_NOTIFY)->exit (&delay)
                ->transition (NETWORK_BEGIN_SEND)->when (&alwaysTrue)->thenf ([this] (BinaryEvent const &/*input*/) {
                    if (callback != nullptr) {
                        callback->onData (totalData);
                    }

                    totalData.clear();
                    totalReceivedBytes = 0;
                    return true;
                });

        /*---------------------------------------------------------------------------*/
        /*--Data-sending-------------------------------------------------------------*/
        /*---------------------------------------------------------------------------*/

        /*
         * Sprawdź bufor wyjśy at (ten z zserializowanymi danymi). Jeśli pusty, to kręć się w tym stanie w kółko,
         * co daje szansę na uruchomienie się maszyny GPS (bo jest INC_SYNCHRO). Jeśli są jakieś dane do wysłania,
         * to sprawdź ile max może przyjąć modem.
         */
        static TimePassedCondition<BinaryEvent> waitForSendDelay (10 * 1000, &gsmTimeCounter);
        m->state (NETWORK_BEGIN_SEND)->exit (&delay)
                ->transition (CLOSE_AND_RECONNECT)->when (&disconnected)
                ->transition (NETWORK_PREPARE_SEND)->whenf ([this] (BinaryEvent const &) { return dataToSendBuffer.size() > 0 ; })
                ->transition (NETWORK_BEGIN_SEND)->when (&waitForSendDelay); // Tylko po to, żeby nam się maszyna nie zresetowała

        /*
         * Uwaga, wysłanie danych jest zaimplementowane w 2 stanach. W NETWORK_PREPARE_SEND idzie USARTem komenda AT+CIPSEND=<bbb>, a w
         * stanie NETWORK_SEND idzie <bbb> bajtów. Jeżeli w NETWORK_SEND nie wyślemy USARTem bajtów do modemu, to modem będzie czekał
         * bez żadnej odpowiedzi i to wygląda jakby się zawiesił.
         *
         * Uwaga, zauważyłem, że modem próbuje zwrócić echo z tymi wysłanymi danymi. Ja mu podaję opk 300B, a on mi odpowiada jakimś
         * "> -" i potem bajty. Firmware się zawiesza, bo ma bufor wejściowy tylko 128B. I ten błąd się objawiał niezależnie czy używałem
         * mojej implementacji sendBlocking czy HAL_UART_Transmit z HAL. Jedyna różnica była taka, że debuger lądował w innych miejscach.
         * Jednak w obydwu przypadkach wysłąnie większej ilości danych przez USART do modemu powodowało flage ORE (rx overrun) podczas
         * odbierania. Było to spowodowane właśnie tym idiotycznycm echem, które przepełniało bufor, ISR przestawało odczytywać flagę
         * RDR, co w konsekwencji oznacza że RXNE przestawało być czyszcone (odczyt z RDR je czyści AFAIK) i następował ORE.
         */
        static int bytesToSendInSendStage = 0;
        // Łapie odpowiedź z poprzedniego stanu, czyli max liczbę bajtów i wysyła komendę CIPSEND=<obliczona liczba B>
        static SendNetworkAction prepareAction (dataToSendBuffer, SendNetworkAction::STAGE_PREPARE, &bytesToSendInSendStage);
        static IntegerCondition<BinaryEvent> bytesToSendZero ((int *)&bytesToSendInSendStage, IntegerCondition<BinaryEvent>::EQ, 0);

        m->state (NETWORK_PREPARE_SEND)->entry (&prepareAction)
                ->transition (CLOSE_AND_RECONNECT)->when (&disconnected)
                ->transition (CHECK_CONNECTION)->when (&bytesToSendZero)
                ->transition (CLOSE_AND_RECONNECT)->when (&error)->then (&longDelay)
                ->transition (NETWORK_SEND)->when (beginsWith<BinaryEvent> ("AT+QISEND="));

        // Ile razy wykonaliśmy cykl NETWORK_ACK_CHECK -> NETWORK_ACK_CHECK_PARSE (oczekiwanie na ACK danych).
        static int ackQueryRetryNo = 0;
        static SendNetworkAction sendAction (dataToSendBuffer, SendNetworkAction::STAGE_SEND, &bytesToSendInSendStage);
        static IntegerAction<BinaryEvent> resetRetry ((int *)&ackQueryRetryNo, IntegerActionType::CLEAR);
        static IntegerAction<BinaryEvent> incRetry ((int *)&ackQueryRetryNo, IntegerActionType::INC);

        m->state (NETWORK_SEND)->entry (&sendAction)
                ->transition (CLOSE_AND_RECONNECT)->when (&disconnected)
                ->transition (NETWORK_ACK_CHECK)->when (eq<BinaryEvent> ("SEND OK"))->then (&resetRetry)
                ->transition (CANCEL_SEND)->when (msPassed<BinaryEvent> (TCP_SEND_DATA_DELAY_MS, &gsmTimeCounter))
                ->transition (CLOSE_AND_RECONNECT)->when (ored<BinaryEvent> (ored<BinaryEvent> (&error, eq<BinaryEvent> ("CLOSED")), ored (eq<BinaryEvent> ("SEND FAIL"), eq<BinaryEvent> ("+PDP: DEACT"))))->then (&longDelay);

        m->state (NETWORK_ACK_CHECK)->entry (at ("AT+QISACK\r\n"))
                ->transition (CLOSE_AND_RECONNECT)->when (&disconnected)
                ->transition (NETWORK_ACK_CHECK_PARSE)->when (anded<BinaryEvent> (&ok, beginsWith<BinaryEvent> ("+QISACK: ", StripInput::STRIP, InputRetention::RETAIN_INPUT)));

        static int sent;
        static int acked;
        static int nAcked;
        static QueryAckAction queryAck (&sent, &acked, &nAcked);
        static IntegerCondition<BinaryEvent> allAcked (&sent, IntegerCondition<BinaryEvent>::EQ, &acked);
        static IntegerCondition<BinaryEvent> retryLimitReached (&ackQueryRetryNo, IntegerCondition<BinaryEvent>::GT, QUERY_ACK_COUNT_LIMIT);

        m->state (NETWORK_ACK_CHECK_PARSE)->entry (&queryAck)->exit (&longDelay)
                ->transition (CLOSE_AND_RECONNECT)->when (&disconnected)
                ->transition (CANCEL_SEND)->when (&retryLimitReached)->thenf ([&ackQueryRetryNo] (BinaryEvent const &) { ackQueryRetryNo = 0; return true; })
                ->transition (NETWORK_DECLARE_READ)->when (&allAcked)->then (&resetRetry)
                ->transition (NETWORK_ACK_CHECK)->when (&alwaysTrue)->then (&incRetry);

        // static SendNetworkAction declareAction (&outputBuffer, SendNetworkAction::STAGE_DECLARE, reinterpret_cast <uint32_t *> (&acked));
        // TODO nie ackuje mi!!!
        static SendNetworkAction declareAction (dataToSendBuffer, SendNetworkAction::STAGE_DECLARE, &bytesToSendInSendStage);
        m->state (NETWORK_DECLARE_READ)->entry (&declareAction)
                ->transition (CLOSE_AND_RECONNECT)->when (&disconnected)
                ->transition (CHECK_CONNECTION)->when (&alwaysTrue);

        /*---------------------------------------------------------------------------*/

        m->state (CANCEL_SEND)->entry (and_action<BinaryEvent> (delayMs<BinaryEvent> (1200), at ("+++\r\n")))
                ->transition (CHECK_CONNECTION)->when (eq<BinaryEvent> ("SEND OK"))
                ->transition (GPRS_RESET)->when (ored<BinaryEvent> (&error, eq<BinaryEvent> ("> -")))->then (delayMs<BinaryEvent> (500));

        /*---------------------------------------------------------------------------*/
        /*--WYŁĄCZENIE-PODCZAS-SLEEP-------------------------------------------------*/
        /*---------------------------------------------------------------------------*/

        m->state (SHUT_DOWN_STAGE_START)
                ->transition (SHUT_DOWN_STAGE_POWER_OFF)->when (&statusHigh)
                ->transition (SHUT_DOWN)->when (&statusLow);

        // Suppress, bo wyłączanie modemu może potrwać nawet do 12 sekund.
        m->state (SHUT_DOWN_STAGE_POWER_OFF, StateFlags::SUPPRESS_GLOBAL_TRANSITIONS)->entry (and_action<BinaryEvent> (&deinitgsmUsart, gsmPwrCycleOn))
                ->transition (SHUT_DOWN)->when (&statusLow)->then (delayMs<BinaryEvent> (500));

        // Stąd tylko resetem maszyny stanów.
        m->state (SHUT_DOWN, StateFlags::SUPPRESS_GLOBAL_TRANSITIONS);
        /* clang-format on */
}

/*****************************************************************************/

void Mc60Modem::power (bool on)
{

        if (!on) {
                {
                        InterruptLock<CortexMInterruptControl> lock;

                        if (getEventQueue ().full ()) {
                                return;
                        }

                        getEventQueue ().push_back (BinaryEvent{ '_', 'O', 'F', 'F' });

                        //                        if (!getEventQueue ().push_back ()) {
                        //                                return;
                        //                        }
                }

                //                BinaryEvent &ev = getEventQueue ().back ();
                //                ev = { '_', 'O', 'F', 'F' };
        }
        else {
                machine.reset ();
        }
}

/*****************************************************************************/

bool Mc60Modem::connect (const char *address, uint16_t port)
{
        //        {
        //                InterruptLock<CortexMInterruptControl> lock;

        //                if (!getEventQueue ().push_back ()) {
        //                        return false;
        //                }
        //        }

        //        BinaryEvent &ev = getEventQueue ().back ();
        //        ev = { '_', 'C', 'O', 'N', 'N' };
        //        ev.argInt2 = 0;
        //        ev.argInt1 = port;
        //        ev.argStr = address;
        this->address = string{ address };
        this->port = port;
        return true;
}

/*****************************************************************************/

void Mc60Modem::disconnect (int /*connectionId*/)
{
        // TODO to tak na prawdę nie rozłącza, tylko nie pozwoli na re-connect
        this->address = string{};
        this->port = 0;
}

/*****************************************************************************/

int Mc60Modem::send (const gsl::span<uint8_t> &data)
{
        if (dataToSendBuffer.size () + data.size () > dataToSendBuffer.max_size () /*|| !isTcpConnected ()*/) {
                return 0;
        }

        std::copy (data.cbegin (), data.cend (), std::back_inserter (dataToSendBuffer));
        return data.size ();
}
