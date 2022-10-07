#include <split/split.h>
#include <assert.h>
#include <float.h>
#include <math.h>
#include <split/bbox3.h>
#include <split/h3.h>
#include <split/vect3.h>

/*

Example: O-shaped polygon crossed by antimeridian.

After intersections are found and sorted by latitude,
intersection pairs 0-1 and 2-3 become segments in
exterior rings of the polygons in result.

          |
  +------(0)------+
  |       |       |
  |  +---(1)---+  |
  |  |    |    |  |
  |  |    |    |  |
  |  +---(2)---+  |
  |       |       |
  +------(3)------+
          |


Algorithm overview:

1. Initialization:
  - empty array of vertices
  - empty array of intersections
  - empty array of interior rings not split by antimeridian

2. Processing polygon rings:
  for each ring in the polygon:
    if ring is crossed by prime or antimeridian:
      for each segment in the ring:
        - add first endpoint to array of vertices
        if segment crosses antimeridian:
          - add an intersection, link to first endpoint
    else:
      - add ring to array of non-split holes

3. Preparing data:
  - sort intersections by latitude
  - set sort order value for each intersection

4. Building multipolygon:
  while there are unused vertices:
    - create empty exterior ring
    - start traversing vertex array forward starting from next unused vertex
    while current vertex is unused:
      - add current vertex to exterior ring
      - get next vertex (depends on traversal direction)
      if there is an intersection between vertices:
        - add intersection point to exterior ring
        - get adjacent intersection from sorted array
        - add next intersection point to exterior ring
        - update traversal direction based on intersection direction value
        - get next vertex
      - move to text vertex
    - check which non-split holes are inside the exterior ring and add them to polygon
    - add polygon to result

 */

#define DEBUG 0
#if DEBUG
# include <stdio.h>
#endif

#define SIGN(x) (((x) < 0) ? -1 : ((x) > 0) ? 1 : 0)

#define FP_EQUAL(v1, v2) ((v1) == (v2) || fabs((v1) - (v2)) < DBL_EPSILON)

#define MAX_INTERSECT_NUM_INIT (4)

typedef enum {
    SplitIntersectDir_None = 0,
    SplitIntersectDir_WE,
    SplitIntersectDir_EW
} SplitIntersectDir;

typedef struct {
    short dir;
    bool is_prime;
    double lat;
    int index;
    int sort_order;
} SplitIntersect;

typedef struct {
    const LatLng* latlng_p;
    int intersect_idx;
    short sign; /* longitude sign is set explicitly in case longitude of the vertex itself is zero */
    int link;   /* links first and last vertices in a ring */
} SplitVertex;

typedef struct {
    /* Vertices */
    int vertex_num;
    SplitVertex* vertices;

    /* Intersections */
    int max_intersect_num;
    int intersect_num;
    SplitIntersect* intersects;
    SplitIntersect** sorted_intersects;

    /* Non-split holes */
    int hole_num;
    const LinkedGeoLoop** holes;
} Split;

static bool is_polygon_crossed_by_180(const LinkedGeoPolygon* polygon);
static LinkedGeoPolygon* split_polygon_by_180(const LinkedGeoPolygon* polygon);

static bool is_ring_crossed(const LinkedGeoLoop* ring);
static double split_180_lat(const LatLng *coord1, const LatLng *coord2);

static bool split_init(Split* split, int ring_num, int vertex_num);
static void split_cleanup(Split* split);

static void split_process_ring(Split* split, const LinkedGeoLoop* ring);
static void split_prepare(Split* split);
static LinkedGeoPolygon* split_create_multi_polygon(Split* split);

static int split_add_vertex(Split* split, const LatLng* latlng);
static void split_add_intersect_after(
    Split* split, int after, SplitIntersectDir dir, bool is_prime, double lat);
static int split_add_intersect(
    Split* split, SplitIntersectDir dir, bool is_prime, double lat);
static void split_link_vertices(Split* split, int idx1, int idx2);
static void split_add_hole(Split* split, const LinkedGeoLoop* hole);

static void split_sort_intersects(Split* split);
static int split_intersect_ptr_cmp(const void* a, const void* b);

