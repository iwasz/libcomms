/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
//#include <etl/vector.h>
#include <cstdint>
#include <vector>
//#include <variant>

// static constexpr size_t BINARY_EVENT_MAX_SIZE = 128;
// using UintVector = etl::vector<uint8_t, BINARY_EVENT_MAX_SIZE>;
using UintVector = std::vector<uint8_t>;

struct BinaryEvent : public UintVector {
        using UintVector::UintVector;
        //static constexpr size_t MAX_SIZE = BINARY_EVENT_MAX_SIZE;

        const char *argStr = nullptr;
        int argInt1 = 0;
        int argInt2 = 0;
};

// struct BinaryEvent {
//        using PayloadType = etl::vector<uint8_t, 64>;

//        size_t size () const { return payload.size (); }

//        PayloadType::value_type &at (size_t i) { return payload.at (i); }
//        PayloadType::value_type const &at (size_t i) const { return payload.at (i); }

//        PayloadType::pointer data () { return payload.data (); }
//        PayloadType::const_pointer data () const { return payload.data (); }

//        void resize (size_t s) { payload.resize (s); }
//        void begin (size_t s) { payload.resize (s); }

//        PayloadType payload;

//        const char *argStr = nullptr;
//        int argInt1 = 0;
//        int argInt2 = 0;
//};
