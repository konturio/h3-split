#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <h3/h3api.h>
#include <split/bbox3.h>
#include <split/vect3.h>
#include "print.h"

static void check_bbox_contains(const Bbox3* bbox, const LatLng* latlng, bool answer);


int main() {
    LatLng latlng1;
    latlng1.lat = degsToRads(-5);
    latlng1.lng = degsToRads(-175);

    LatLng latlng2;
    latlng2.lat = degsToRads(5);
    latlng2.lng = degsToRads(175);

    Bbox3 bbox;
    bbox3_from_segment_latlng(&bbox, &latlng1, &latlng2);

    LatLng latlng;

    latlng.lat = degsToRads(1);
    latlng.lng = degsToRads(-174);
    check_bbox_contains(&bbox, &latlng, false);

    latlng.lng = degsToRads(-177);
    latlng.lat = degsToRads(3);
    check_bbox_contains(&bbox, &latlng, true);
}


void check_bbox_contains(const Bbox3* bbox, const LatLng* latlng, bool answer) {
    bool result = bbox3_contains_latlng(bbox, latlng);

    if (result != answer)
        printf("[fail] ");

    printf("bbox ");
    print_bbox3(bbox);
    printf(result ? " contains " : " does not contain ");
    print_latlng(latlng);
    printf("\n");

    if (result != answer)
        exit(EXIT_FAILURE);
}