static int split_find_next_vertex(Split* split, int* start);
static LinkedGeoPolygon* split_create_polygon_vertex(Split* split, int vertex_idx);
static const SplitIntersect* split_get_intersect_after(const Split* split, int idx);

static void split_intersect_get_latlng(const SplitIntersect* intersect, short sign, LatLng* latlng);

static int count_polygon_vertices(const LinkedGeoPolygon* polygon, int* ring_num);
static int count_ring_vertices(const LinkedGeoLoop* ring);

static short latlng_ring_pos(
    const LinkedGeoLoop* ring, short sign, const Bbox3* bbox, const LatLng* latlng);
static short segment_intersect(const Vect3* v1, const Vect3* v2, const Vect3* u1, const Vect3* u2);
static short point_between(const Vect3* v1, const Vect3* v2, const Vect3* p);

static LinkedGeoPolygon* copy_linked_geo_polygon(const LinkedGeoPolygon* polygon);
static LinkedGeoLoop* copy_linked_geo_loop(const LinkedGeoLoop* loop);
static LinkedLatLng* copy_linked_latlng(const LinkedLatLng* latlng);

static LinkedLatLng* add_latlng_unique(LinkedGeoLoop* loop, const LatLng* latlng, bool* skipped);

#if DEBUG
static void dbg_print_split(const Split* split);
static void dbg_print_vect3(const Vect3* vect);
static void dbg_print_bbox_polygon(const Bbox3* bbox);
static void dbg_print_latlng(const LatLng* latlng);
static void dbg_print_double(double value);
#endif

bool is_crossed_by_180(const LinkedGeoPolygon* multi_polygon) {
    for (const LinkedGeoPolygon* polygon = multi_polygon;
         polygon != NULL;
         polygon = polygon->next)
    {
        if (is_polygon_crossed_by_180(polygon))
            return true;
    }
    return false;
}


LinkedGeoPolygon* split_by_180(const LinkedGeoPolygon* multi_polygon) {
    LinkedGeoPolygon* result = NULL;
    LinkedGeoPolygon* last = NULL;

    for (const LinkedGeoPolygon* polygon = multi_polygon;
         polygon != NULL;
         polygon = polygon->next)
    {
        /* Split or copy next polygon */
        LinkedGeoPolygon* next_result = is_polygon_crossed_by_180(polygon)
            ? split_polygon_by_180(polygon)
            : copy_linked_geo_polygon(polygon);
        if (!next_result) {
            if (result)
                free_linked_geo_polygon(result);
            return NULL;
        }

         if (!result) {
            result = next_result;
            last = next_result;
        } else {
            last->next = next_result;
        }
        while (last->next)
            last = last->next;
    }

    return result;
}


bool is_polygon_crossed_by_180(const LinkedGeoPolygon* polygon) {
    return (polygon->first && polygon->first->first)
        ? is_ring_crossed(polygon->first)
        : false;
}


LinkedGeoPolygon* split_polygon_by_180(const LinkedGeoPolygon* polygon) {
#if DEBUG
    printf("Splitting polygon\n");
#endif

    int ring_num = 0;
    int vertex_num = count_polygon_vertices(polygon, &ring_num);

    /* Init data */
    Split split;
    split_init(&split, ring_num, vertex_num);

    /* Process ring */
    const LinkedGeoLoop* ring = polygon->first;
    do {
        if (ring == polygon->first || is_ring_crossed(ring)) {
            split_process_ring(&split, ring);
        } else {
            split_add_hole(&split, ring);
        }
        ring = ring->next;
    } while (ring);

    /* Prepare data */
    split_prepare(&split);

#if DEBUG
    dbg_print_split(&split);
#endif

    /* Construct result */
    LinkedGeoPolygon* result = split_create_multi_polygon(&split);

    /* Cleanup */
    split_cleanup(&split);

    return result;
}


bool is_ring_crossed(const LinkedGeoLoop* ring) {
    const LinkedLatLng* cur = ring->first;
    const LinkedLatLng* next = cur->next;
    if (!next)
        return false; /* ring contains a single point */

    for(; cur != NULL; cur = cur->next, next = next->next ? next->next : ring->first) {
        /* Check if segment is split by antimeridian */
        double lng = cur->vertex.lng;
        double next_lng = next->vertex.lng;
        if (SIGN(lng) != SIGN(next_lng)
            && fabs(lng) + fabs(next_lng) > M_PI)
        {
            return true;
        }
    };
    return false;
}


