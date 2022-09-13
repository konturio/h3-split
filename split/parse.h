#pragma once

#include <stddef.h>
#include <h3/h3api.h>
#include <split/types.h>

typedef enum {
    WktParseError_Ok = 0,
    WktParseError_TypeExpected,
    WktParseError_InvalidType,
    WktParseError_LeftParenExpected,
    WktParseError_RightParenExpected,
    WktParseError_CommaExpected,
    WktParseError_NumberExpected,
    WktParseError_InvalidNumber,
    WktParseError_MemAllocFailed
} WktParseError;

typedef struct {
    WktParseError error;
    H3Type type;
    void* object;
    size_t error_pos;
} WktParseResult;

WktParseResult wkt_parse(const char* wkt, size_t len);

const char* wkt_parse_error_to_string(WktParseError error);
