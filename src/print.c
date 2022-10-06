#include <split/print.h>
#include <assert.h>
#include <float.h>
#include <stdio.h>

static const char WktPrintTypeNamePolygon[] = "POLYGON";
static const char WktPrintTypeNameMultipolygon[] = "MULTIPOLYGON";

static void print_polygon_data(const LinkedGeoPolygon* polygon);
static void print_ring(const LinkedGeoLoop* ring);
static void print_point(const LinkedLatLng* point);
static void print_double(double value);

void print_polygon(const LinkedGeoPolygon* polygon) {
    if (polygon->next) {
        printf("%s(", WktPrintTypeNameMultipolygon);
    } else {
        printf("%s", WktPrintTypeNamePolygon);
    }

    const LinkedGeoPolygon* cur = polygon;
    while (cur) {
        if (cur != polygon)
            printf(", ");
        print_polygon_data(cur);
        cur = cur->next;
    }

    if (polygon->next) {
        printf(")");
    }
}


void print_polygon_data(const LinkedGeoPolygon* polygon) {
    if (polygon->first) {
        printf("(");
        const LinkedGeoLoop* ring = polygon->first;
        while (ring) {
            if (ring != polygon->first)
                printf(", ");
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
        printf(", ");
        print_point(first);
    }
    printf(")");
}


void print_point(const LinkedLatLng* point) {
    print_double(radsToDegs(point->vertex.lng));
    printf(" ");
    print_double(radsToDegs(point->vertex.lat));
}


void print_double(double value) {
    /* printf("%.*f", DECIMAL_DIG, value); */
    printf("%f", value);
}
