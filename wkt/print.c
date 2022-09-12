#include "print.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>

static const char WktPrintTypeNamePolygon[] = "POLYGON";

static void print_ring(const LinkedGeoLoop* ring);
static void print_double(double value);

void print_polygon(const LinkedGeoPolygon* polygon) {
    printf("%s", WktPrintTypeNamePolygon);
    if (polygon->first) {
        printf("(");
        LinkedGeoLoop* ring = polygon->first;
        while (ring) {
            print_ring(ring);
            ring = ring->next;
        }
        printf(")");
    }
}


void print_ring(const LinkedGeoLoop* ring) {
    printf("(");
    assert(ring->first);
    LinkedLatLng* point = ring->first;
    while (point) {
        print_double(point->vertex.lng);
        printf(" ");
        print_double(point->vertex.lat);

        point = point->next;
        if (point)
            printf(", ");
    }
    printf(")");
}


void print_double(double value) {
    printf("%.*f", DECIMAL_DIG, value);
}
