#pragma once
#include "cex.h"
#include "lib/json/json.h"

/// Hi this is my comment
json$$struct();
typedef struct Stock
{
    u64 id;
    json$$field(.nullable = true, .optional = true);
    char* ticker;
    json$$field(.nullable = true);
    char* exchange;
} Stock;


json$$struct();
typedef struct ItemNullable
{
    json$$field(.nullable = true);
    sbuf_c sbuf_field;
    json$$field(.nullable = true);
    str_s str_s_field;
    json$$field(.nullable = true);
    char* char_field;
    json$$field(.nullable = true);
    Stock* stock_field;
    json$$field(.nullable = true);
    Stock stock_val;
} ItemNullable;

json$$struct();
typedef struct Item
{
    sbuf_c sbuf_field;
    str_s str_s_field;
    char* char_field;
    Stock* stock_field;

    json$$field(.skip = true);
    Stock* stock_field_skipped;
} Item;

json$$struct();
typedef struct Order
{
    u64 id;
    f32 price;
    i32 qty;
    bool is_active;
    char* exchange;
    Stock* stock;
} Order;
