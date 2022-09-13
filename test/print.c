#include "print.h"
#include <float.h>
#include <stdio.h>

void print_nl() {
    printf("\n");
}

void print_latlng(const LatLng* latlng) {
    printf("(");
    print_double(radsToDegs(latlng->lng));
    printf(" ");
    print_double(radsToDegs(latlng->lat));
    printf(")");
}

void print_vect3(const Vect3* vect) {
    printf("(");
    print_double(vect->x);
    printf(", ");
    print_double(vect->y);
    printf(", ");
    print_double(vect->z);
    printf(")");
}

void print_bbox3(const Bbox3* bbox) {
    printf("((");
    print_double(bbox->xmin);
    printf(", ");
    print_double(bbox->ymin);
    printf(", ");
    print_double(bbox->zmin);
    printf("), (");
    print_double(bbox->xmax);
    printf(", ");
    print_double(bbox->ymax);
    printf(", ");
    print_double(bbox->zmax);
    printf("))");
}

void print_bbox3_latlng(const Bbox3* bbox) {
    Vect3 v1, v2;
    v1.x = bbox->xmin;
    v1.y = bbox->ymin;
    v1.z = bbox->zmin;
    v2.x = bbox->xmax;
    v2.y = bbox->ymax;
    v2.z = bbox->zmax;

    LatLng latlng1, latlng2;
    vect3_to_lat_lng(&v1, &latlng1);
    vect3_to_lat_lng(&v2, &latlng2);

    printf("((");
    print_double(radsToDegs(latlng1.lng));
    printf(", ");
    print_double(radsToDegs(latlng1.lat));
    printf("), (");
    print_double(radsToDegs(latlng2.lng));
    printf(", ");
    print_double(radsToDegs(latlng2.lat));
    printf("))");
}

void print_double(double value) {
    printf("%.*f", DECIMAL_DIG, value);
    /* printf("%f", value); */
}
