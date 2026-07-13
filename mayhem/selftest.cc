// Authored known-answer/behavioral oracle for nanort + the nanosg example parsers.
// Upstream ships no assertion-based test suite (test/regression is a demo program), so this
// supplements it with known-answer checks. Prints one "PASS <name>" / "FAIL <name>" line per
// check; exits non-zero iff any check fails.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "nanort.h"
#include "obj-loader.h"
#include "render-config.h"

static int g_pass = 0, g_fail = 0;

static void check(bool ok, const char *name) {
  if (ok) { g_pass++; printf("PASS %s\n", name); }
  else    { g_fail++; printf("FAIL %s\n", name); }
}

static bool near_f(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) < eps;
}

int main() {
  // 1) vnormalize known answer: (3,4,0) -> (0.6, 0.8, 0)
  {
    nanort::real3<float> v(3.0f, 4.0f, 0.0f);
    nanort::real3<float> n = nanort::vnormalize(v);
    check(near_f(n[0], 0.6f) && near_f(n[1], 0.8f) && near_f(n[2], 0.0f),
          "vnormalize_known_answer");
  }

  // 2) vcross known answer: x cross y = z
  {
    nanort::real3<float> x(1.0f, 0.0f, 0.0f), y(0.0f, 1.0f, 0.0f);
    nanort::real3<float> z = nanort::vcross(x, y);
    check(near_f(z[0], 0.0f) && near_f(z[1], 0.0f) && near_f(z[2], 1.0f),
          "vcross_known_answer");
  }

  // 3) BVH build + ray traverse hit: unit triangle in z=0 plane, ray straight down at (0.25,0.25)
  {
    float vertices[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    unsigned int faces[3] = {0, 1, 2};
    nanort::TriangleMesh<float> mesh(vertices, faces, sizeof(float) * 3);
    nanort::TriangleSAHPred<float> pred(vertices, faces, sizeof(float) * 3);
    nanort::BVHAccel<float> accel;
    nanort::BVHBuildOptions<float> opts;
    bool built = accel.Build(1, mesh, pred, opts);

    nanort::Ray<float> ray;
    ray.org[0] = 0.25f; ray.org[1] = 0.25f; ray.org[2] = 1.0f;
    ray.dir[0] = 0.0f;  ray.dir[1] = 0.0f;  ray.dir[2] = -1.0f;
    ray.min_t = 0.0f;   ray.max_t = 100.0f;
    nanort::TriangleIntersector<float, nanort::TriangleIntersection<float> >
        isector(vertices, faces, sizeof(float) * 3);
    nanort::TriangleIntersection<float> isect;
    bool hit = built && accel.Traverse(ray, isector, &isect);
    check(hit && near_f(isect.t, 1.0f) && near_f(isect.u, 0.25f) && near_f(isect.v, 0.25f),
          "bvh_traverse_hit_known_answer");
  }

  // 4) BVH ray miss: ray pointing away from the triangle must not hit
  {
    float vertices[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    unsigned int faces[3] = {0, 1, 2};
    nanort::TriangleMesh<float> mesh(vertices, faces, sizeof(float) * 3);
    nanort::TriangleSAHPred<float> pred(vertices, faces, sizeof(float) * 3);
    nanort::BVHAccel<float> accel;
    nanort::BVHBuildOptions<float> opts;
    bool built = accel.Build(1, mesh, pred, opts);

    nanort::Ray<float> ray;
    ray.org[0] = 0.25f; ray.org[1] = 0.25f; ray.org[2] = 1.0f;
    ray.dir[0] = 0.0f;  ray.dir[1] = 0.0f;  ray.dir[2] = 1.0f;
    ray.min_t = 0.0f;   ray.max_t = 100.0f;
    nanort::TriangleIntersector<float, nanort::TriangleIntersection<float> >
        isector(vertices, faces, sizeof(float) * 3);
    nanort::TriangleIntersection<float> isect;
    bool hit = built && accel.Traverse(ray, isector, &isect);
    check(built && !hit, "bvh_traverse_miss");
  }

  // 5) render-config JSON parsing known answer
  {
    const char *path = "/tmp/selftest_config.json";
    {
      std::ofstream os(path);
      os << "{\"obj_filename\": \"scene.obj\", \"scene_scale\": 2.5, \"eye\": [1.0, 2.0, 3.0]}";
    }
    example::RenderConfig config;
    bool ok = example::LoadRenderConfig(&config, path);
    check(ok && config.obj_filename == "scene.obj" && near_f(config.scene_scale, 2.5f) &&
              near_f(config.eye[0], 1.0f) && near_f(config.eye[1], 2.0f) && near_f(config.eye[2], 3.0f),
          "render_config_parse_known_answer");
  }

  // 6) obj-loader parsing known answer: one triangle, check mesh + vertex content
  {
    const char *path = "/tmp/selftest_tri.obj";
    {
      std::ofstream os(path);
      os << "v 0 0 0\nv 2 0 0\nv 0 2 0\nf 1 2 3\n";
    }
    std::vector<example::Mesh<float> > meshes;
    std::vector<example::Material> materials;
    std::vector<example::Texture> textures;
    bool ok = example::LoadObj(path, 0.5f, &meshes, &materials, &textures);
    bool content = ok && meshes.size() == 1 && meshes[0].faces.size() == 3 &&
                   meshes[0].vertices.size() >= 9;
    if (content) {
      // scale 0.5 + bbox recentering: vertex 1 (2,0,0) -> (0.5,-0.5,0)
      float vx = meshes[0].vertices[3 * meshes[0].faces[1] + 0];
      float vy = meshes[0].vertices[3 * meshes[0].faces[1] + 1];
      content = near_f(vx, 0.5f) && near_f(vy, -0.5f);
    }
    check(content, "obj_loader_parse_known_answer");
  }

  printf("SELFTEST passed=%d failed=%d\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