double split_180_lat(const LatLng *coord1, const LatLng *coord2) {
    Vect3 p1, p2, normal, s;
    double y;

    /* Normal of circle containing points: normal = p1 x p2 */
    vect3_from_lat_lng(coord1, &p1);
    vect3_from_lat_lng(coord2, &p2);
    vect3_cross(&p1, &p2, &normal);

    /* y coordinate of 0/180 meridian circle normal */
    y = (coord1->lng < 0 || coord2->lng > 0) ? -1 : 1;

    /* Circle plane intersection vector: s = (p1 x p2) x {0, y, 0} */
    s.x = -(normal.z * y);
    s.y = 0;
    s.z = normal.x * y;
    vect3_normalize(&s); /* intersection point coordinates on unit sphere */

    return asin(s.z); /* latitude */
}


bool split_init(Split* split, int ring_num, int vertex_num) {
    *split = (Split){0};

    split->vertices = malloc(vertex_num * sizeof(SplitVertex));
    if (!split->vertices) {
        split_cleanup(split);
        return false;
    }

    split->max_intersect_num = MAX_INTERSECT_NUM_INIT;
    split->intersects = malloc(split->max_intersect_num * sizeof(SplitIntersect));
    if (!split->intersects) {
        split_cleanup(split);
        return false;
    }

    split->sorted_intersects = malloc(vertex_num * sizeof(SplitIntersect*));
    if (!split->sorted_intersects) {
        split_cleanup(split);
        return false;
    }

    if (ring_num > 1) {
        split->holes = malloc((ring_num - 1) * sizeof(LinkedGeoLoop*));
        if (!split->holes) {
            split_cleanup(split);
            return false;
        }
    }

    return true;
}


void split_cleanup(Split* split) {
    if (split->vertices)
        free(split->vertices);
    if (split->intersects)
        free(split->intersects);
    if (split->sorted_intersects)
        free(split->sorted_intersects);
    if (split->holes)
        free(split->holes);
    *split = (Split){0};
}


void split_process_ring(Split* split, const LinkedGeoLoop* ring) {
    const LinkedLatLng* cur = ring->first;
    const LinkedLatLng* next = cur->next;
    assert(next);
    short sign = 0;
    int first_vertex_idx = -1;
    int vertex_idx = -1;
    for (; cur != NULL; cur = cur->next, next = next->next ? next->next : ring->first) {
        /* Add vertex */
        vertex_idx = split_add_vertex(split, &cur->vertex);
        if (first_vertex_idx < 0)
            first_vertex_idx = vertex_idx;

        double lng = cur->vertex.lng;
        double next_lng = next->vertex.lng;
        short next_sign = SIGN(next_lng);
        if (sign == 0) {
            sign = SIGN(lng);

            if (sign != 0) {
                /* Set sign for vertices traversed so far */
                for (int i = first_vertex_idx; i <= vertex_idx; ++i)
                    split->vertices[i].sign = sign;
            }
        } else {
            /* Set vertex sign */
            split->vertices[vertex_idx].sign = sign;
        }

        if (sign != 0 && next_sign != 0 && next_sign != sign) {
            /* Prime or antimeridian crossed */

            /* Add intersection after current vertex */
            SplitIntersectDir dir = (sign < 0) ? SplitIntersectDir_WE : SplitIntersectDir_EW;
            bool is_prime = (fabs(lng) + fabs(next_lng) < M_PI);
            double lat = split_180_lat(&cur->vertex, &next->vertex);
            split_add_intersect_after(split, vertex_idx, dir, is_prime, lat);

            sign = next_sign;
        }
    }

    /* Link first and last vertices */
    split_link_vertices(split, first_vertex_idx, vertex_idx);
}


void split_prepare(Split* split) {
    split_sort_intersects(split);
}


