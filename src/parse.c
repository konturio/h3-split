#include <split/parse.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <split/h3.h>

static const char WktTypeNamePolygon[] = "polygon";
static const char WktTypeNameMultiPolygon[] = "multipolygon";

typedef enum {
    WktObjectType_None = 0,
    WktObjectType_Polygon,
    WktObjectType_MultiPolygon
} WktObjectType;

typedef struct {
    const char* data;
    size_t len;
} WktData;

static void result_init(WktParseResult* result);
static void data_init(WktData* data, const char* wkt, size_t len);

static bool is_empty(WktData* data);
static void advance(WktData* data, size_t step);
static void skip_ws(WktData* data);
static WktObjectType read_type(WktData* data, WktParseResult* result);

static void parse_polygon(WktData* data, WktParseResult* result);
static void parse_multi_polygon(WktData* data, WktParseResult* result);

static bool parse_next_polygon(
    WktData* data, WktParseResult* result,
    LinkedGeoPolygon** multi_polygon,
    LinkedGeoPolygon** last_polygon);
static bool parse_next_ring(
    WktData* data, WktParseResult* result, LinkedGeoPolygon* polygon);

static bool parse_next_point(
    WktData* data, WktParseResult* result, LinkedGeoLoop* ring);

static double parse_coord(WktData* data, WktParseResult* result);

static LinkedGeoPolygon* create_empty_polygon();
static LinkedGeoLoop* create_empty_ring();

static void add_ring(LinkedGeoPolygon* polygon, LinkedGeoLoop* ring);
static void add_point(LinkedGeoLoop* ring, LinkedLatLng* point);

static void free_polygon(LinkedGeoPolygon* polygon);
static void free_ring(LinkedGeoLoop* ring);

WktParseResult wkt_parse(const char* wkt, size_t len) {
    WktParseResult result;
    result_init(&result);

    WktData data;
    data_init(&data, wkt, len);

    /* Parse type */
    WktObjectType type = read_type(&data, &result);
    if (result.error)
        return result;

    /* Parse data */
    switch (type) {
        case WktObjectType_Polygon:
            parse_polygon(&data, &result);
            break;

        case WktObjectType_MultiPolygon:
            parse_multi_polygon(&data, &result);
            break;

        default:
            assert(false);
    }

    result.error_pos = result.error ? data.data - wkt : 0;
    return result;
}


const char* wkt_parse_error_to_string(WktParseError error) {
    switch (error) {
        case WktParseError_Ok:
            return "Ok";
        case WktParseError_TypeExpected:
            return "Type name expected";
        case WktParseError_InvalidType:
            return "Invalid type name";
        case WktParseError_LeftParenExpected:
            return "`(' expected";
        case WktParseError_RightParenExpected:
            return "`)' expected";
        case WktParseError_CommaExpected:
            return "`,' expected";
        case WktParseError_NumberExpected:
            return "Number expected";
        case WktParseError_InvalidNumber:
            return "Invalid number";
        case WktParseError_MemAllocFailed:
            return "Memory allocation failure";
        default:
            assert(false);
    }
}


void data_init(WktData* data, const char* wkt, size_t len) {
    assert(wkt);
    data->data = wkt;
    data->len = len;
}


void result_init(WktParseResult* result) {
    result->error = WktParseError_Ok;
    result->type = H3Type_None;
    result->object = NULL;
}


bool is_empty(WktData* data) {
    return data->len == 0;
}


void advance(WktData* data, size_t step) {
    assert(step <= data->len);
    data->data += step;
    data->len -= step;
}


void skip_ws(WktData* data) {
    while (data->len > 0 && isspace(data->data[0]))
        advance(data, 1);
}


WktObjectType read_type(WktData* data, WktParseResult* result) {
    /* Whitespace */
    skip_ws(data);
    if (is_empty(data)) {
        result->error = WktParseError_TypeExpected;
        return WktObjectType_None;
    }

    /* Find end of type name */
    size_t pos = 0;
    while (pos < data->len && isalpha(data->data[pos]))
        ++pos;

    if (pos == 0) {
        result->error = WktParseError_TypeExpected;
        return WktObjectType_None;
    }

    WktObjectType type = WktObjectType_None;

    /* Copy to null-terminated string, convert to lowercase */
    char* typename = malloc(pos + 1); /* alloc. copy */
    if (!typename) {
        result->error = WktParseError_MemAllocFailed;
        return WktObjectType_None;
    }
    memcpy(typename, data->data, pos);
    typename[pos] = '\0';
    for (size_t i = 0; i < pos; ++i)
        typename[i] = tolower(typename[i]);

    /* Match to type */
    if (strncmp(typename, WktTypeNamePolygon, pos) == 0)
        type = WktObjectType_Polygon;
    else if (strncmp(typename, WktTypeNameMultiPolygon, pos) == 0)
        type = WktObjectType_MultiPolygon;

    free(typename); /* free copy */

    /* Advance */
    advance(data, pos);

    if (type == WktObjectType_None)
        result->error = WktParseError_InvalidType;
    return type;
}


