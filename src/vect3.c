#include <split/vect3.h>
#include <float.h>
#include <math.h>

#define FP_EQUAL(v1, v2) ((v1) == (v2) || fabs((v1) - (v2)) < DBL_EPSILON)

void
vect3_from_lat_lng(const LatLng *coord, Vect3 *vect)
{
    vect->x = cos(coord->lat) * cos(coord->lng);
    vect->y = cos(coord->lat) * sin(coord->lng);
    vect->z = sin(coord->lat);
}

void
vect3_to_lat_lng(const Vect3 *vect, LatLng *coord)
{
    coord->lng = atan2(vect->y, vect->x);
    coord->lat = asin(vect->z);
}

bool
vect3_eq(const Vect3* v1, const Vect3* v2)
{
    return FP_EQUAL(v1->x, v2->x)
        && FP_EQUAL(v1->y, v2->y)
        && FP_EQUAL(v1->z, v2->z);
}

void
vect3_normalize(Vect3 *vect)
{
    double len = sqrt(
        vect->x * vect->x
        + vect->y * vect->y
        + vect->z * vect->z);
    if (len > 0) {
        vect->x = vect->x / len;
        vect->y = vect->y / len;
        vect->z = vect->z / len;
    } else {
        vect->x = 0;
        vect->y = 0;
        vect->z = 0;
    }
}

void
vect3_sum(const Vect3 *vect1, const Vect3 *vect2, Vect3 *sum)
{
    sum->x = vect1->x + vect2->x;
    sum->y = vect1->y + vect2->y;
    sum->z = vect1->z + vect2->z;
}

void
vect3_diff(const Vect3 *vect1, const Vect3 *vect2, Vect3 *diff)
{
    diff->x = vect1->x - vect2->x;
    diff->y = vect1->y - vect2->y;
    diff->z = vect1->z - vect2->z;
}

void
vect3_cross(const Vect3 *vect1, const Vect3 *vect2, Vect3 *prod)
{
    prod->x = vect1->y * vect2->z - vect1->z * vect2->y;
    prod->y = vect1->z * vect2->x - vect1->x * vect2->z;
    prod->z = vect1->x * vect2->y - vect1->y * vect2->x;
}


double
vect3_dot(const Vect3 *vect1, const Vect3 *vect2)
{
    return (vect1->x * vect2->x) + (vect1->y * vect2->y) + (vect1->z * vect2->z);
}

void
vect3_scale(Vect3 *vect, double fact)
{
    vect->x *= fact;
    vect->y *= fact;
    vect->z *= fact;
}
