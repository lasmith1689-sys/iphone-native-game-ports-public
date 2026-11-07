#include "shell_policy.h"
#include <math.h>

int md_geometry_valid(const MDGeometry *g) {
    if (!g || !g->scene || !g->window || g->foreground != 1 || g->attached != 1)
        return 0;
    if (!isfinite(g->points_w) || !isfinite(g->points_h) || !isfinite(g->scale) ||
        g->points_w <= 0 || g->points_h <= 0 || g->scale < 1 || g->scale > 4 ||
        !g->pixels_w || !g->pixels_h || g->pixels_w > 8192 || g->pixels_h > 8192 ||
        (uint64_t)g->pixels_w * g->pixels_h > UINT64_C(33554432) ||
        g->orientation < 1 || g->orientation > 4)
        return 0;
    return fabs(g->points_w * g->scale - g->pixels_w) <= 0.01 &&
           fabs(g->points_h * g->scale - g->pixels_h) <= 0.01;
}

int md_geometry_equal(const MDGeometry *a, const MDGeometry *b) {
    return md_geometry_valid(a) && md_geometry_valid(b) &&
        a->scene == b->scene && a->window == b->window &&
        a->points_w == b->points_w && a->points_h == b->points_h &&
        a->scale == b->scale && a->pixels_w == b->pixels_w &&
        a->pixels_h == b->pixels_h && a->orientation == b->orientation;
}

int md_begin(MDPolicy *p, const MDGeometry *g) {
    if (!p || p->begun || p->cancelled || p->failed) return 0;
    p->begun = 1; /* A refused attempt also consumes this lifetime. */
    if (!md_geometry_valid(g)) { p->failed = 1; return 0; }
    p->geometry = *g;
    return 1;
}

int md_commit(MDPolicy *p, const MDGeometry *g) {
    if (!p || !p->begun || p->committed || p->cancelled || p->failed) return 0;
    if (!md_geometry_equal(&p->geometry, g)) { p->cancelled = 1; return 0; }
    p->committed = 1;
    return 1;
}

void md_cancel(MDPolicy *p) { if (p && p->begun) p->cancelled = 1; }
void md_fail(MDPolicy *p) { if (p) p->failed = 1; }

int md_complete(MDPolicy *p, int gpu_completed, int pixels_matched) {
    if (!p || !p->committed || p->completion_observed) return 0;
    p->completion_observed = 1;
    p->gpu_completed = gpu_completed == 1;
    p->pixels_matched = p->gpu_completed && pixels_matched == 1;
    if (!p->gpu_completed || !p->pixels_matched) p->failed = 1;
    return 1;
}

int md_presented(MDPolicy *p, double presented_time) {
    if (!p || !p->committed || p->presentation_observed) return 0;
    p->presentation_observed = 1;
    p->presented_time = presented_time;
    p->presentation_valid = isfinite(presented_time) && presented_time > 0;
    if (!p->presentation_valid) p->failed = 1;
    return 1;
}

int md_success(const MDPolicy *p) {
    return p && p->committed && !p->cancelled && !p->failed &&
        p->completion_observed && p->gpu_completed && p->pixels_matched &&
        p->presentation_observed && p->presentation_valid;
}

int md_pixels_match(const uint8_t *b, size_t n) {
    if (!b || n != 64u * 256u) return 0;
    const size_t center = 32u * 256u + 32u * 4u;
    /* Only two interior samples are asserted, not an image-wide oracle. */
    return b[0] == 255 && b[1] == 0 && b[2] == 0 && b[3] == 255 &&
        b[center] == 0 && b[center+1] == 255 && b[center+2] == 0 && b[center+3] == 255;
}
