#pragma once
#include "cex.h"
#include "lib/json/json.h"

serde$$struct()
/// Hi this is my comment
typedef struct Stock {
    u64 id;
    char* ticker;
    char* exchange;
} Stock;


serde$$struct()
typedef struct ItemNullable {
    sbuf_c sbuf_field;
    str_s str_s_field;
    char* char_field;
    Stock* stock_field;
} ItemNullable;
