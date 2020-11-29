#include "DataItem.h"
#include "Constants.h"
struct Bucket
{
    DataItem data[ITEMS_PER_BUCKET];
    int localDepth;
};
