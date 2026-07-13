// In-process libFuzzer harness for the nanosg example's input-parsing code paths
// (render-config JSON via picojson, wavefront .obj via tiny_obj_loader / obj-loader.cc).
// Replaces the old `/nanosg @@` CLI target: the nanosg binary is an X11/OpenGL GUI app
// that cannot open a display in a headless fuzzing container (it aborts before parsing
// any input, yielding 0 edges), so the same parsing code paths are driven in-process.
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "obj-loader.h"
#include "render-config.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char tmpl[] = "/tmp/fuzz_nanosg_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return 0;
    ssize_t w = write(fd, data, size);
    close(fd);
    if (w != static_cast<ssize_t>(size)) { unlink(tmpl); return 0; }

    example::RenderConfig config;
    example::LoadRenderConfig(&config, tmpl);

    std::vector<example::Mesh<float> > meshes;
    std::vector<example::Material> materials;
    std::vector<example::Texture> textures;
    example::LoadObj(tmpl, 1.0f, &meshes, &materials, &textures);

    for (size_t i = 0; i < textures.size(); i++) {
        delete[] textures[i].image;
    }

    unlink(tmpl);
    return 0;
}
