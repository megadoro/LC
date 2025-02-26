#include "ising.h"

int main ( int argc, char *argv[] )
{
    if( argc < 2 ){
        printf( "usage: %s <input.bin> [<input.bin>]\n", argv[0] );
        return 0;
    }
    int i;
    for(i = 1; i < argc; i++){
        FILE *fin;

        if( !(fin = fopen(argv[i], "rb") ) ){
            printf("Error opening file: %s\n", argv[i]);
            break;
        }
        raw storage = load_data(fin,0,1000);
        fclose(fin);
        printf( "\nReading: %s\n", argv[i]);

        int step = 1;
        if(!storage.id) step = 20;

        double mean = 0.0f;
        int t, k, k_max;
        for(t = 0; t < storage.size; t++)
            mean += storage.data[t];
        mean /= storage.size;
        for(t = 0; t < storage.size; t++)
            storage.data[t] -= mean;
        double variance = 0.0f;
        for(t = 0; t < storage.size; t++)
            variance += storage.data[t] * storage.data[t];
        variance /= storage.size;

        char output[50];
        sprintf(output, "data/%d_%f_%s_%d.acr", storage.l, storage.b, storage.algorithm, storage.size );
        FILE *fout = fopen(output,"a");
        fprintf(fout , "#\t%d\t%f\t%s\t%d\n", storage.l, storage.b, storage.algorithm, storage.size );

        double autocorrelation, autocorrelation_time;
        for(k_max = 0; k_max < 100 * step; k_max += step){
            autocorrelation_time = 0.5f;
            for(k = 0; k < k_max; k++){
                autocorrelation = 0.0f;
                for(t = 0; t < storage.size - k; t++)
                    autocorrelation += storage.data[t] * storage.data[t+k];
                autocorrelation /= variance * (storage.size - k);
                autocorrelation_time += autocorrelation;
            }
            fprintf(fout, "%d\t%e\n", k_max, autocorrelation_time);
        }
        printf("Written to: %s\n", output);
        raw_close(&storage);
        fclose(fout);
    }
    return 0;
}

