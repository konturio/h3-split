#include "print.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>

static const char WktPrintTypeNamePolygon[] = "POLYGON";

static void print_ring(const LinkedGeoLoop* ring);
static void print_point(const LinkedLatLng* point);
static void print_double(double value);

void print_polygon(const LinkedGeoPolygon* polygon) {
    printf("%s", WktPrintTypeNamePolygon);
    if (polygon->first) {
        printf("(");
        const LinkedGeoLoop* ring = polygon->first;
        while (ring) {
            print_ring(ring);
            ring = ring->next;
        }
        printf(")");
    }
}


void print_ring(const LinkedGeoLoop* ring) {
    assert(ring->first);
    assert(ring->last);

    const LinkedLatLng* first = ring->first;
    const LinkedLatLng* last = ring->last;

    printf("(");
    const LinkedLatLng* point = first;
    while (point) {
        print_point(point);
        point = point->next;
        if (point)
            printf(", ");
    }
    /* Close ring */
    if (first->vertex.lng != last->vertex.lng
        || first->vertex.lat != last->vertex.lat)
    {
        print_point(first);
    }
    printf(")");
}


void print_point(const LinkedLatLng* point) {
    print_double(point->vertex.lng);
    printf(" ");
    print_double(point->vertex.lat);
}


void print_double(double value) {
    printf("%.*f", DECIMAL_DIG, value);
}
