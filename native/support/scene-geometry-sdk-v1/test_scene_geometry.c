#include <assert.h>
#include <stdio.h>

#include "ngp_scene_geometry_v1.h"

int main(void) {
    NgpSceneGeometryV1 initial = {.orientation = 3};
    for (size_t i = 0; i < NGP_SCENE_RECT_COUNT_V1; ++i) {
        initial.rect[i][0] = (double)i - 4.0;
        initial.rect[i][2] = 804.0;
        initial.rect[i][3] = 402.0;
    }
    for (size_t i = 0; i < NGP_SCENE_SCALE_COUNT_V1; ++i) initial.scale[i] = 3.0;
    NgpSceneGeometryV1 current = initial;
    assert(ngp_scene_geometry_unchanged_v1(&initial, &current, 3, 3.0));
    assert(!ngp_scene_geometry_unchanged_v1(&initial, &current, 4, 3.0));
    assert(!ngp_scene_geometry_unchanged_v1(&initial, &current, 3, 2.0));
    current.orientation = 4;
    assert(!ngp_scene_geometry_unchanged_v1(&initial, &current, 3, 3.0));
    for (size_t r = 0; r < NGP_SCENE_RECT_COUNT_V1; ++r)
        for (size_t c = 0; c < 4; ++c) {
            current = initial;
            current.rect[r][c] += 0.5;
            assert(!ngp_scene_geometry_unchanged_v1(&initial, &current, 3, 3.0));
        }
    for (size_t i = 0; i < NGP_SCENE_SCALE_COUNT_V1; ++i) {
        current = initial;
        current.scale[i] = 2.0;
        assert(!ngp_scene_geometry_unchanged_v1(&initial, &current, 3, 3.0));
    }
    NgpSceneGeometryV1 unknown = {0};
    assert(!ngp_scene_geometry_unchanged_v1(&unknown, &unknown, 0, 0.0));
    current = initial;
    current.rect[0][0] = INFINITY;
    assert(!ngp_scene_geometry_unchanged_v1(&current, &current, 3, 3.0));
    current = initial;
    current.rect[1][0] = NAN;
    assert(!ngp_scene_geometry_unchanged_v1(&current, &current, 3, 3.0));
    current = initial;
    current.rect[2][2] = 0.0;
    assert(!ngp_scene_geometry_unchanged_v1(&current, &current, 3, 3.0));
    current = initial;
    current.scale[3] = 0.0;
    assert(!ngp_scene_geometry_unchanged_v1(&current, &current, 3, 3.0));
    puts("PASS scene geometry: identical, 45 refusal cases");
    return 0;
}
