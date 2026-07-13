#include <stdint.h>
#include <stdio.h>

#include <vector>

#include <fuzzer/FuzzedDataProvider.h>
#include "nanort.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);

    float fx = provider.ConsumeFloatingPointInRange<float>(-1000.0f, 1000.0f);
    float fy = provider.ConsumeFloatingPointInRange<float>(-1000.0f, 1000.0f);
    float fz = provider.ConsumeFloatingPointInRange<float>(-1000.0f, 1000.0f);
    nanort::real3<float> f3(fx, fy, fz);
    nanort::real3<float> n = nanort::vnormalize(f3);
    nanort::vcross(f3, n);
    nanort::vdot(f3, n);

    // Broadened coverage: build a small BVH from fuzzed triangles and trace fuzzed rays.
    size_t ntris = provider.ConsumeIntegralInRange<size_t>(1, 16);
    std::vector<float> vertices;
    std::vector<unsigned int> faces;
    for (size_t i = 0; i < ntris * 3; i++) {
        vertices.push_back(provider.ConsumeFloatingPointInRange<float>(-100.0f, 100.0f));
        vertices.push_back(provider.ConsumeFloatingPointInRange<float>(-100.0f, 100.0f));
        vertices.push_back(provider.ConsumeFloatingPointInRange<float>(-100.0f, 100.0f));
        faces.push_back(static_cast<unsigned int>(i));
    }

    nanort::BVHBuildOptions<float> build_options;
    nanort::TriangleMesh<float> mesh(vertices.data(), faces.data(), sizeof(float) * 3);
    nanort::TriangleSAHPred<float> pred(vertices.data(), faces.data(), sizeof(float) * 3);
    nanort::BVHAccel<float> accel;
    if (!accel.Build(static_cast<unsigned int>(ntris), mesh, pred, build_options)) {
        return 0;
    }

    nanort::Ray<float> ray;
    ray.org[0] = provider.ConsumeFloatingPointInRange<float>(-200.0f, 200.0f);
    ray.org[1] = provider.ConsumeFloatingPointInRange<float>(-200.0f, 200.0f);
    ray.org[2] = provider.ConsumeFloatingPointInRange<float>(-200.0f, 200.0f);
    nanort::real3<float> dir(
        provider.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f),
        provider.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f),
        provider.ConsumeFloatingPointInRange<float>(-1.0f, 1.0f));
    dir = nanort::vnormalize(dir);
    ray.dir[0] = dir[0];
    ray.dir[1] = dir[1];
    ray.dir[2] = dir[2];
    ray.min_t = 0.0f;
    ray.max_t = 1.0e+30f;

    nanort::TriangleIntersector<float, nanort::TriangleIntersection<float> >
        isector(vertices.data(), faces.data(), sizeof(float) * 3);
    nanort::TriangleIntersection<float> isect;
    accel.Traverse(ray, isector, &isect);

    return 0;
}
