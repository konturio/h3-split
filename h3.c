#include "h3.h"

void free_linked_geo_polygon(LinkedGeoPolygon* polygon) {
    LinkedGeoLoop* loop = polygon->first;
    while (loop) {
        LinkedGeoLoop* next = loop->next;
        free_linked_geo_loop(loop);
        loop = next;
    }
    free(polygon);
    // TODO: free polygon->next??
}

void free_linked_geo_loop(LinkedGeoLoop* loop) {
    LinkedLatLng* point = loop->first;
    while (point) {
        LinkedLatLng* next = point->next;
        free(point);
        point = next;
    }
    free(loop);
    // TODO: free loop->next??
}
