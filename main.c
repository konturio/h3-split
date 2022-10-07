#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <h3/h3api.h>
#include <split/h3.h>
#include <split/parse.h>
#include <split/print.h>
#include <split/split.h>

static void exit_usage(const char* name);

typedef struct {
    const char* input_path;
    bool verbose;
} Args;

static void parse_args(Args* args, int argc, char** argv);
static char* read_input(const char* path, size_t* size);


int main(int argc, char** argv) {
    /* Parse arguments */
    Args args;
    parse_args(&args, argc, argv);

    /* Read data */
    size_t size;
    char* data = read_input(args.input_path, &size);
    if (!data) {
        printf("Failed to read data from `%s'\n", args.input_path);
        exit(EXIT_FAILURE);
    }

    /* Parse */
    WktParseResult parse_result = wkt_parse(data, size);
    if (parse_result.error) {
        printf(
            "(at %d) %s\n",
            (int) parse_result.error_pos,
            wkt_parse_error_to_string(parse_result.error));
        if (parse_result.message)
            printf("%s\n", parse_result.message);
        exit(EXIT_FAILURE);
    }
    LinkedGeoPolygon* polygon = parse_result.object;

    if (args.verbose) {
        /* Print input */
        printf("Input:\n");
        print_polygon(polygon);
        printf("\n\n");
    }

    if (is_crossed_by_180(polygon)) {
        if (args.verbose) printf("Split\n\n");

        /* Split and print */
        LinkedGeoPolygon* multi_polygon = split_by_180(polygon);
        print_polygon(multi_polygon);
        printf("\n");
        free_linked_geo_polygon(multi_polygon);

    } else {
        if (args.verbose) printf("Not split\n\n");

        /* Not split, just print input */
        print_polygon(polygon);
        printf("\n");
    }

    /* Cleanup */
    if (polygon)
        free(polygon);
    if (data)
        free(data);
}


void exit_usage(const char* name) {
    printf("Usage:\n");
    printf("$ %s <filename>[ -v]\n", name);
    printf("$ echo <wkt> | %s\n", name);
    exit(EXIT_FAILURE);
}


void parse_args(Args* args, int argc, char** argv) {
    *args = (Args){0};

    int opt;
    while ((opt = getopt(argc, argv, "vf:")) != -1) {
        switch (opt) {
            case 'v':
                args->verbose = true;
                break;
            default:
                exit_usage(argv[0]);
        }
    }

    if (optind < argc)
        args->input_path = argv[optind];
}


char* read_input(const char* path, size_t* size) {
    *size = 0;

    /* Open file stream */
    FILE* input = path ? fopen(path, "r") : stdin;
    if (!input)
        return NULL;

    /* Allocate initial data memory */
    size_t alloc_size = 128;
    char* data = malloc(alloc_size);
    if (!data) {
        if (path) fclose(input);
        return NULL;
    }

    /* Read file data */
    char buffer[128];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        /* Reallocate data memory */
        while (*size + bytes_read > alloc_size) {
            alloc_size *= 2;
            data = realloc(data, alloc_size);
            if (!data) {
                /* Failed to reallocate */
                if (path) fclose(input);
                free(data);
                *size = 0;
                return NULL;
            }
        }

        /* Copy data */
        memcpy(data + *size, buffer, bytes_read);
        *size += bytes_read;
    }

    if (path) fclose(input);
    return data;
}
