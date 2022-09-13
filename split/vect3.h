#pragma once

#include <stdbool.h>
#include <h3/h3api.h>

typedef struct Vect3 {
    double x;
    double y;
    double z;
} Vect3;

void vect3_from_lat_lng(const LatLng *coord, Vect3 *vect);

void vect3_to_lat_lng(const Vect3 *vect, LatLng *coord);

bool vect3_eq(const Vect3* v1, const Vect3* v2);

void vect3_normalize(Vect3 *vect);

void vect3_sum(const Vect3 *vect1, const Vect3 *vect2, Vect3 *sum);

void vect3_diff(const Vect3 *vect1, const Vect3 *vect2, Vect3 *diff);

void vect3_cross(const Vect3 *vect1, const Vect3 *vect2, Vect3 *prod);

double vect3_dot(const Vect3 *vect1, const Vect3 *vect2);

void vect3_scale(Vect3 *vect, double fact);