LinkedGeoPolygon* split_create_multi_polygon(Split* split) {
    LinkedGeoPolygon* multi_polygon = NULL;
    LinkedGeoPolygon* last = NULL;
    int vertex_idx_start = 0;
    while (true) {
        /* Get next starting vertex */
        int vertex_idx = split_find_next_vertex(split, &vertex_idx_start);
        if (vertex_idx < 0) break; /* done */

        /* Create next polygon */
        LinkedGeoPolygon* polygon = split_create_polygon_vertex(split, vertex_idx);
        if (!polygon) {
            free_linked_geo_polygon(multi_polygon);
            return NULL;
        }

        /* Add polygon */
        if (!multi_polygon) {
            multi_polygon = polygon;
        } else {
            last->next = polygon;
        }
        last = polygon;
    }
    return multi_polygon;
}


int split_add_vertex(Split* split, const LatLng* latlng) {
    int index = split->vertex_num;
    SplitVertex* vertex = &split->vertices[split->vertex_num++];
    vertex->latlng_p = latlng;
    vertex->intersect_idx = -1;
    vertex->sign = 0;
    vertex->link = -1;
    return index;
}


void split_add_intersect_after(
    Split* split, int after, SplitIntersectDir dir, bool is_prime, double lat)
{
    int idx = split_add_intersect(split, dir, is_prime, lat);
    SplitIntersect* intersect = &split->intersects[idx];
    intersect->index = after;
    split->vertices[after].intersect_idx = idx;
}


int split_add_intersect(Split* split, SplitIntersectDir dir, bool is_prime, double lat)
{
    if (split->intersect_num == split->max_intersect_num) {
        /* Reallocate memory for intersections */
        /* NOTE: ignoring possible reallocation error */
        split->max_intersect_num *= 2;
        if (split->max_intersect_num > split->vertex_num)
            split->max_intersect_num = split->vertex_num;
        split->intersects = realloc(
            split->intersects,
            split->max_intersect_num * sizeof(SplitIntersect));
    }
    assert(split->intersects);

    int idx = split->intersect_num++;
    SplitIntersect* intersect = &split->intersects[idx];
    intersect->dir = dir;
    intersect->is_prime = is_prime;
    intersect->lat = lat;
    intersect->index = -1;
    intersect->sort_order = -1;
    return idx;
}


void split_link_vertices(Split* split, int idx1, int idx2) {
    assert(idx1 >= 0 && idx2 >= 0);
    assert(idx1 < split->vertex_num && idx2 < split->vertex_num);
    split->vertices[idx1].link = idx2;
    split->vertices[idx2].link = idx1;
}


void split_add_hole(Split* split, const LinkedGeoLoop* hole) {
    split->holes[split->hole_num++] = hole;
}


void split_sort_intersects(Split* split) {
    for (int i = 0; i < split->intersect_num; ++i) {
        SplitIntersect* intersect = &split->intersects[i];
        assert(intersect->dir != SplitIntersectDir_None);
        split->sorted_intersects[i] = intersect;
    }

    qsort(
        split->sorted_intersects,
        split->intersect_num,
        sizeof(SplitIntersect*),
        &split_intersect_ptr_cmp);
    for (int i = 0; i < split->intersect_num; ++i)
        split->sorted_intersects[i]->sort_order = i;
}


/**
   Sort intersections by latitude.

   For points on prime meridian sort value is:
   * 180deg - lat, if lat >= 0
   * -180deg - lat, it lat < 0

         S=-90        0           N=90
    ------*-----------+------------*------>
    prime       antimeridian         prime
   -180-lat          lat            180-lat
 */
int split_intersect_ptr_cmp(const void* a, const void* b) {
    const SplitIntersect* i1 = *((const SplitIntersect**) a);
    const SplitIntersect* i2 = *((const SplitIntersect**) b);

    double v1 = i1->lat;
    if (i1->is_prime)
        v1 = ((v1 < 0) ? -M_PI : M_PI) - v1;

    double v2 = i2->lat;
    if (i2->is_prime)
        v2 = ((v2 < 0) ? -M_PI : M_PI) - v2;

    if (v1 == v2)
        return 0;
    if (v1 < v2)
        return -1;
    return 1;
}


int split_find_next_vertex(Split* split, int* start) {
    for (int i = *start; i < split->vertex_num; ++i) {
        if (split->vertices[i].latlng_p) {
            *start = i + 1;
            return i;
        }
    }
    return -1;
}


