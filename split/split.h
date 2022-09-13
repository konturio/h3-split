#pragma once

#include <stdbool.h>
#include <h3/h3api.h>

bool is_crossed_by_180(const LinkedGeoPolygon* polygon);

LinkedGeoPolygon* split_by_180(const LinkedGeoPolygon* polygon);
