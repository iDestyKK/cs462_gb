#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <unistd.h>

#include "header.h"
#include "jacobi.h"

hw_params_t hw3_params = {0,};

#define BUFSIZE 1024
#define EPSILON 0.000005

int
read_input_file(hw_params_t* params) {
    FILE *infile;
    char buf[BUFSIZE], *cmd;
    int i, n, ret = 0;

    params->max_iterations = -1;  /* uncountable */
    params->num_sources = 0;  /* default values */
    params->num_threads = 1;
    params->vis_step = 10000000;  /* big enough to never generate the graphical heat map */
    params->alg_type = 0;  /* pick a sensible default */

    if (NULL == params->input )
        return -1;

    infile = fopen(params->input, "r");
    if (NULL == infile ) {
        return -1;
    }
    while (NULL != fgets(buf, BUFSIZE, infile) ) {
        if ('#' == buf[0])
            continue;  /* leave the comments out */
        /* skip all spaces */
        cmd = buf;
        while (isspace(*cmd)) cmd++;

        if (0 == strncmp(cmd, "iter", 4) ) {
            sscanf(cmd + 5, "%u", &params->max_iterations);
            continue;
        }
        if (0 == strncmp(cmd, "alg", 3) ) {
            sscanf(cmd + 4, "%u", &params->alg_type);
            continue;
        }
        if (0 == strncmp(cmd, "resolution", 10) ) {
            sscanf(cmd + 11, "%u", &params->resolution);
            continue;
        }
        if (0 == strncmp(cmd, "threads", 7) ) {
            sscanf(cmd + 8, "%u", &params->num_threads);
            continue;
        }
        if (0 == strncmp(cmd, "vis_res", 7) ) {
            sscanf(cmd + 8, "%u", &params->vis_res);
            continue;
        }
        if (0 == strncmp(cmd, "vis_step", 8) ) {
            sscanf(cmd + 9, "%d", &params->vis_step);
            continue;
        }
        if (0 == strncmp(cmd, "numsrc", 6) ) {
            sscanf(cmd + 7, "%u", &params->num_sources);
            continue;
        }
        params->heat_sources = (heat_source_t*)malloc(params->num_sources * sizeof(heat_source_t));
        for (i = 0; i < params->num_sources; ++i) {
            n = sscanf(buf, "%f %f %lf %lf",
                       &(params->heat_sources[i].x), &(params->heat_sources[i].y),
                       &(params->heat_sources[i].range), &(params->heat_sources[i].temp));
            if (4 != n) {
                fprintf(stderr, "Input file %s malformed (pb with the heat sources)\n",
                        params->input);
                ret = -1;
                goto complete_and_return;
            }
            /* Bail out if we are short in heat sources */
            if ((i != (params->num_sources-1)) && (NULL == fgets(buf, BUFSIZE, infile))) {
                fprintf(stderr, "Missing heat sources (%d != %d). Update the count\n",
                        params->num_sources, i+1);
                params->num_sources = i + 1;
                break;
            }
        }
        break;
    }
    params->vis_data = NULL;
    if (HW3_FLAG_GENERATE_HEAT_IMAGE & params->flags) {
        params->vis_res += 2;  /* for the borders */
        params->vis_data = (double*)malloc(params->vis_res * params->vis_res * sizeof(double));
    }

 complete_and_return:
    fclose(infile);
    return ret;
}

void
print_params(hw_params_t* param) {
    printf("# Max iterations ");
    if (0 == param->max_iterations)
        printf("unrestricted\n");
    else
        printf("%u\n", param->max_iterations);
    printf("# Resolution %u\n",  param->resolution);
    printf("# Visual Resolution %u\n", param->vis_res);
    printf("# Input file %s\n", param->input);
    printf("# Num heat sources %u\n", param->num_sources);
    if (HW3_FLAG_VERBOSE & param->flags) {
        for (int i = 0; i < param->num_sources; ++i) {
            printf("# Heat src %d: (%f, %f) %lf %lf\n", i,
                   param->heat_sources[i].x, param->heat_sources[i].y,
                   param->heat_sources[i].range, param->heat_sources[i].temp);
        }
    }
}