LinkedGeoPolygon* split_create_polygon_vertex(Split* split, int vertex_idx) {
    /* Create result polygon */
    LinkedGeoPolygon* polygon = malloc(sizeof(LinkedGeoPolygon));
    if (!polygon)
        return NULL;
    *polygon = (LinkedGeoPolygon){0};

    /* Create outer shell loop */
    LinkedGeoLoop* loop = malloc(sizeof(LinkedGeoLoop));
    if (!loop) {
        free_linked_geo_polygon(polygon);
        return NULL;
    }
    *loop = (LinkedGeoLoop){0};
    add_linked_geo_loop(polygon, loop);

    int idx = vertex_idx;
    SplitVertex* vertex = &split->vertices[idx];
    assert(vertex->latlng_p);
    assert(vertex->sign != 0);

    short sign = vertex->sign;
    short step = 1; /* traversal direction */
#if DEBUG
    printf("# Starting from vertex idx: %d\n", idx);
#endif
    while (vertex->latlng_p) {
        bool skipped;
        int next_idx, intersect_idx;

        assert(vertex->sign == sign);
        assert(vertex->latlng_p->lng == 0 || ((sign > 0) == (vertex->latlng_p->lng > 0)));
#if DEBUG
        printf("\nstep: %d\n", step);
#endif
        /* Add vertex */
        if (!add_latlng_unique(loop, vertex->latlng_p, &skipped) && !skipped) {
            free_linked_geo_polygon(polygon);
            return NULL;
        }

        /* Unset coordinates for visited vertex */
        vertex->latlng_p = NULL;

        /* Get indices of the other endpoint and potential intersection */
        if (vertex->link > -1 && ((step > 0) == (idx > vertex->link))) {
            next_idx = vertex->link;
            /* Possible intersection is after last ring vertex */
            intersect_idx = (next_idx > idx) ? next_idx : idx;
        } else {
            next_idx = idx + step;
            /* Possible inersection is after first vertex */
            intersect_idx = (next_idx > idx) ? idx : next_idx;
        }

#if DEBUG
        printf("segment: %d, %d\n", idx, next_idx);
#endif

        /* Is there intersection on the segment? */
        const SplitIntersect* intersect = split_get_intersect_after(split, intersect_idx);
#if DEBUG
        printf("intersect idx: %d (%s)\n", intersect_idx, (intersect ? "intersect" : "no intersect"));
#endif
        if (intersect) {
            LatLng latlng;

            /* Get intersection coordinates */
            split_intersect_get_latlng(intersect, sign, &latlng);

            /* Add intersection vertex */
            if (!add_latlng_unique(loop, &latlng, &skipped) && !skipped) {
                free_linked_geo_polygon(polygon);
                return NULL;
            }

            /* Find next intersection */
            int sort_order = (intersect->sort_order % 2 == 0)
                ? intersect->sort_order + 1
                : intersect->sort_order - 1;
            intersect = split->sorted_intersects[sort_order];
            intersect_idx = intersect->index;

            /* Get next intersection coordinates */
            split_intersect_get_latlng(intersect, sign, &latlng);

            /* Add next intersection vertex */
            if (!add_latlng_unique(loop, &latlng, &skipped) && !skipped) {
                free_linked_geo_polygon(polygon);
                return NULL;
            }

            step = ((sign > 0) == (intersect->dir == SplitIntersectDir_WE)) ? 1 : -1;
            if (step > 0) {
                /* Intersecting segment second point */
                const SplitVertex* intersect_vertex = &split->vertices[intersect_idx];
                if (intersect_vertex->link > -1 && intersect_idx > intersect_vertex->link) {
                    next_idx = intersect_vertex->link;
                } else {
                    next_idx = intersect_idx + 1;
                }

            } else {
                /* Intersecting segment first point */
                next_idx = intersect_idx;
            }
        }

        /* Move to next vertex */
        idx = next_idx;
        assert(0 <= idx && idx < split->vertex_num);
        vertex = &split->vertices[idx];

#if DEBUG
        printf("next vertex idx: %d\n", idx);
        if (!vertex->latlng_p)
            printf("done\n\n");
#endif
    }

    /* Assign holes */
    Bbox3 bbox;
    bbox3_from_linked_loop(&bbox, loop);
#if DEBUG
    printf("Assigning holes: %d total\n", split->hole_num);
    dbg_print_bbox_polygon(&bbox);
    printf("\n");
#endif
    for (int i = 0; i < split->hole_num; ++i) {
        const LinkedGeoLoop* hole = split->holes[i];
        if (!hole) continue;

        /* Check if hole vertices are inside the polygon */
        short pos = 0;
        for (const LinkedLatLng* cur = hole->first; cur != NULL; cur = cur->next) {
            pos = latlng_ring_pos(loop, sign, &bbox, &cur->vertex);
            if (pos != 0) break; /* the vertex is either inside or outside */
        }

        if (pos != -1) {
#if DEBUG
            printf("hole assigned\n");
#endif
            /* Copy hole */
            LinkedGeoLoop* hole_copy = copy_linked_geo_loop(hole);
            if (!hole_copy) {
                free_linked_geo_loop(hole_copy);
                return NULL;
            }

            /* Add to polygon */
            add_linked_geo_loop(polygon, hole_copy);

            /* Remove hole from the list */
            split->holes[i] = NULL;
        }
    }

    return polygon;
}


