#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "header.h"
#include "jacobi.h"

hw_params_t cosc462_hw_params = {0,};

#define BUFSIZE 1024
#define EPSILON 0.000005

int read_input_file( hw_params_t* params )
{
    FILE *infile;
    char buf[BUFSIZE], *cmd;
    int i, n, ret = 0;

    params->max_iterations = -1;  /* uncountable */
    params->num_sources = 0;  /* default values */
    params->num_threads = 1;
    params->vis_step = 10000000;  /* big enough to never generate the graphical heat map */
    params->alg_type = 0;  /* pick a sensible default */
    params->num_proc_p = 1;  /* this process grid will always work */

    if( NULL == params->input )
        return -1;

    infile = fopen(params->input, "r");
    if( NULL == infile ) {
        return -1;
    }
    while( NULL != fgets(buf, BUFSIZE, infile) ) {
        if( '#' == buf[0] )
            continue;  /* leave the comments out */
        /* skip all spaces */
        cmd = buf;
        while(isspace(*cmd)) cmd++;

        if( 0 == strncmp(cmd, "iter", 4) ) {
            sscanf(cmd + 5, "%u", &params->max_iterations);
            continue;
        }
        if( 0 == strncmp(cmd, "alg", 3) ) {
            sscanf(cmd + 4, "%u", &params->alg_type);
            continue;
        }
        if( 0 == strncmp(cmd, "resolution", 10) ) {
            sscanf(cmd + 11, "%u", &params->resolution);
            continue;
        }
        if( 0 == strncmp(cmd, "threads", 7) ) {
            sscanf(cmd + 8, "%u", &params->num_threads);
            continue;
        }
        if( 0 == strncmp(cmd, "vis_res", 7) ) {
            sscanf(cmd + 8, "%u", &params->vis_res);
            continue;
        }
        if( 0 == strncmp(cmd, "vis_step", 8) ) {
            sscanf(cmd + 9, "%d", &params->vis_step);
            continue;
        }
        if( 0 == strncmp(cmd, "numsrc", 6) ) {
            sscanf(cmd + 7, "%u", &params->num_sources);
            continue;
        }
        if( 0 == strncmp(cmd, "num_proc_p", 10) ) {
            sscanf(cmd + 11, "%u", &params->num_proc_p);
            continue;
        }
        params->heat_sources = (heat_source_t*)malloc(params->num_sources * sizeof(heat_source_t));
        for( i = 0; i < params->num_sources; i++ ) {
            n = sscanf(buf, "%f %f %lf %lf",
                       &(params->heat_sources[i].x), &(params->heat_sources[i].y),
                       &(params->heat_sources[i].range), &(params->heat_sources[i].temp));
            if( 4 != n ) {
                fprintf(stderr, "Input file %s malformed (pb with the heat sources)\n",
                        params->input);
                ret = -1;
                goto complete_and_return;
            }
            /* Bail out if we are short in heat sources */
            if( (i != (params->num_sources-1)) && (NULL == fgets(buf, BUFSIZE, infile)) ) {
                fprintf(stderr, "Missing heat sources (%d != %d). Update the count\n",
                        params->num_sources, i+1);
                params->num_sources = i + 1;
                break;
            }
        }
        break;
    }
    params->vis_data = NULL;
    if( COSC462_HW_FLAG_GENERATE_HEAT_IMAGE & params->flags ) {
        params->vis_res += 2;  /* for the borders */
        params->vis_data = (double*)malloc(params->vis_res * params->vis_res * sizeof(double));
    }

 complete_and_return:
    fclose(infile);
    return ret;
}

