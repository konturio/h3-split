#pragma once

#include <h3/h3api.h>

void add_linked_geo_loop(LinkedGeoPolygon* polygon, LinkedGeoLoop* loop);

void add_linked_latlng(LinkedGeoLoop* loop, LinkedLatLng* latlng);

void free_linked_geo_polygon(LinkedGeoPolygon* polygon);

void free_linked_geo_loop(LinkedGeoLoop* loop);