const SplitIntersect* split_get_intersect_after(const Split* split, int idx) {
    SplitVertex* vertex = &split->vertices[idx];
    return (vertex->intersect_idx >= 0)
        ? &split->intersects[vertex->intersect_idx]
        : NULL;
}


void split_intersect_get_latlng(const SplitIntersect* intersect, short sign, LatLng* latlng) {
    latlng->lat = intersect->lat;
    if (intersect->is_prime) {
        latlng->lng = 0;
    } else {
        latlng->lng = (sign > 0) ? M_PI : -M_PI;
    }
}


int count_polygon_vertices(const LinkedGeoPolygon* polygon, int* ring_num) {
    assert(polygon->first && polygon->last);

    if (ring_num) *ring_num = 0;
    int num = 0;
    LinkedGeoLoop* ring = polygon->first;
    while (ring) {
        if (ring_num) ++(*ring_num);
        num += count_ring_vertices(ring);
        ring = ring->next;
    }
    return num;
}


int count_ring_vertices(const LinkedGeoLoop* ring) {
    assert(ring->first && ring->last);

    int num = 0;
    LinkedLatLng* point = ring->first;
    while (point) {
        ++num;
        point = point->next;
    }
    return num;
}


short latlng_ring_pos(const LinkedGeoLoop* ring, short sign, const Bbox3* bbox, const LatLng* latlng) {
    /* Check longitude sign */
    assert(sign != 0);
    short sign_latlng = SIGN(latlng->lng);
    if (sign_latlng != 0 && sign_latlng != sign)
        return -1;

    /* Point to vector */
    Vect3 vect;
    vect3_from_lat_lng(latlng, &vect);

    /* Check bbox */
    if (!bbox3_contains_vect3(bbox, &vect)) {
#if DEBUG
        printf("hole is not in bbox\n");
#endif
        return -1;
    }

    /* Create a point that's guaranteed to be outside the polygon */
    LatLng out;
    out.lng = (latlng->lng == 0) ? -sign * 1e-10 : -latlng->lng;
    out.lat = latlng->lat;
    Vect3 out_vect;
    vect3_from_lat_lng(&out, &out_vect);

    /* Count a number of intersections between the ring and (latlng, out) segment */
    int intersect_num = 0;
    const LinkedLatLng* cur = ring->first;
    const LinkedLatLng* next = cur->next;
    if (!next)
        return true; /* single ring vertex exactly matches the point */

    Vect3 cur_vect, next_vect;
    vect3_from_lat_lng(&cur->vertex, &cur_vect);
    for (; cur != NULL; cur = cur->next, next = next->next ? next->next : ring->first) {
        /* Check if point matches ring vertex */
        if (vect3_eq(&vect, &cur_vect))
            return 0;

        /* Next vertex */
        vect3_from_lat_lng(&next->vertex, &next_vect);

        /* Check if segment endpoints match */
        if (!vect3_eq(&cur_vect, &next_vect)) {
            short intersect = segment_intersect(&cur_vect, &next_vect, &vect, &out_vect);
            if (intersect == 0)
                return 0; /* point on ring segment */

            if (intersect > 0)
                ++intersect_num;
        }

        cur_vect = next_vect;
    }

    return (intersect_num % 2 == 0) ? -1 : 1;
}


