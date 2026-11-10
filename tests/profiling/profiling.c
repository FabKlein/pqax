#include <hal.h>
#include "profiling.h"
#include "prefix_ubenchs.h"
#include <stdlib.h>
#include <string.h>

static uint64_t target_frequency_mhz = 32;

void profiling_set_target_frequency_mhz(uint64_t frequency_mhz)
{
    target_frequency_mhz = frequency_mhz;
}

static uint64_t elapsed_to_cycles(uint64_t elapsed)
{
#if defined(NANO_CYCLES)
    /* nanoseconds * MHz / 1000, arranged to avoid overflow */
    return (elapsed / 1000) * target_frequency_mhz
           + ((elapsed % 1000) * target_frequency_mhz) / 1000;
#else
    return elapsed;
#endif
}

static int cmp_uint64_t(const void *a, const void *b)
{
    return (int)((*((const uint64_t *)a)) - (*((const uint64_t *)b)));
}

profiling_statistics_t measure_statistics(ubench_t func)
{
    uint64_t t0, t1;
    uint64_t cycles[TEST_COUNT];
    uint64_t sum = 0;
    profiling_statistics_t statistics;

    uint32_t data0[2048] __attribute__((aligned(16)));
    uint32_t data1[2048] __attribute__((aligned(16)));
    uint32_t data2[2048] __attribute__((aligned(16)));
    uint32_t data3[2048] __attribute__((aligned(16)));
    uint32_t data4[2048] __attribute__((aligned(16)));
    for( unsigned cnt=0; cnt < WARMUP_ITERATIONS; cnt++ )
        func ( data0 + 1024, data1 + 1024,
               data2 + 1024, data3 + 1024,
               data4 + 1024 );

    for( unsigned cnt=0; cnt < TEST_COUNT; cnt++ )
    {
        t0 = get_cyclecounter();
        for( unsigned cntp=0; cntp < ITER_PER_TEST; cntp++ )
        {
            func( data0 + 1024, data1 + 1024,
                  data2 + 1024, data3 + 1024,
                  data4 + 1024 );
        }
        t1 = get_cyclecounter();
        cycles[cnt] = elapsed_to_cycles(t1 - t0) / ITER_PER_TEST;
        sum += cycles[cnt];
    }

    /* Report statistics in cycles per benchmark pattern. */
    qsort( cycles, TEST_COUNT, sizeof(uint64_t), cmp_uint64_t );
    statistics.min = (unsigned int)(cycles[0] / UBENCH_PATTERN_REPEATS);
    statistics.max = (unsigned int)(cycles[TEST_COUNT - 1] /
                                    UBENCH_PATTERN_REPEATS);
    statistics.average = (unsigned int)(sum /
                                        (TEST_COUNT * UBENCH_PATTERN_REPEATS));
    statistics.median = (unsigned int)(cycles[TEST_COUNT >> 1] /
                                       UBENCH_PATTERN_REPEATS);
    return statistics;
}

unsigned int measure(ubench_t func)
{
    return measure_statistics(func).median;
}

static unsigned int subtract_saturating(unsigned int value,
                                        unsigned int baseline)
{
    return value > baseline ? value - baseline : 0;
}

static profiling_statistics_t subtract_baseline(
    profiling_statistics_t statistics, unsigned int baseline)
{
    statistics.min = subtract_saturating(statistics.min, baseline);
    statistics.max = subtract_saturating(statistics.max, baseline);
    statistics.average = subtract_saturating(statistics.average, baseline);
    statistics.median = subtract_saturating(statistics.median, baseline);
    return statistics;
}

void print_line_with_star( unsigned len, unsigned pos )
{

}

void profile_full()
{
    const unsigned maxchars = 80;
    char tmp[maxchars + 1];
    profiling_statistics_t calibration =
        measure_statistics(ubenchs_prefix[0]);

    debug_printf( "===== Stepwise profiling =======\n");
    debug_printf("Calibration (empty loop): "
                 "[min: %u, max: %u, avg: %u, med: %u]\n",
                 calibration.min, calibration.max,
                 calibration.average, calibration.median);
    for( unsigned int i=0; i < num_ubenchs_prefix; i++ )
    {
        profiling_statistics_t statistics =
            measure_statistics(ubenchs_prefix[i + 1]);
        statistics = subtract_baseline(statistics, calibration.median);
        unsigned int star_position = statistics.median % maxchars;
        memset( tmp, '.', maxchars );
        tmp[star_position] = '*';
        tmp[maxchars] = '\0';
        debug_printf("[%3u]: %60s %s "
                     "[min: %u, max: %u, avg: %u, med: %u]\n",
                     i, ubench_prefix_instructions[i], tmp,
                     statistics.min, statistics.max,
                     statistics.average, statistics.median);
    }
}
