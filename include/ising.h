#ifndef ISING
#define ISING

#include <stdio.h>
#include <stdlib.h>
#include "mersenne.h"
#include <time.h>
#include <string.h>
#include <math.h>

#define BETA_CRITICAL   (log(1.0f+sqrt(2.0f))/2.0f)

#define RIGHT  i][(j+1)%SIZE
#define LEFT   i][(SIZE+j-1)%SIZE
#define UP     (SIZE+i-1)%SIZE][j
#define DOWN   (i+1)%SIZE][j
#define CENTER i][j

int SIZE = 32;
double BETA = 0.0f;

typedef struct _spin{
    int s;
    int link[4];
    int cl;
    struct _spin *next;
}spin;

spin **ising = NULL;
spin **cluster = NULL;

int merge_clusters(int a, int b)
{
    if(a != b){
        spin *tmp;
        for(tmp = cluster[b]; tmp->next != NULL; tmp = tmp->next)
            tmp->cl = a;
        tmp->cl = a;
        tmp->next = cluster[a];
        cluster[a] = cluster[b];
        cluster[b] = NULL;
    }
    return a;
}

void create_clusters()
{
    int i,j;
    /* activate links */
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++){
        ising[CENTER].link[0] = ising[RIGHT].link[2] = ( (ising[CENTER].s==ising[RIGHT].s)&&(mersenne()<(1-exp(-2*BETA))) );
        ising[CENTER].link[1] = ising[UP   ].link[3] = ( (ising[CENTER].s==ising[UP   ].s)&&(mersenne()<(1-exp(-2*BETA))) );
        ising[CENTER].link[2] = ising[LEFT ].link[0] = ( (ising[CENTER].s==ising[LEFT ].s)&&(mersenne()<(1-exp(-2*BETA))) );
        ising[CENTER].link[3] = ising[DOWN ].link[1] = ( (ising[CENTER].s==ising[DOWN ].s)&&(mersenne()<(1-exp(-2*BETA))) );
    }
    /* initialize every spin as its own cluster */
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++){
        ising[CENTER].cl = i*SIZE+j;
        cluster[ising[CENTER].cl] = &ising[CENTER];
        ising[CENTER].next = NULL;
    }
    /* merge linked clusters */
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++){
/*        if( ising[CENTER].link[0] ) merge_clusters(ising[CENTER].cl, ising[RIGHT].cl);*/
        if( ising[CENTER].link[1] ) merge_clusters(ising[CENTER].cl, ising[UP].cl);
        if( ising[CENTER].link[2] ) merge_clusters(ising[CENTER].cl, ising[LEFT].cl);
/*        if( ising[CENTER].link[3] ) merge_clusters(ising[CENTER].cl, ising[DOWN ].cl);*/
    }
}

void SW()
{
    int k;
    spin *tmp;
    /* flip clusters */
    for(k = 0; k < SIZE * SIZE; k++)
        if( mersenne() < 0.5f )
            for( tmp = cluster[k]; tmp != NULL; tmp = tmp->next )
                tmp->s *= -1;
    create_clusters();
}

void MH()
{
    int i,j,delta_energy;
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++)
        if( mersenne() < 0.5f ){
            delta_energy = 2*(ising[RIGHT].s+ising[UP].s+ising[LEFT].s+ising[DOWN].s)*ising[CENTER].s;
            if( (delta_energy <= 0)||(mersenne() < exp(- BETA * delta_energy)) )
                ising[CENTER].s = -ising[CENTER].s;
        }
}

void clear()
{
    if(ising){
        int i;
        for(i = 0; i < SIZE; i++)
            if(ising[i])
                free(ising[i]);
        free(ising);
        ising = NULL;
    }
    if(cluster){
        free(cluster);
        cluster = NULL;
    }
}

void init(int lattice_size, double beta_value)
{
    SIZE = lattice_size;
    BETA = beta_value;

    int i,j,k;
    seed_mersenne( (long)time(NULL) );
    for(k = 0; k < 543210; k++) mersenne();

    clear();
    cluster = (spin**)malloc(SIZE*SIZE*sizeof(spin*));
    ising = (spin**)malloc(SIZE * sizeof(spin*));
    for(i = 0; i < SIZE; i++){
        ising[i] = (spin*)malloc(SIZE * sizeof(spin));
        for(j = 0; j < SIZE; j++)
            ising[i][j].s = 1;
    }

    create_clusters();
}

