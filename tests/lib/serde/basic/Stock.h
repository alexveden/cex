#pragma once
#include "cex.h"

/// Hi this is my comment
serde$$struct(.name = "MyStock")
typedef struct Stock {
    u64 id;
    char* ticker;
    char* exchange;

    serde$$field(.name = "my_json_name", .skip = true)
    char* my_field;
} Stock;