void parse_polygon(WktData* data, WktParseResult* result) {
    /* Object type */
    result->type = H3Type_GeoPolygon;

    LinkedGeoPolygon* polygon = NULL;
    LinkedGeoPolygon* dummy_last_polygon = NULL;

    if (parse_next_polygon(data, result, &polygon, &dummy_last_polygon)) {
        /* Return polygon */
        assert(polygon);
        result->object = polygon;

    } else if (!result->error) {
        /* Return empty polygon */
        polygon = create_empty_polygon();
        if (polygon)
            result->object = polygon;
        else
            result->error = WktParseError_MemAllocFailed;
    }
}


void parse_multi_polygon(WktData* data, WktParseResult* result) {
    /* Object type */
    result->type = H3Type_GeoPolygon;

    /* Whitespace */
    skip_ws(data);
    if (is_empty(data)) {
        LinkedGeoPolygon* multi_polygon = create_empty_polygon();
        if (multi_polygon)
            result->object = multi_polygon;
        else
            result->error = WktParseError_MemAllocFailed;
        return;
    }

    /* Multi polygon data start */
    if (data->data[0] != '(') {
        result->error = WktParseError_LeftParenExpected;
        return;
    }
    advance(data, 1);

    /* Parse  */
    LinkedGeoPolygon* multi_polygon = NULL;
    LinkedGeoPolygon* last_polygon = NULL;
    while (parse_next_polygon(data, result, &multi_polygon, &last_polygon)) {}
    if (result->error) {
        if (multi_polygon)
            free_polygon(multi_polygon);
        return;
    }

    /* Polygon data end */
    skip_ws(data);
    if (is_empty(data) || data->data[0] != ')') {
        if (multi_polygon)
            free_polygon(multi_polygon);
        result->error = WktParseError_RightParenExpected;
        return;
    }

    /* Return multipolygon */
    if (!multi_polygon) {
        multi_polygon = create_empty_polygon();
        if (multi_polygon)
            result->object = multi_polygon;
        else
            result->error = WktParseError_MemAllocFailed;
    }
    result->object = multi_polygon;
}


bool parse_next_polygon(
    WktData* data, WktParseResult* result,
    LinkedGeoPolygon** multi_polygon,
    LinkedGeoPolygon** last_polygon)
{
    assert(multi_polygon && last_polygon);

    /* Whitespace */
    skip_ws(data);
    if (is_empty(data) || data->data[0] == ')')
        return false; /* end of polygon data, handled by caller */

    /* Comma */
    if (*multi_polygon) {
        assert(*last_polygon);
        /* Not a first polygon, comma expected */
        if (data->data[0] != ',') {
            result->error = WktParseError_CommaExpected;
            return false;
        }
        /* Move to polygon data */
        advance(data, 1);
        skip_ws(data);
    }

    /* Polygon data start */
    if (data->data[0] != '(') {
        result->error = WktParseError_LeftParenExpected;
        return false;
    }
    advance(data, 1);

    /* Create polygon */
    LinkedGeoPolygon* polygon = create_empty_polygon();
    if (!polygon) {
        result->error = WktParseError_MemAllocFailed;
        return false;
    }

    /* Parse rings */
    while (parse_next_ring(data, result, polygon)) {}
    if (result->error) {
        free_polygon(polygon);
        return false;
    }

    /* Polygon data end */
    skip_ws(data);
    if (is_empty(data) || data->data[0] != ')') {
        free_polygon(polygon);
        result->error = WktParseError_RightParenExpected;
        return false;
    }
    advance(data, 1);

    /* Return polygon */
    if (!(*multi_polygon)) {
        *multi_polygon = polygon;
    } else {
        (*last_polygon)->next = polygon;
    }
    *last_polygon = polygon;

    return true;
}