double get_beta_critical()
{
    return BETA_CRITICAL;
}

char *get_algorithm_string(void (*algorithm)())
{
    if( algorithm == MH ) return "MH";
    if( algorithm == SW ) return "SW";
    return NULL;
}

double get_energy()
{
    int i, j, energy = 0;
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++)
        energy += - ( ising[UP].s + ising[LEFT].s ) * ising[CENTER].s;
    return 1.0f * energy;
}

double get_signed_magnetization()
{
    int i, j, magnetization = 0;
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++)
        magnetization += ising[CENTER].s;
    return 1.0f * magnetization;
}

double get_magnetization()
{
    int i, j, magnetization = 0;
    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++)
        magnetization += ising[CENTER].s;
    return 1.0f * abs(magnetization);
}

int get_largest_cluster()
{
    int k;
    int cl_size, cl_size_max = 0;
    spin *tmp;
    for(k = 0; k < SIZE * SIZE; k++){
        cl_size = 0;
        for(tmp = cluster[k]; tmp != NULL; tmp = tmp->next)
            cl_size++;
        if(cl_size > cl_size_max)
            cl_size_max = cl_size;
    }
    return cl_size_max;
}

double get_correlation(int dist)
{
    double *Sx = (double*)malloc(SIZE * sizeof(double));
    double *Sy = (double*)malloc(SIZE * sizeof(double));
    int i,j;
    for(i = 0; i < SIZE; i++)
        Sx[i] = Sy[i] = 0.0f;

    for(i = 0; i < SIZE; i++) for(j = 0; j < SIZE; j++){
        Sx[j] += ising[i][j].s;
        Sy[i] += ising[i][j].s;
    }
    for(i = 0; i < SIZE; i++){
        Sx[i] /= SIZE;
        Sy[i] /= SIZE;
    }

    double correlation = 0.0f;
    for(i = 0; i < SIZE; i++){
        correlation += Sx[i] * Sx[(i+dist)%SIZE] / (Sx[i] * Sx[i]);
        correlation += Sx[i] * Sx[(SIZE+i-dist)%SIZE] / (Sx[i] * Sx[i]);
        correlation += Sy[i] * Sy[(i+dist)%SIZE] / (Sy[i] * Sy[i]);
        correlation += Sy[i] * Sy[(SIZE+i-dist)%SIZE] / (Sy[i] * Sy[i]);
    }
    free(Sx);
    free(Sy);
    return correlation / (4 * SIZE);
}

void dump_data(int lattice_size, double beta_value, void (*algorithm)(), int run_time)
{
    if( (algorithm != MH) && (algorithm != SW) ){ printf("\nInvalid Algorithm!\n"); }
    else{
        int id = (algorithm == SW);
        /* .bin header */
        char filename_bin[50];
        sprintf(filename_bin, "data/%d_%f_%s_%d.bin", lattice_size, beta_value, get_algorithm_string(algorithm), run_time);
        FILE *f_bin = fopen(filename_bin, "wb");
        int cols = 3+lattice_size/2;
        fwrite(&cols, sizeof(int), 1, f_bin);
        fwrite(&lattice_size, sizeof(int), 1, f_bin);
        fwrite(&beta_value, sizeof(double), 1, f_bin);
        fwrite(&id, sizeof(int), 1, f_bin);
        fwrite(&run_time, sizeof(int), 1, f_bin);
        /* data gathering */
        printf("\nExecuting %s : L = %d : β = %f : time = %d\n", get_algorithm_string(algorithm), lattice_size, beta_value, run_time);
        init(lattice_size, beta_value);
        double tmp; int t, dist;
        for(t = 0; t < run_time; t++){
            tmp = get_energy() / (lattice_size * lattice_size);
            fwrite(&tmp, sizeof(double), 1, f_bin);

            tmp = get_magnetization() / (lattice_size * lattice_size);
            fwrite(&tmp, sizeof(double), 1, f_bin);

            if(algorithm == MH){
                fwrite(&tmp, sizeof(double), 1, f_bin);
            }
            if(algorithm == SW){
                tmp = (get_largest_cluster() * 1.0f) / (lattice_size * lattice_size);
                fwrite(&tmp, sizeof(double), 1, f_bin);
            }
            for(dist = 0; dist < lattice_size / 2; dist++){
                tmp = get_correlation(dist);
                fwrite(&tmp, sizeof(double), 1, f_bin);
            }

            algorithm();
        }
        fclose(f_bin);
        clear();
    }
}

