/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "SendNetworkAction.h"
#include "Usart.h"
#include "collection/CircularBuffer.h"
//#include "config.h"
#include <cstring>
//#include "string_utils.h"

extern Usart *modemUsart;

#if 0
bool SendNetworkAction::run (const string &)
{
#ifndef UNIT_TEST
        /*
         * Przygotowuje i wysyła taką "preambułę", która mówi ile bajtów będzie
         * wysyłanych w następnym kroku.
         */
        if (stage == STAGE_PREPARE) {
                *bytesToSendInSendStage = MAX_BYTES_SEND;

                //                if (MAX_BYTES_SEND < *bytesToSendInSendStage) {
                //                        *bytesToSendInSendStage = MAX_BYTES_SEND;
                //                }

                if (outputBuffer->size () < *bytesToSendInSendStage) {
                        *bytesToSendInSendStage = outputBuffer->size ();
                }

                if (*bytesToSendInSendStage == 0) {
                        return true;
                }

                // TODO zamienić na jakąś mniej pamięciożerną funkcję niż snprintf ?
                static char command[MAX_BYTES_SEND];
                snprintf (command, MAX_BYTES_SEND, "AT+CIPSEND=0,%lu\r\n", *bytesToSendInSendStage);
                modemUsart->transmit (command);
#if 0
                debug->log (GSM_SENT_USART, GSM_SENT_USART_T, command);
#endif
        }
        /*
         * Wysyłanie zadeklarowanej w poprzednim kroku liczby bajtów.
         */
        else if (stage == STAGE_SEND) {
                if (*bytesToSendInSendStage == 0) {
                        return true;
                }

                // debug->log (GSM_SENT_TO_SERVER_B, GSM_SENT_TO_SERVER_B_T, bytesToSendInSendStage);
                // gsmObject->sendBuffer (*bytesToSendInSendStage);

                uint8_t *partA, *partB;
                size_t lenA, lenB;
                outputBuffer->retrieve (&partA, &lenA, &partB, &lenB, *bytesToSendInSendStage);

                if (partA && lenA) {
                        modemUsart->transmit (partA, lenA);
                }

                if (partB && lenB) {
                        modemUsart->transmit (partB, lenB);
                }
        }
        /*
         * Zdjęcie wysłanej liczby bajtów z bufora.
         */
        else if (stage == STAGE_DECLARE) {
                outputBuffer->declareRead (*bytesToSendInSendStage);
                // debug->log (GSM_DECLARED_SEND_B, GSM_DECLARED_SEND_B_T, bytesToSendInSendStage);
        }
#endif
        return true;
}

/*****************************************************************************/

uint32_t SendNetworkAction::parseCipsendRespoinse (const char *input)
{
        /*
            const char *b = strstr (input, "+QISEND:");

            if (!b) {
                    return 0;
            }

            b += 8;

            // Bug w modemie M66 powoduje, że komenda QISEND=? zwraca napis <length> zamiast prawdziwego rozmiaru
            const char *c = strstr (input, "<length>");

            if (c) {
                    return GSM_MAX_BYTES_SEND;
            }

            return strtoul (b, nullptr, 10);
            */

        // Wg dokumentacji BG96 to jest zawszze 1460
        return 1460;
}

#endif

bool SendTransparentAction::run (const string &)
{

        /*
         * Wysyłanie zadeklarowanej w poprzednim kroku liczby bajtów.
         */
        //        if (*bytesToSendInSendStage == 0) {
        //                return true;
        //        }

        // debug->log (GSM_SENT_TO_SERVER_B, GSM_SENT_TO_SERVER_B_T, bytesToSendInSendStage);
        // gsmObject->sendBuffer (*bytesToSendInSendStage);

        uint8_t *partA, *partB;
        size_t lenA, lenB;
        outputBuffer.retrieve (&partA, &lenA, &partB, &lenB, 2048);

        if (partA && lenA) {
                modemUsart->transmit (partA, lenA);
        }

        if (partB && lenB) {
                modemUsart->transmit (partB, lenB);
        }

        outputBuffer.declareReadAll ();
        return true;
}
