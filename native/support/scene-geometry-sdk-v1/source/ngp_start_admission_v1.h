#ifndef NGP_START_ADMISSION_V1_H
#define NGP_START_ADMISSION_V1_H

enum {
    NGP_START_WAIT, NGP_START_READY, NGP_START_EXPIRED, NGP_START_FINISHED,
    NGP_START_MAX_POLLS = 100,
    NGP_START_STABLE_POLLS = 3
};

typedef struct {
    unsigned polls;
    unsigned stable;
    double width;
    double height;
    int finished;
} NGPStartAdmissionV1;

static inline int ngp_start_admission_v1_poll(NGPStartAdmissionV1 *gate, int ready,
                                             double width, double height) {
    if (gate->finished) return NGP_START_FINISHED;
    ++gate->polls;
    if (ready && width > height && height > 0 && width <= 16384) {
        gate->stable = gate->width == width && gate->height == height ? gate->stable + 1 : 1;
        gate->width = width;
        gate->height = height;
        if (gate->stable == NGP_START_STABLE_POLLS) {
            gate->finished = 1;
            return NGP_START_READY;
        }
    } else {
        gate->stable = 0;
    }
    if (gate->polls == NGP_START_MAX_POLLS) {
        gate->finished = 1;
        return NGP_START_EXPIRED;
    }
    return NGP_START_WAIT;
}
#endif