static struct option long_opts[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"time", no_argument, NULL, 't'},
    {"image", required_argument, 0, 'i'},
    {0, 0, 0, 0}
};

int
main(int argc, char* argv[]) {
    int opt_idx = 0;
    int c;
    while (-1 != (c = getopt_long(argc, argv, "vti:", long_opts, &opt_idx))) {
        switch (c) {
        case 'v':
            if (HW3_FLAG_VERBOSE & hw3_params.flags)
                hw3_params.flags |= HW3_FLAG_VERY_VERBOSE;
            else
                hw3_params.flags |= HW3_FLAG_VERBOSE;
            break;
        case 't':
            hw3_params.flags |= HW3_FLAG_TIME_ALG;
            break;
        case 'i':
            hw3_params.flags |= HW3_FLAG_GENERATE_HEAT_IMAGE;
            hw3_params.vis_output = strdup(optarg);
            break;
        case '?':
            /* An error was already printed out */
            printf("something unknown (%s)\n", (NULL == optarg ? "NULL" : optarg));
            break;
        }
    }
    /* Do we have an input filename ? */
    if ((optind < 0) || (optind >= argc) ) {
        fprintf(stderr, "Missing input file\n");
        return -1;
    }
    hw3_params.input = strdup(argv[optind]);
    read_input_file(&hw3_params);
    print_params(&hw3_params);

    relaxation_params_t* rp = relaxation_init(&hw3_params);
    if (NULL == rp) {
        fprintf(stderr, "Error initializing the relaxation. Bail out!\n");
        return -1;
    }
    if (HW3_FLAG_GENERATE_HEAT_IMAGE & hw3_params.flags)  {
        relaxation_coarsen(rp, hw3_params.vis_data, hw3_params.vis_res, hw3_params.vis_res);
        dump_gray_image(hw3_params.vis_output, 0,
                        hw3_params.vis_data, hw3_params.vis_res, hw3_params.vis_res);
    }

    int iter = 0;
    double residual, start, elapsed, ignore = 0.0;

    start = wtime();
    while (1) {
        /* If we have to generate the images, let's not count this time towward
         * the relaxation time.
         */
        if (0 == (iter % hw3_params.vis_step)) {
            double s = wtime();
            if (HW3_FLAG_GENERATE_HEAT_IMAGE & hw3_params.flags) {
                relaxation_coarsen(rp, hw3_params.vis_data, hw3_params.vis_res, hw3_params.vis_res);
                /*print_matrix(hw3_params.vis_data, hw3_params.vis_res, hw3_params.vis_res);*/
                dump_gray_image(hw3_params.vis_output, iter,
                                hw3_params.vis_data, hw3_params.vis_res, hw3_params.vis_res);
            }
            if (HW3_FLAG_VERY_VERBOSE & hw3_params.flags) {
                relaxation_print(rp, stdout);
            }
            ignore += (wtime() - s);
        }

        residual = relaxation_apply(rp);
        /* print info on the residual */
        if ((HW3_FLAG_VERBOSE & hw3_params.flags) && (0 == (iter % hw3_params.vis_step))) {
            fprintf(stderr, "Residual %lf\n", residual);
        }
        if (residual < EPSILON) {  /* good enough */
            if (HW3_FLAG_VERY_VERBOSE & hw3_params.flags) {
                fprintf(stderr, "Complete after %d iterations with a residual %lf < %lf\n",
                        iter, residual, EPSILON );
            }
            break;
        }
        if ((hw3_params.max_iterations > 0) && (iter >= hw3_params.max_iterations)) {
            break;
        }
        iter++;
    }
    elapsed = wtime() - start - ignore;
    if (HW3_FLAG_TIME_ALG & hw3_params.flags) {
        printf("Completion in %ld microseconds\n", (unsigned long int)elapsed);
    }
    relaxation_fini(&rp);

    return 0;
}
