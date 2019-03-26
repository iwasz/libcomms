/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#ifndef ICOMMUNICATIONINTERFACE_H
#define ICOMMUNICATIONINTERFACE_H

#include <cstdint>
#include <cstdlib>

/**
 * For now I think this interface will represent a TCP capable device with AT-commands interface
 * (TODO so name of this should reflect that in some way, now its to broad).
 */
struct ICommunicationInterface {

        struct Callback {
                enum class Error : uint8_t { NO_ERROR };

                virtual ~Callback () = default;
                // TODO add "ICommunicationInterface &owner" to every method as a first argument for convenience.
                virtual void onConnected (int connectionId) = 0;
                virtual void onDisconnected (int connectionId) = 0;
                virtual void onData (uint8_t const *data, size_t len) = 0;
                virtual void onError (Error error) = 0;
        };

        ICommunicationInterface (Callback *c = nullptr) : callback (c) {}
        virtual ~ICommunicationInterface () = default;

        /// TCP connection
        virtual bool connect (const char *address, uint16_t port) = 0;
        virtual void disconnect (int connectionId) = 0;
        virtual int send (int connectionId, uint8_t *data, size_t len) = 0;
        virtual void run () = 0;

        void setCallback (Callback *c) { callback = c; }

protected:
        Callback *callback;
};

#endif // ICOMMUNICATIONINTERFACE_H