void print_params(hw_params_t* param)
{
    int size;
    MPI_Comm_size(param->comm, &size);
    printf("# Participants %d (P = %d, Q = %d)\n",
           size, param->num_proc_p, size / param->num_proc_p);
    printf("# Max iterations ");
    if( 0 == param->max_iterations )
        printf("unrestricted\n");
    else
        printf("%u\n", param->max_iterations);
    printf("# Resolution %u\n",  param->resolution);
    printf("# Visual Resolution %u\n", param->vis_res);
    printf("# Input file %s\n", param->input);
    printf("# Num heat sources %u\n", param->num_sources);
    if( COSC462_HW_FLAG_VERBOSE & param->flags ) {
        for( int i = 0; i < param->num_sources; i++ ) {
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

int main(int argc, char* argv[] )
{
    MPI_Comm jcomm;
    int opt_idx = 0;
    int c, csize, crank;

    while( -1 != (c = getopt_long(argc, argv, "vti:", long_opts, &opt_idx)) ) {
        switch (c) {
        case 'v':
            if( COSC462_HW_FLAG_VERBOSE & cosc462_hw_params.flags )
                cosc462_hw_params.flags |= COSC462_HW_FLAG_VERY_VERBOSE;
            else
                cosc462_hw_params.flags |= COSC462_HW_FLAG_VERBOSE;
            break;
        case 't':
            cosc462_hw_params.flags |= COSC462_HW_FLAG_TIME_ALG;
            break;
        case 'i':
            cosc462_hw_params.flags |= COSC462_HW_FLAG_GENERATE_HEAT_IMAGE;
            cosc462_hw_params.vis_output = strdup(optarg);
            break;
        case '?':
            /* An error was already printed out */
            printf("something unknown (%s)\n", (NULL == optarg ? "NULL" : optarg));
            break;
        }
    }
    /* Do we have an input filename ? */
    if( (optind < 0) || (optind >= argc) ) {
        fprintf(stderr, "Missing input file\n");
        return -1;
    }
    cosc462_hw_params.input = strdup(argv[optind]);
    read_input_file(&cosc462_hw_params);

    /**
     * Initialize MPI
     */
    MPI_Init(NULL, NULL);
    MPI_Comm_dup( MPI_COMM_WORLD, &jcomm);
    MPI_Comm_size( jcomm, &csize);
    MPI_Comm_rank( jcomm, &crank);

    cosc462_hw_params.comm = jcomm;
    if( 0 == crank )  /* only the root print the banner */
        print_params(&cosc462_hw_params);

    relaxation_params_t* rp = relaxation_init(&cosc462_hw_params);
    if( NULL == rp ) {
        fprintf(stderr, "Error initializing the relaxation. Bail out!\n");
        return -1;
    }
    if( COSC462_HW_FLAG_GENERATE_HEAT_IMAGE & cosc462_hw_params.flags ) {
        relaxation_coarsen(rp, cosc462_hw_params.vis_data, cosc462_hw_params.vis_res, cosc462_hw_params.vis_res);
        dump_gray_image(cosc462_hw_params.vis_output, 0,
                        cosc462_hw_params.vis_data, cosc462_hw_params.vis_res, cosc462_hw_params.vis_res);
    }

    int iter = 0;
    double residual, start, elapsed, ignore = 0.0;

    start = wtime();
    while(1) {
        /* If we have to generate the images, let's not count this time towward
         * the relaxation time.
         */
        if( 0 == (iter % cosc462_hw_params.vis_step) ) {
            double s = wtime();
            if( COSC462_HW_FLAG_GENERATE_HEAT_IMAGE & cosc462_hw_params.flags ) {
                relaxation_coarsen(rp, cosc462_hw_params.vis_data, cosc462_hw_params.vis_res, cosc462_hw_params.vis_res);
                /*print_matrix(cosc462_hw_params.vis_data, cosc462_hw_params.vis_res, cosc462_hw_params.vis_res);*/
                dump_gray_image(cosc462_hw_params.vis_output, iter,
                                cosc462_hw_params.vis_data, cosc462_hw_params.vis_res, cosc462_hw_params.vis_res);
            }
            if( COSC462_HW_FLAG_VERY_VERBOSE & cosc462_hw_params.flags ) {
                relaxation_print(rp, stdout);
            }
            ignore += (wtime() - s);
        }

        residual = relaxation_apply(rp);
        /* print info on the residual */
        if( (COSC462_HW_FLAG_VERBOSE & cosc462_hw_params.flags) && (0 == (iter % cosc462_hw_params.vis_step)) && (0 == crank)) {
            fprintf(stderr, "Residual %lf\n", residual);
        }
        if( residual < EPSILON ) {  /* good enough */
            if( COSC462_HW_FLAG_VERY_VERBOSE & cosc462_hw_params.flags && (0 == crank)) {
                fprintf(stderr, "Complete after %d iterations with a residual %lf < %lf\n",
                        iter, residual, EPSILON );
            }
            break;
        }
        if( (cosc462_hw_params.max_iterations > 0) && (iter >= cosc462_hw_params.max_iterations) ) {
            break;
        }
        iter++;
    }
    elapsed = wtime() - start - ignore;
    if( COSC462_HW_FLAG_TIME_ALG & cosc462_hw_params.flags && (0 == crank) ) {
        printf("Completion in %ld microseconds\n", (unsigned long int)elapsed);
    }
    relaxation_fini(&rp);

    /**
     * Turn off MPI.
     */
    MPI_Finalize();

    return 0;  /* done */
}
