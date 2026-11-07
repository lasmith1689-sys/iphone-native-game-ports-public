#ifndef NGP_SCENE_GEOMETRY_V1_H
#define NGP_SCENE_GEOMETRY_V1_H

#include <math.h>
#include <stddef.h>

enum { NGP_SCENE_RECT_COUNT_V1 = 8, NGP_SCENE_SCALE_COUNT_V1 = 5 };

typedef struct {
    long orientation;
    double rect[NGP_SCENE_RECT_COUNT_V1][4];
    double scale[NGP_SCENE_SCALE_COUNT_V1];
} NgpSceneGeometryV1;

static inline int ngp_scene_geometry_unchanged_v1(const NgpSceneGeometryV1 *initial,
    const NgpSceneGeometryV1 *current, long previous_orientation, double previous_scale) {
    if (initial->orientation <= 0 || initial->orientation != previous_orientation ||
        initial->orientation != current->orientation || previous_scale != initial->scale[0]) return 0;
    for (size_t r = 0; r < NGP_SCENE_RECT_COUNT_V1; ++r) {
        if (initial->rect[r][2] <= 0 || initial->rect[r][3] <= 0) return 0;
        for (size_t c = 0; c < 4; ++c)
            if (!isfinite(initial->rect[r][c]) || initial->rect[r][c] != current->rect[r][c]) return 0;
    }
    for (size_t i = 0; i < NGP_SCENE_SCALE_COUNT_V1; ++i)
        if (!isfinite(initial->scale[i]) || initial->scale[i] <= 0 ||
            initial->scale[i] != current->scale[i]) return 0;
    return 1;
}

#endif
