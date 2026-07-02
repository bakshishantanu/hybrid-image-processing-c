#include <stdio.h>
#include <omp.h>

int main()
{
    printf("Available threads (omp_get_max_threads): %d\n", omp_get_max_threads());

    #pragma omp parallel
    {
        #pragma omp single
        {
            printf("Threads being used in this parallel region: %d\n", omp_get_num_threads());
        }
    }
    return 0;
}