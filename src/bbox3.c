#include <split/bbox3.h>
#include <math.h>
#include <string.h>
#include <split/vect3.h>

#define DEBUG 0
#if DEBUG
# include <stdio.h>
#endif

typedef struct {
    double x;
    double y;
} Vect2;

static void bbox3_merge_vect3(Bbox3* bbox, const Vect3* v1);

static void vect2_normalize(Vect2* vect);

static short vect2_segment_side(const Vect2* v1, const Vect2* v2, const Vect2* v);


void bbox3_from_vect3(Bbox3* bbox, const Vect3* vect) {
    bbox->xmin = bbox->xmax = vect->x;
    bbox->ymin = bbox->ymax = vect->y;
    bbox->zmin = bbox->zmax = vect->z;
}


void bbox3_merge(Bbox3* bbox, const Bbox3* other) {
    if (other->xmin < bbox->xmin) bbox->xmin = other->xmin;
    if (other->xmax > bbox->xmax) bbox->xmax = other->xmax;
    if (other->ymin < bbox->ymin) bbox->ymin = other->ymin;
    if (other->ymax > bbox->ymax) bbox->ymax = other->ymax;
    if (other->zmin < bbox->zmin) bbox->zmin = other->zmin;
    if (other->zmax > bbox->zmax) bbox->zmax = other->zmax;
}


void bbox3_from_linked_loop(Bbox3* bbox, const LinkedGeoLoop* loop) {
    const LinkedLatLng* cur = loop->first;
    const LinkedLatLng* next = cur->next;

    Vect3 vect;
    vect3_from_lat_lng(&cur->vertex, &vect);
    bbox3_from_vect3(bbox, &vect);

    if (!next) return;

    for (; cur != NULL; cur = cur->next, next = next->next ? next->next : loop->first) {
        Vect3 next_vect;
        vect3_from_lat_lng(&next->vertex, &next_vect);

        if (!vect3_eq(&vect, &next_vect)) {
            Bbox3 segment_bbox;
            bbox3_from_segment_vect3(&segment_bbox, &vect, &next_vect);
            bbox3_merge(bbox, &segment_bbox);
        }

        vect = next_vect;
    }
}


bool bbox3_contains_vect3(const Bbox3* bbox, const Vect3* vect) {
    return bbox->xmin <= vect->x && vect->x <= bbox->xmax
        && bbox->ymin <= vect->y && vect->y <= bbox->ymax
        && bbox->zmin <= vect->z && vect->z <= bbox->zmax;
}


bool bbox3_contains_latlng(const Bbox3* bbox, const LatLng* latlng) {
    Vect3 vect;
    vect3_from_lat_lng(latlng, &vect);
    return bbox3_contains_vect3(bbox, &vect);
}


void bbox3_from_segment_vect3(Bbox3* bbox, const Vect3* v1, const Vect3* v2) {
    /* Init bbox */
    bbox3_from_vect3(bbox, v1);
    bbox3_merge_vect3(bbox, v2);

    /* Check if points are the same */
    if (vect3_eq(v1, v2))
        return;

    /* Normal of plain containing v1/v2 */
    Vect3 vn;
    vect3_cross(v1, v2, &vn);
    vect3_normalize(&vn);

    /* Vector orthogonal to v1 in plane of v1/v2 */
    Vect3 v3;
    vect3_cross(&vn, v1, &v3);
    vect3_normalize(&v3);

    /* Project v1, v2 onto the plane, using v1/v3 as basis */
    Vect2 r1, r2;
    r1.x = 1.0;
    r1.y = 0.0;
    r2.x = vect3_dot(v2, v1);
    r2.y = vect3_dot(v2, &v3);

    /* Origin */
    Vect2 orig;
    orig.x = 0.0;
    orig.y = 0.0;
    /* Origin side relative to (r1, r2) */
    short orig_side = vect2_segment_side(&r1, &r2, &orig);

    /* Axis points: (1, 0, 0), (-1, 0, 0), (0, 1, 0), ... */
    Vect3 axes[6];
    for (int i = 0; i < 6; ++i) {
        axes[i].x = 0.0;
        axes[i].y = 0.0;
        axes[i].z = 0.0;
    }
    axes[0].x = axes[2].y = axes[4].z = 1.0;
    axes[1].x = axes[3].y = axes[5].z = -1.0;

    for (int i = 0; i < 6; ++i) {
        /* Project axis onto the plane, normalize */
        Vect2 rx;
        rx.x = vect3_dot(&axes[i], v1);
        rx.y = vect3_dot(&axes[i], &v3);
        vect2_normalize(&rx);

        /* Is projected axis vector between r1 and r2 (is origin on opposite side of (r1, r2))? */
        if (vect2_segment_side(&r1, &r2, &rx) != orig_side) {
            Vect3 vx;
            vx.x = rx.x * v1->x + rx.y * v3.x;
            vx.y = rx.x * v1->y + rx.y * v3.y;
            vx.z = rx.x * v1->z + rx.y * v3.z;
            bbox3_merge_vect3(bbox, &vx);
        }
    }
}


void bbox3_from_segment_latlng(Bbox3* bbox, const LatLng* ll1, const LatLng* ll2) {
    Vect3 v1, v2;
    vect3_from_lat_lng(ll1, &v1);
    vect3_from_lat_lng(ll2, &v2);

    bbox3_from_segment_vect3(bbox, &v1, &v2);
}


void bbox3_merge_vect3(Bbox3* bbox, const Vect3* v1) {
    Bbox3 v1_bbox;
    bbox3_from_vect3(&v1_bbox, v1);
    bbox3_merge(bbox, &v1_bbox);
}


void vect2_normalize(Vect2* vect) {
    double len = sqrt(vect->x * vect->x + vect->y * vect->y);
    if (len > 0.0) {
        vect->x /= len;
        vect->y /= len;
    } else {
        vect->x = 0.0;
        vect->y = 0.0;
    }
}


short vect2_segment_side(const Vect2* v1, const Vect2* v2, const Vect2* v) {
    double side = (v->x - v1->x) * (v2->y - v1->y) - (v2->x - v1->x) * (v->y - v1->y);
    return (side < 0) ? -1 : (side > 0) ? 1 : 0;
}
