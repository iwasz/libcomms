/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdlib>
#include <etl/vector.h>

static constexpr size_t OUTPUT_BUFFER_SIZE = 2048; /// Bufor na dane z µC do modemu.
using Buffer = etl::vector<uint8_t, OUTPUT_BUFFER_SIZE>;
