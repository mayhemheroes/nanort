#include <stdint.h>
#include <stdio.h>
#include <climits>

#include <fuzzer/FuzzedDataProvider.h>
#include "nanort.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    int xx = provider.ConsumeIntegral<int>();
    int yy = provider.ConsumeIntegral<int>();
    int zz = provider.ConsumeIntegral<int>();

    nanort::real3<int> r3(xx, yy, zz);
    nanort::vnormalize(r3);

    return 0;
}