typedef struct _raw{
    int l;              /* lattice size */
    double b;           /* beta */
    int id;             /* algorithm id */
    char *algorithm;    /* algorithm name */
    int size;           /* number of samples */
    double *data;       /* samples */
}raw;

void raw_close(raw *obj)
{
    if(obj->data != NULL) free(obj->data);
    obj->l = 0;
    obj->b = 0.0f;
    obj->id = 0;
    obj->algorithm = NULL;
    obj->size = 0;
    obj->data = NULL;
}

raw load_data(FILE *f, int column, int skip)
{
    fseek(f, 0L, SEEK_SET);
    raw content = { 0, 0.0f, 0, NULL, 0, NULL };
    int cols; fread(&cols, sizeof(int), 1, f );
    if((column < 0)||(column >= cols)||(skip < 0)) return content;
    /* read lattice size */
    fread(&content.l, sizeof(int), 1, f);
    /* read beta value */
    fread(&content.b, sizeof(double), 1, f);
    /* read algorithm id and name */
    fread(&content.id, sizeof(int), 1, f);
    if(content.id == 0) content.algorithm = "MH";
    if(content.id == 1) content.algorithm = "SW";
    /* read size */
    fread(&content.size, sizeof(int), 1, f);
    /* skip initial data */
    fseek(f, cols * sizeof(double) * skip, SEEK_CUR);
    content.size -= skip;
    /* read data */
    content.data = (double*)malloc(content.size * sizeof(double));
    int t;
    for(t = 0; t < content.size; t++){
        fseek(f, column * sizeof(double), SEEK_CUR);
        fread(&content.data[t], sizeof(double), 1, f);
        fseek(f, ((cols-1)-column) * sizeof(double), SEEK_CUR);
    }
    fseek(f, 0L, SEEK_SET);
    return content;
}

typedef struct _header{
    int cols;           /* number of columns */
    int l;              /* lattice size */
    double b;           /* beta */
    int id;             /* algorithm id */
    char *algorithm;    /* algorithm name */
    int size;           /* number of samples */
}header;

header get_header(FILE *f)
{
    header hdr;
    fseek(f, 0L, SEEK_SET);
    /* read number of columns */
    fread(&hdr.cols, sizeof(int), 1, f );
    /* read lattice size */
    fread(&hdr.l, sizeof(int), 1, f);
    /* read beta value */
    fread(&hdr.b, sizeof(double), 1, f);
    /* read algorithm id and name */
    fread(&hdr.id, sizeof(int), 1, f);
    if(hdr.id == 0) hdr.algorithm = "MH";
    if(hdr.id == 1) hdr.algorithm = "SW";
    /* read size */
    fread(&hdr.size, sizeof(int), 1, f);
    return hdr;
}

double *bin_data(double *storage, int storage_size, int bin_size)
{
    if (storage_size < bin_size) return NULL;

    int n_bins = storage_size / bin_size;
    storage_size = bin_size * n_bins;

    double *binned_data = (double*)malloc(n_bins * sizeof(double));
    int t;
    for(t = 0; t < n_bins; t++)
        binned_data[t] = 0.0f;
    for(t = 0; t < storage_size; t++)
        binned_data[t/bin_size] += storage[t];
    for(t = 0; t < n_bins; t++)
        binned_data[t] /= bin_size;
    return binned_data;
}

double *jackknife(double *storage, int storage_size, int bin_size)
{
    if (storage_size < bin_size) return NULL;

    int n_bins = storage_size / bin_size;
    storage_size = bin_size * n_bins;

    int t;
    double sum = 0.0f;
    double *binned_data = (double*)malloc(n_bins * sizeof(double));
    for(t = 0; t < storage_size; t++)
        sum += storage[t];
    for(t = 0; t < n_bins; t++)
        binned_data[t] = 0.0f;
    for(t = 0; t < storage_size; t++)
        binned_data[t/bin_size] += storage[t];
    for(t = 0; t < n_bins; t++)
        binned_data[t] = (sum - binned_data[t]) / ( storage_size - bin_size );
    return binned_data;
}

int get_bin_size(int ID, int lattice_size)
{
    if(ID == 0) return 1000;
    if(ID == 1) return 50;
    return 1;

}
#endif
