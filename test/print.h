#pragma once

#include <h3/h3api.h>
#include <split/bbox3.h>
#include <split/vect3.h>

void print_nl();

void print_latlng(const LatLng* latlng);

void print_vect3(const Vect3* vect);

void print_bbox3(const Bbox3* bbox);

void print_bbox3_latlng(const Bbox3* bbox);

void print_bbox3_polygon(const Bbox3* bbox);

void print_double(double value);