short segment_intersect(const Vect3* v1, const Vect3* v2, const Vect3* u1, const Vect3* u2) {
    /* Normals of V and U planes */
    Vect3 vn, un;
    vect3_cross(v1, v2, &vn);
    vect3_normalize(&vn);
    vect3_cross(u1, u2, &un);
    vect3_normalize(&un);

    /* Are the planes the same? */
    double normal_dot = vect3_dot(&vn, &un);
    if (FP_EQUAL(fabs(normal_dot), 1.0)) {
        short ret = point_between(v1, v2, u1);
        if (ret == -1)
            ret = point_between(v1, v2, u2);
        if (ret == -1)
            ret = point_between(u1, u2, v1);
        if (ret == -1)
            ret = point_between(u1, u2, v2);
        return ret;
    }

    short v1_side = SIGN(vect3_dot(&un, v1));
    short v2_side = SIGN(vect3_dot(&un, v2));
    short u1_side = SIGN(vect3_dot(&vn, u1));
    short u2_side = SIGN(vect3_dot(&vn, u2));

    /* v1 and v2 are on the same side of U plane */
    if (v1_side == v2_side && v1_side != 0)
        return -1;

    /* u1 and u2 are on the same side of V plane */
    if (u1_side == u2_side && u1_side != 0)
        return -1;

    if (v1_side != v2_side && (v1_side + v2_side) == 0
        && u1_side != u2_side && (u1_side + u2_side) == 0)
    {
        /* Intersection point */
        Vect3 intersect;
        vect3_cross(&vn, &un, &intersect);
        vect3_normalize(&intersect);

        /* Is intersection point inside both arcs? */
        if (point_between(v1, v2, &intersect) != -1 && point_between(u1, u2, &intersect) != -1)
            return 1;

        /* Try antipodal intersection point */
        vect3_scale(&intersect, -1.0);
        if (point_between(v1, v2, &intersect) != -1 && point_between(u1, u2, &intersect) != -1)
            return 1;

        return -1;
    }

    assert(v1_side == 0 || v2_side == 0 || u1_side == 0 || u2_side == 0);
    return 0;
}


short point_between(const Vect3* v1, const Vect3* v2, const Vect3* p) {
    if (vect3_eq(p, v1) || vect3_eq(p, v2))
        return 0;

    /* Vector bisecting angle between v1 and v2 */
    Vect3 middle;
    vect3_sum(v1, v2, &middle);
    vect3_normalize(&middle);

    /* How similar is v1 (or v2) to middle */
    double min_similarity = vect3_dot(v1, &middle);

    if (fabs(1.0 - min_similarity) > 1e-10) {
        /* Edge is sufficiently curved, use dot product test */
        /*
          If projection length is larger than the projection of bounding vectors,
          then point vector must be closer to center.
         */
        return (vect3_dot(p, &middle) > min_similarity) ? 1 : -1;

    } else {
        /* Edge is very narrow, dot product test fails. */
        /* Check if vectors from bounds to point are in opposite directions */
        /* TODO: is this test applicable for both cases for coplanar vectors ?? */
        Vect3 d1, d2;
        vect3_diff(p, v1, &d1);
        vect3_normalize(&d1);
        vect3_diff(p, v2, &d2);
        vect3_normalize(&d2);
        return (vect3_dot(&d1, &d2) < 0.0) ? 1 : -1;
    }
}


LinkedGeoPolygon* copy_linked_geo_polygon(const LinkedGeoPolygon* polygon) {
#if DEBUG
    printf("Copying polygon\n");
#endif

    LinkedGeoPolygon *copy = malloc(sizeof(LinkedGeoPolygon));
    if (!copy)
        return NULL;
    *copy = (LinkedGeoPolygon){0};

    for (const LinkedGeoLoop* loop = polygon->first;
         loop != NULL;
         loop = loop->next)
    {
        LinkedGeoLoop* loop_copy = copy_linked_geo_loop(loop);
        if (!loop_copy) {
            free_linked_geo_polygon(copy);
            return NULL;
        }
        add_linked_geo_loop(copy, loop_copy);
    }

    return copy;
}


