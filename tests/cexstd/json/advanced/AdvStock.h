#pragma once
#include "cex.h"
#include "cexstd/json/json.h"

/// Hi this is my comment
json$$struct();
typedef struct Stock_c
{
    u64 id;
    json$$field(.nullable = true, .optional = true);
    char* ticker;
    json$$field(.nullable = true);
    char* exchange;
} Stock_c;


json$$struct();
typedef struct ItemNullable_c
{
    json$$field(.nullable = true);
    sbuf_c sbuf_field;
    json$$field(.nullable = true);
    str_s str_s_field;
    json$$field(.nullable = true);
    char* char_field;
    json$$field(.nullable = true);
    Stock_c* stock_field;
    json$$field(.nullable = true);
    Stock_c stock_val;
} ItemNullable_c;

json$$struct();
typedef struct Item_c
{
    sbuf_c sbuf_field;
    str_s str_s_field;
    char* char_field;
    Stock_c stock_field;

    json$$field(.skip = true);
    Stock_c* stock_field_skipped;
} Item;

json$$struct();
typedef struct Order
{
    u64 id;
    f32 price;
    i32 qty;
    bool is_active;
    char* exchange;
    Stock_c* stock;
} Order;

json$$struct();
typedef struct Order2_c
{
    json$$field(.name = "my_json_id");
    u64 id;
} Order2_c;