bool parse_next_ring(
    WktData* data, WktParseResult* result, LinkedGeoPolygon* polygon)
{
    /* Whitespace */
    skip_ws(data);
    if (is_empty(data) || data->data[0] == ')')
        return false; /* end of ring data, handled by caller */

    /* Comma */
    if (polygon->last) {
        /* Interior ring, comma expected */
        if (data->data[0] != ',') {
            result->error = WktParseError_CommaExpected;
            return false;
        }
        /* Move to ring data */
        advance(data, 1);
        skip_ws(data);
    }

    /* Ring data start */
    if (data->data[0] != '(') {
        result->error = WktParseError_LeftParenExpected;
        return false;
    }
    advance(data, 1);

    /* Create ring */
    LinkedGeoLoop* ring = create_empty_ring();
    if (!ring) {
        result->error = WktParseError_MemAllocFailed;
        return false;
    }

    /* Parse points */
    while (parse_next_point(data, result, ring)) {}
    if (result->error) {
        free_ring(ring);
        return false;
    }

    /* Ring data end */
    skip_ws(data);
    if (is_empty(data) || data->data[0] != ')') {
        free_ring(ring);
        result->error = WktParseError_RightParenExpected;
        return false;
    }
    advance(data, 1);

    /* Add ring */
    add_ring(polygon, ring);

    return true;
}


bool parse_next_point(
    WktData* data, WktParseResult* result, LinkedGeoLoop* ring)
{
    skip_ws(data);
    if (is_empty(data) || data->data[0] == ')')
        return false; /* end of point data, handled by caller */

    /* Comma */
    if (ring->last) {
        /* Not a first point, comma expected */
        if (data->data[0] != ',') {
            result->error = WktParseError_CommaExpected;
            return false;
        }
        /* Move to point data */
        advance(data, 1);
    }

    /* Parse point coordinates */
    LatLng coords = {0};
    /* lng */
    coords.lng = parse_coord(data, result);
    if (result->error)
        return false;
    coords.lng = degsToRads(coords.lng);
    /* lat */
    coords.lat = parse_coord(data, result);
    if (result->error)
        return false;
    coords.lat = degsToRads(coords.lat);

    /* Skip last point */
    /* assuming only the last point exactly matches the first */
    if (ring->first) {
        LatLng* coords_first = &ring->first->vertex;
        if (coords.lng == coords_first->lng
            && coords.lat == coords_first->lat)
        {
            return false;
        }
    }

    /* Create point */
    LinkedLatLng* point = malloc(sizeof(LinkedLatLng));
    if (!point) {
        result->error = WktParseError_MemAllocFailed;
        return false;
    }
    *point = (LinkedLatLng){0};
    point->vertex = coords;

    /* Add point */
    add_point(ring, point);

    return true;
}


double parse_coord(WktData* data, WktParseResult* result) {
    skip_ws(data);

    /* Find end of number */
    size_t pos = 0;
    while (pos < data->len
           && (isalnum(data->data[pos]) || strchr("+-.", data->data[pos])))
        ++pos;

    if (pos == 0) {
        result->error = WktParseError_NumberExpected;
        return 0.0;
    }

    /* Copy to null-terminated string */
    char* number = malloc(pos + 1);
    if (!number) {
        result->error = WktParseError_MemAllocFailed;
        return 0.0;
    }
    memcpy(number, data->data, pos);
    number[pos] = '\0';

    /* Parse number */
    char* end;
    double value = strtod(data->data, &end);
    if (errno || end != data->data + pos)
        result->error = WktParseError_InvalidNumber;

    /* Advance */
    advance(data, pos);

    free(number); /* free copy */
    return value;
}


LinkedGeoPolygon* create_empty_polygon() {
    LinkedGeoPolygon* polygon = malloc(sizeof(LinkedGeoPolygon));
    if (polygon)
        *polygon = (LinkedGeoPolygon){0};
    return polygon;
}


LinkedGeoLoop* create_empty_ring() {
    LinkedGeoLoop* ring = malloc(sizeof(LinkedGeoLoop));
    if (ring)
        *ring = (LinkedGeoLoop){0};
    return ring;
}


void add_ring(LinkedGeoPolygon* polygon, LinkedGeoLoop* ring) {
    LinkedGeoLoop* last = polygon->last;
    if (!last) {
        assert(!polygon->first);
        polygon->first = ring;
    } else {
        last->next = ring;
    }
    polygon->last = ring;
}


void add_point(LinkedGeoLoop* ring, LinkedLatLng* point) {
    LinkedLatLng* last = ring->last;
    if (!last) {
        assert(!ring->first);
        ring->first = point;
    } else {
        last->next = point;
    }
    ring->last = point;
}


void free_polygon(LinkedGeoPolygon* polygon) {
    free_linked_geo_polygon(polygon);
}


void free_ring(LinkedGeoLoop* ring) {
    free_linked_geo_loop(ring);
}
