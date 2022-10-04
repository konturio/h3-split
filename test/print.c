#include "print.h"
#include <float.h>
#include <stdio.h>

void print_nl() {
    printf("\n");
}


void print_latlng(const LatLng* latlng) {
    print_double(radsToDegs(latlng->lng));
    printf(" ");
    print_double(radsToDegs(latlng->lat));
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

void print_double(double value) {
    printf("%.*f", DECIMAL_DIG, value);
    /* printf("%f", value); */
}
