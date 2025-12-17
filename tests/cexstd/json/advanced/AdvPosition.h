#pragma once
#include "cex.h"
#include "cexstd/json/json.h"
#include "AdvStock.h"

json$$struct();
typedef struct Position_c {
    i32 qty;
    f32 fill_price;
    Stock_c* stock;
} Position_c;
