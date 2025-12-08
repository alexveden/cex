#pragma once
#include "cex.h"
#include "lib/json/json.h"

/// Hi this is my comment
serde$$struct();
typedef struct Stock
{
    u64 id;
    serde$$field(.nullable = true, .optional = true);
    char* ticker;
    serde$$field(.nullable = true);
    char* exchange;
} Stock;


serde$$struct();
typedef struct ItemNullable
{
    serde$$field(.nullable = true);
    sbuf_c sbuf_field;
    serde$$field(.nullable = true);
    str_s str_s_field;
    serde$$field(.nullable = true);
    char* char_field;
    serde$$field(.nullable = true);
    Stock* stock_field;
    serde$$field(.nullable = true);
    Stock stock_val;
} ItemNullable;

serde$$struct();
typedef struct Item
{
    sbuf_c sbuf_field;
    str_s str_s_field;
    char* char_field;
    Stock* stock_field;

    serde$$field(.skip = true);
    Stock* stock_field_skipped;
} Item;

serde$$struct();
typedef struct Order
{
    u64 id;
    f32 price;
    i32 qty;
    bool is_active;
    char* exchange;
    Stock* stock;
} Order;
