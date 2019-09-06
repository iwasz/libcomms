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

static constexpr size_t ESP_BUFFER_SIZE = 3 * 2048;
using buffer = etl::vector<uint8_t, ESP_BUFFER_SIZE>;
