#pragma once
#include "cex.h"
#include "lib/json/json.h"
#include "Stock.h"

json$$struct();
typedef struct Position {
    i32 qty;
    f32 fill_price;
    Stock* stock;
} Position;
