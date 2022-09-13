#pragma once

#include <stdbool.h>
#include <h3/h3api.h>

typedef struct Vect3 Vect3;

typedef struct Bbox3 {
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double zmin;
    double zmax;
} Bbox3;

void bbox3_from_vect3(Bbox3* bbox, const Vect3* vect);

void bbox3_merge(Bbox3* bbox, const Bbox3* other);

void bbox3_from_linked_loop(Bbox3* bbox, const LinkedGeoLoop* loop);

bool bbox3_contains_vect3(const Bbox3* bbox, const Vect3* vect);
bool bbox3_contains_latlng(const Bbox3* bbox, const LatLng* latlng);

void bbox3_from_segment_vect3(Bbox3* bbox, const Vect3* v1, const Vect3* v2);
void bbox3_from_segment_latlng(Bbox3* bbox, const LatLng* v1, const LatLng* v2);
