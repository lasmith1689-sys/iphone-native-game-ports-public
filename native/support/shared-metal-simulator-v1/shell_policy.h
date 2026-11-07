#ifndef MD_SHELL_POLICY_H
#define MD_SHELL_POLICY_H
#include <stddef.h>
#include <stdint.h>

/* Owned diagnostic policy. All calls belong to one main-thread lifetime. */
typedef struct {
    uintptr_t scene, window;
    double points_w, points_h, scale;
    uint32_t pixels_w, pixels_h;
    int orientation, foreground, attached;
} MDGeometry;

typedef struct {
    MDGeometry geometry;
    int begun, committed, cancelled, failed;
    int completion_observed, gpu_completed, pixels_matched;
    int presentation_observed, presentation_valid;
    double presented_time;
} MDPolicy;

int md_geometry_valid(const MDGeometry *g);
int md_geometry_equal(const MDGeometry *a, const MDGeometry *b);
int md_begin(MDPolicy *p, const MDGeometry *g);
/* Checks geometry on the same main-thread turn immediately before commit. */
int md_commit(MDPolicy *p, const MDGeometry *g);
void md_cancel(MDPolicy *p);
void md_fail(MDPolicy *p);
int md_complete(MDPolicy *p, int gpu_completed, int pixels_matched);
int md_presented(MDPolicy *p, double presented_time);
int md_success(const MDPolicy *p);
/* Exactly 64 x 64 BGRA8Unorm, 256 bytes per row; no tolerance or sRGB. */
int md_pixels_match(const uint8_t *bytes, size_t byte_count);
#endif
