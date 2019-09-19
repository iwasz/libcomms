/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <gsl/gsl>

/**
 * For now I think this interface will represent a TCP capable device with AT-commands interface
 * (TODO so name of this should reflect that in some way, now its to broad).
 */
struct ICommunicationInterface {

        struct Callback {
                enum class Error : uint8_t { NO_ERROR };

                virtual ~Callback () = default;
                // TODO add "ICommunicationInterface &owner" to every method as a first argument for convenience.
                // virtual void onConnected (int connectionId) = 0;
                // virtual void onDisconnected (int connectionId) = 0;
                // virtual void onData (gsl::span<uint8_t const> data) = 0;
                virtual void onError (Error error) = 0;
        };

        ICommunicationInterface (Callback *c = nullptr) : callback (c) {}
        virtual ~ICommunicationInterface () = default;

        virtual bool connect (const char *address, uint16_t port) = 0; /// TCP connection
        virtual bool isConnected () const = 0;                         /// Is TCP connection present.
        virtual void disconnect (int connectionId) = 0;

        virtual int send (gsl::span<uint8_t> data) = 0;
        virtual size_t read (gsl::span<uint8_t> outBuf) = 0;
        virtual bool hasData () const = 0;
        //        virtual size_t peek (gsl::span<uint8_t> outBuf) = 0;
        //        virtual size_t declare (size_t bytes) = 0;

        using DataBuffer = std::deque<uint8_t>;
        virtual DataBuffer &getDataBuffer () = 0;
        virtual DataBuffer const &getDataBuffer () const = 0;

        virtual void run () = 0;

        void setCallback (Callback *c) { callback = c; }

protected:
        Callback *callback;
};
