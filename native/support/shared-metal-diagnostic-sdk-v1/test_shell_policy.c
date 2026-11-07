#include "shell_policy.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static MDGeometry geometry(void) {
    MDGeometry g = {1, 2, 390, 844, 3, 1170, 2532, 1, 1, 1};
    return g;
}

int main(void) {
    MDGeometry g = geometry(), b;
    MDPolicy p = {0};
    CHECK(md_geometry_valid(&g));
    CHECK(!md_geometry_valid(NULL));
    CHECK(!md_begin(NULL, &g));
    CHECK(!md_commit(NULL, &g));
    CHECK(!md_success(NULL));
    CHECK(!md_success(&p));
    CHECK(!md_commit(&p, &g));
    CHECK(!md_complete(&p, 1, 1));
    CHECK(!md_presented(&p, 1));
    /* Each malformed geometry also consumes the one allowed start attempt. */
    for (unsigned i = 0; i < 18; ++i) {
        b = g;
        switch (i) {
            case 0: b.scene = 0; break;
            case 1: b.window = 0; break;
            case 2: b.foreground = 0; break;
            case 3: b.attached = 0; break;
            case 4: b.points_w = NAN; break;
            case 5: b.points_h = INFINITY; break;
            case 6: b.scale = NAN; break;
            case 7: b.scale = 0; break;
            case 8: b.scale = 5; break;
            case 9: b.points_w = 0; break;
            case 10: b.points_h = -1; break;
            case 11: b.pixels_w = 0; break;
            case 12: b.pixels_h = 8193; break;
            case 13: b.orientation = 0; break;
            case 14: b.orientation = 5; break;
            case 15: ++b.pixels_w; break;
            case 16: ++b.pixels_h; break;
            case 17: b.points_w = b.points_h = 8192; b.scale = 1;
                b.pixels_w = b.pixels_h = 8192; break;
        }
        memset(&p, 0, sizeof p);
        CHECK(!md_geometry_valid(&b));
        CHECK(!md_begin(&p, &b));
        CHECK(p.failed && p.begun);
        CHECK(!md_begin(&p, &g));
    }
    for (unsigned i = 0; i < 7; ++i) {
        memset(&p, 0, sizeof p);
        CHECK(md_begin(&p, &g));
        b = g;
        switch (i) {
            case 0: ++b.scene; break;
            case 1: ++b.window; break;
            case 2: b.orientation = 2; break;
            case 3: b.points_w = 400; b.pixels_w = 1200; break;
            case 4: b.points_h = 800; b.pixels_h = 2400; break;
            case 5: b.scale = 2; b.pixels_w = 780; b.pixels_h = 1688; break;
            case 6: b.foreground = 0; break;
        }
        CHECK(!md_commit(&p, &b));
        CHECK(p.cancelled && !p.committed);
        CHECK(!md_commit(&p, &g));
    }
    /* Both legitimate callback orders; neither individual event proves success. */
    for (unsigned order = 0; order < 2; ++order) {
        memset(&p, 0, sizeof p);
        CHECK(md_begin(&p, &g));
        CHECK(md_commit(&p, &g));
        CHECK(!md_commit(&p, &g));
        if (order) CHECK(md_presented(&p, 123.5));
        else CHECK(md_complete(&p, 1, 1));
        CHECK(!md_success(&p));
        if (order) CHECK(md_complete(&p, 1, 1));
        else CHECK(md_presented(&p, 123.5));
        CHECK(md_success(&p));
        CHECK(!md_complete(&p, 0, 0));
        CHECK(!md_presented(&p, 0));
        CHECK(md_success(&p));
        CHECK(!md_begin(&p, &g));
    }
    /* Cancellation before commit, between callbacks, and after both: no restart. */
    for (unsigned when = 0; when < 4; ++when) {
        memset(&p, 0, sizeof p);
        CHECK(md_begin(&p, &g));
        if (when) CHECK(md_commit(&p, &g));
        if (when >= 2) CHECK(md_complete(&p, 1, 1));
        if (when == 3) CHECK(md_presented(&p, 1));
        md_cancel(&p);
        CHECK(p.cancelled);
        CHECK(!md_commit(&p, &g));
        if (when == 1) CHECK(md_complete(&p, 1, 1));
        if (when == 1 || when == 2) CHECK(md_presented(&p, 1));
        CHECK(!md_success(&p));
        CHECK(!md_begin(&p, &g));
    }
    for (unsigned failure = 0; failure < 6; ++failure) {
        memset(&p, 0, sizeof p);
        CHECK(md_begin(&p, &g));
        CHECK(md_commit(&p, &g));
        CHECK(md_complete(&p, failure != 0, failure != 1));
        double t = failure == 2 ? 0 : failure == 3 ? -1 : failure == 4 ? NAN : failure == 5 ? INFINITY : 1;
        CHECK(md_presented(&p, t));
        CHECK(!md_success(&p));
        CHECK(p.failed);
    }
    memset(&p, 0, sizeof p);
    CHECK(md_begin(&p, &g));
    CHECK(md_commit(&p, &g));
    md_fail(&p); /* Deadline/unobserved result cannot turn into success later. */
    CHECK(md_complete(&p, 1, 1));
    CHECK(md_presented(&p, 1));
    CHECK(!md_success(&p));

    uint8_t bytes[64 * 256];
    memset(bytes, 0xA5, sizeof bytes);
    CHECK(!md_pixels_match(bytes, sizeof bytes));
    const size_t center = 32u * 256u + 32u * 4u;
    const uint8_t blue[4] = {255, 0, 0, 255}, green[4] = {0, 255, 0, 255};
    memcpy(bytes, blue, 4); memcpy(bytes + center, green, 4);
    CHECK(md_pixels_match(bytes, sizeof bytes));
    CHECK(!md_pixels_match(NULL, sizeof bytes));
    CHECK(!md_pixels_match(bytes, sizeof bytes - 1));
    CHECK(!md_pixels_match(bytes, sizeof bytes + 1));
    for (unsigned i = 0; i < 8; ++i) {
        size_t at = i < 4 ? i : center + i - 4;
        bytes[at] ^= 1;
        CHECK(!md_pixels_match(bytes, sizeof bytes));
        bytes[at] ^= 1;
    }
    printf("PASS %u checks; pure owned policy only; no UIKit or GPU executed\n", checks);
    return 0;
}
