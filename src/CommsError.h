/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "ErrorHandler.h"

static const int LIBCOMMS_LIBRARY_CODE = 0x0100000;
enum { QUIRDI_PARSE_SIZE_GT = LAYER_MASK & LIBCOMMS_LIBRARY_CODE, PARSE_RECV_SIZE_GT };

static const char *LIBCOMMS_LIBRARY_EXCEPTION[] = {"QUIRDI_PARSE_SIZE_GT", "PARSE_RECV_SIZE_GT"};