LinkedGeoLoop* copy_linked_geo_loop(const LinkedGeoLoop* loop) {
    LinkedGeoLoop* copy = malloc(sizeof(LinkedGeoLoop));
    if (!copy)
        return NULL;
    *copy = (LinkedGeoLoop){0};

    /* Copy vertices */
    for (const LinkedLatLng* latlng = loop->first;
         latlng != NULL;
         latlng = latlng->next)
    {
        LinkedLatLng* latlng_copy = copy_linked_latlng(latlng);
        if (!latlng_copy) {
            free_linked_geo_loop(copy);
            return NULL;
        }
        add_linked_latlng(copy, latlng_copy);
    }

    return copy;
}


LinkedLatLng* copy_linked_latlng(const LinkedLatLng* latlng) {
    LinkedLatLng* copy = malloc(sizeof(LinkedLatLng));
    if (!copy)
        return NULL;
    *copy = (LinkedLatLng){0};
    copy->vertex = latlng->vertex;
    return copy;
}


LinkedLatLng* add_latlng_unique(LinkedGeoLoop* loop, const LatLng* latlng, bool* skipped) {
#if DEBUG
    dbg_print_latlng(latlng);
    printf("\n");
#endif

    *skipped = false;
    if (loop->last) {
        const LatLng* last = &loop->last->vertex;
        if (last->lat == latlng->lat && last->lng == latlng->lng) {
#if DEBUG
            printf("^ duplicate vertex, skipped\n");
#endif
            *skipped = true;
            return NULL;
        }
    }

    LinkedLatLng* linked = malloc(sizeof(LinkedLatLng));
    if (!linked)
        return NULL;
    *linked = (LinkedLatLng){0};
    linked->vertex = *latlng;

    add_linked_latlng(loop, linked);

    return linked;
}


#if DEBUG

void dbg_print_split(const Split* split) {
    for (int i = 0; i < split->vertex_num; ++i) {
        const SplitVertex* vertex = &split->vertices[i];

        printf("[%d] ", i);
        dbg_print_latlng(vertex->latlng_p);

        const SplitIntersect* intersect = split_get_intersect_after(split, i);
        if (intersect) {
            printf(" x ");
            printf("[%d] ", intersect->sort_order);
            printf("%s ", (intersect->dir == SplitIntersectDir_EW) ? "E>W" : "W>E");
            printf("lat: ");
            dbg_print_double(radsToDegs(intersect->lat));
        }
        if (vertex->link >= 0)
            printf(" link: [%d]", vertex->link);
        printf("\n");
    }
}


void dbg_print_vect3(const Vect3* vect) {
    printf("(");
    dbg_print_double(vect->x);
    printf(", ");
    dbg_print_double(vect->y);
    printf(", ");
    dbg_print_double(vect->z);
    printf(")");
    printf("[len: %f]", vect3_len(vect));
}



void dbg_print_bbox_polygon(const Bbox3* bbox) {
    Vect3 v1, v2;
    v1.x = bbox->xmin;
    v1.y = bbox->ymin;
    v1.z = bbox->zmin;
    v2.x = bbox->xmax;
    v2.y = bbox->ymax;
    v2.z = bbox->zmax;

    LatLng ll1, ll2;
    vect3_to_lat_lng(&v1, &ll1);
    vect3_to_lat_lng(&v2, &ll2);

    LatLng nw, ne, se, sw;
    nw.lng = ll1.lng;
    nw.lat = ll2.lat;
    ne.lng = ll2.lng;
    ne.lat = ll2.lat;
    se.lng = ll2.lng;
    se.lat = ll1.lat;
    sw.lng = ll1.lng;
    sw.lat = ll1.lat;

    printf("((");
    dbg_print_latlng(&nw);
    printf(", ");
    dbg_print_latlng(&ne);
    printf(", ");
    dbg_print_latlng(&se);
    printf(", ");
    dbg_print_latlng(&sw);
    printf(", ");
    dbg_print_latlng(&nw);
    printf("))");
}


void dbg_print_latlng(const LatLng* latlng) {
    dbg_print_double(radsToDegs(latlng->lng));
    printf(" ");
    dbg_print_double(radsToDegs(latlng->lat));
}


void dbg_print_double(double value) {
    printf("%.*f", DECIMAL_DIG, value);
}

#endif
