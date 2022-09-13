#include <split/h3.h>
#include <assert.h>

void add_linked_geo_loop(LinkedGeoPolygon* polygon, LinkedGeoLoop* loop) {
    LinkedGeoLoop* last = polygon->last;
    if (!last) {
        assert(!polygon->first);
        polygon->first = loop;
    } else {
        last->next = loop;
    }
    polygon->last = loop;
}


void add_linked_latlng(LinkedGeoLoop* loop, LinkedLatLng* latlng) {
    LinkedLatLng* last = loop->last;
    if (!last) {
        assert(!loop->first);
        loop->first = latlng;
    } else {
        last->next = latlng;
    }
    loop->last = latlng;
}


void free_linked_geo_polygon(LinkedGeoPolygon* polygon) {
    while (polygon) {
        LinkedGeoPolygon* next_polygon = polygon->next;

        /* Free loops */
        LinkedGeoLoop* loop = polygon->first;
        while (loop) {
            LinkedGeoLoop* next_loop = loop->next;
            free_linked_geo_loop(loop);
            loop = next_loop;
        }

        free(polygon);
        polygon = next_polygon;
    }
}


void free_linked_geo_loop(LinkedGeoLoop* loop) {
    LinkedLatLng* point = loop->first;
    while (point) {
        LinkedLatLng* next = point->next;
        free(point);
        point = next;
    }
    free(loop);
    // no recursion
}
