/**
 *
 * Set Heap Size in ldscript.ld to 0x1000000 (16MB)
 *
 */

#define PROFILING

#include "xanomaly_detector_axi.h"  /* accelerator */
#include "unistd.h"      /* sleep */
#include "stdlib.h"
#include "malloc.h"
#include "xil_io.h"      /* peripheral read/write wrappers */
#include "platform.h"    /* platform init/cleanup functions */
#include "xil_cache.h"   /* enable/disable caches etc */
#include "xil_printf.h"  /* UART debug print functions */
#include "xparameters.h" /* peripherals base addresses */

#ifdef PROFILING
#include "xtmrctr.h"     /* timer */

XTmrCtr TimerCounterInst;
#define TMRCTR_DEVICE_ID	XPAR_TMRCTR_0_DEVICE_ID
#define TIMER_CNTR_0		0
#endif

#include "src.h"
#include "dst.h"

#define __DEBUG__

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define SHORT_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define SHORT_TO_BINARY(byte)  \
  (byte & 0x8000 ? '1' : '0'), \
  (byte & 0x4000 ? '1' : '0'), \
  (byte & 0x2000 ? '1' : '0'), \
  (byte & 0x1000 ? '1' : '0'), \
  (byte & 0x800 ? '1' : '0'), \
  (byte & 0x400 ? '1' : '0'), \
  (byte & 0x200 ? '1' : '0'), \
  (byte & 0x100 ? '1' : '0'), \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')


#define MAX_PRINT_ELEMENTS (4)

#define ITERATION_FACTOR (1)

const unsigned INPUT_N_ELEMENTS = src_SAMPLE_COUNT*src_FEATURE_COUNT;
const unsigned OUTPUT_N_ELEMENTS = dst_SAMPLE_COUNT*dst_FEATURE_COUNT;

#if 0
/* base address for the accelerator */
#define MEM_BASE_ADDR XPAR_PS7_DDR_0_S_AXI_BASEADDR

/* data offsets and pointers */
#define SRC_BUFFER_BASE (MEM_BASE_ADDR + 0x00000000)
unsigned char *src_mem = (unsigned char*)SRC_BUFFER_BASE;

#define GLD_BUFFER_BASE (MEM_BASE_ADDR + 0x00010000)
unsigned char *gld_mem = (unsigned char*)GLD_BUFFER_BASE;

#define DST_BUFFER_BASE (MEM_BASE_ADDR + 0x00020000)
unsigned char *dst_mem = (unsigned char*)DST_BUFFER_BASE;
#else
unsigned char *src_mem;
unsigned char *gld_mem;
unsigned char *dst_mem;
#endif

/* accelerator configuration */
XAnomaly_detector_axi do_anomaly_detector;
XAnomaly_detector_axi_Config *do_anomaly_detector_cfg;

/* accelerator initialization routine */
void init_accelerators()
{
    xil_printf("INFO: Initializing accelerator\n\r");
    do_anomaly_detector_cfg = XAnomaly_detector_axi_LookupConfig(XPAR_ANOMALY_DETECTOR_AXI_DEVICE_ID);
    if (do_anomaly_detector_cfg)
    {
        int status  = XAnomaly_detector_axi_CfgInitialize(&do_anomaly_detector, do_anomaly_detector_cfg);
        if (status != XST_SUCCESS)
        {
            xil_printf("ERROR: Initializing accelerator\n\r");
        }
    }
}

//#if defined(__HPC_ACCELERATOR__) || defined(__ACP_ACCELERATOR__)
///*
// *  TODO: remember to edit core_baremetal_polling_bsp/psu_cortexa53_0/libsrc/standalon_v6_5/src/bspconfig.h
// *
// *  #define EL1_NONSECURE 1
// *
// */
//void init_accelerator_coherency(UINTPTR base_addr)
//{
//    /* Enable snooping of APU caches from CCI */
//    Xil_Out32(0xFD6E4000, 0x1);
//
//    /* Configure AxCACHE for write-back read and write-allocate (ARCACHE is [7:4], AWCACHE is [11:8]) */
//    /* Configure AxPROT[2:0] for data access [2], secure access [1], unprivileged access [0] */
//    Xil_Out32(base_addr, 0xFF0);
//}
//#endif

/* golden model of the accelerator in software */
int anomaly_detector_sw(unsigned char *src, unsigned char *dst, unsigned input_n_elements, unsigned output_n_elements)
{
    xil_printf("INFO: Golden results are pre-compiled. It would be nice to run a software model here.\n\r");
    // See src.h and dst.h for input and golden output respectively.
    return 0;
}

#ifdef PROFILING
/* profiling function */
u64 get_elapsed_time_ns(u64 clks)
{
    return clks * 1000000000/XPAR_AXI_TIMER_MCU_CLOCK_FREQ_HZ;
}
#endif

/* dump data to the console */
void dump_data(const char* label, unsigned char* data, unsigned sample_count, unsigned feature_count, unsigned print_hex, unsigned print_bin)
{
    xil_printf("INFO:   %s[%u][%u]:\n\r", label, sample_count, feature_count);
    /* print at most MAX_PRINT_ELEMENTS */
    for (unsigned i = 0; i < sample_count && i < MAX_PRINT_ELEMENTS; i++)
    {
        xil_printf("INFO:     [%u] ", i);
        if (print_hex)
            for (unsigned j = 0; j < feature_count; j++)
            {
                unsigned index = i * feature_count + j;
                xil_printf("%02X ", data[index]);
            }
        if (print_bin)
            for (unsigned j = 0; j < feature_count; j++)
            {
                unsigned index = i * feature_count + j;
                xil_printf(""BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[index]));
                xil_printf(" ");
            }
        xil_printf("\n\r");
    }
    for (unsigned i = sample_count - MAX_PRINT_ELEMENTS; i < sample_count; i++)
    {
        xil_printf("INFO:     [%u] ", i);
        if (print_hex)
            for (unsigned j = 0; j < feature_count; j++)
            {
                unsigned index = i * feature_count + j;
                xil_printf("%02X ", data[index]);
            }
        if (print_bin)
            for (unsigned j = 0; j < feature_count; j++)
            {
                unsigned index = i * feature_count + j;
                xil_printf(""BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(data[index]));
                xil_printf(" ");
            }
        xil_printf("\n\r");
    }
}

/* the top of the hill :-) */
int main(int argc, char** argv)
{
#ifdef PROFILING
    u32 calibration_time = 0;
    u32 sw_elapsed = 0, hw_elapsed = 0, cache_elapsed = 0;
#endif

    int status;
    char __attribute__ ((unused)) dummy; /* dummy input */

    int hw_errors;

    xil_printf("\n\r");
    xil_printf("INFO: ===============================================\n\r");
    xil_printf("INFO: Anomaly Detector (w/ polling)\n\r");
    xil_printf("INFO: ===============================================\n\r");

    /* initialize platform (uart and caches) */
    init_platform();

    init_accelerators();

#ifdef PROFILING
    /* Timer Counter */
    status = XTmrCtr_Initialize(&TimerCounterInst, TMRCTR_DEVICE_ID);
    if (status != XST_SUCCESS){
    	print("ERROR: Timer counter initialization failed \r\n");
    	return status;
    }
    print("INFO: Timer counter initialized\r\n");
#endif

    src_mem = malloc(INPUT_N_ELEMENTS * sizeof(unsigned char));
    dst_mem = malloc(OUTPUT_N_ELEMENTS * sizeof(unsigned char));
    gld_mem = malloc(OUTPUT_N_ELEMENTS * sizeof(unsigned char));

    /* calibration */
#ifdef PROFILING
    XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_0);
    sleep(3);
    XTmrCtr_Stop(&TimerCounterInst, TIMER_CNTR_0);
    calibration_time = XTmrCtr_GetValue(&TimerCounterInst, TIMER_CNTR_0);
    xil_printf("INFO: Time calibration for three seconds: %u clk, %u ns\n\r", calibration_time, get_elapsed_time_ns(calibration_time));
#endif

    /* initialize memory */
    xil_printf("INFO: Initialize memory\n\r");
    xil_printf("INFO:   - Sample count: %u\n\r", src_SAMPLE_COUNT); /* Same as dst_SAMPLE_COUNT */
    xil_printf("INFO:   - Input-feature count: %u\n\r", src_FEATURE_COUNT);
    xil_printf("INFO:   - Output-class count: %u\n\r", dst_FEATURE_COUNT);
    xil_printf("INFO:   - Data size: %u B\n\r", sizeof(unsigned char));
    xil_printf("INFO:   - Total input size: %u B\n\r", src_FEATURE_COUNT * src_SAMPLE_COUNT * sizeof(unsigned char));
    xil_printf("INFO:   - Total output size: %u B\n\r", dst_FEATURE_COUNT * dst_SAMPLE_COUNT * sizeof(unsigned char));

    // Set Heap Size in ldscript.ld to 0x1000000 (16MB)
    //malloc_stats();

    for (int i = 0; i < INPUT_N_ELEMENTS; i++) {
        src_mem[i] = src_data[i];
    }
    for (int i = 0; i < OUTPUT_N_ELEMENTS; i++) {
        gld_mem[i] = dst_data[i];
        dst_mem[i] = 0xFF;
    }

    /* ****** SOFTWARE REFERENCE ****** */
#ifdef __DEBUG__
    xil_printf("INFO: Start SW accelerator\n\r");
#endif
#ifdef PROFILING
    XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_0);
#endif
    anomaly_detector_sw(src_mem, gld_mem, INPUT_N_ELEMENTS, OUTPUT_N_ELEMENTS);
#ifdef PROFILING
    XTmrCtr_Stop(&TimerCounterInst, TIMER_CNTR_0);
    sw_elapsed = XTmrCtr_GetValue(&TimerCounterInst, TIMER_CNTR_0);
#endif
#if 0
#ifdef __DEBUG__
    unsigned invocation_count = ITERATION_FACTOR*src_SAMPLE_COUNT;
    xil_printf("INFO: Number of accelerator invocations: %u\n\r", invocation_count);
    xil_printf("INFO:   - Iteration factor: %u\n\r", ITERATION_FACTOR);
    xil_printf("INFO:   - Sample count : %u\n\r", src_SAMPLE_COUNT);
#endif
#endif
    /* ****** ACCELERATOR ****** */
    xil_printf("INFO: Press any key to start the accelerator: ");
    dummy = inbyte();
    xil_printf("\n\rINFO: \n\r");
#ifdef __DEBUG__
    xil_printf("INFO: Configure and start accelerator\n\r");
#endif

#ifdef PROFILING
    XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_0);
#endif
    Xil_DCacheFlushRange((UINTPTR)src_mem, INPUT_N_ELEMENTS * sizeof(unsigned char));
    Xil_DCacheFlushRange((UINTPTR)dst_mem, OUTPUT_N_ELEMENTS * sizeof(unsigned char));
    Xil_DCacheFlushRange((UINTPTR)gld_mem, OUTPUT_N_ELEMENTS * sizeof(unsigned char));
#ifdef PROFILING
    XTmrCtr_Stop(&TimerCounterInst, TIMER_CNTR_0);
    cache_elapsed = XTmrCtr_GetValue(&TimerCounterInst, TIMER_CNTR_0);
#endif

    for (unsigned j = 0; j < ITERATION_FACTOR; j++) {

    	unsigned char *src_mem_i = src_mem;
    	unsigned char *dst_mem_i = dst_mem;

    	for (unsigned i = 0; i < src_SAMPLE_COUNT; i++) {

    		/* Configure the accelerator */
#ifdef PROFILING
    		XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_0);
#endif
    		XAnomaly_detector_axi_Set_in_V(&do_anomaly_detector, (unsigned)src_mem_i);
    		XAnomaly_detector_axi_Set_out_V(&do_anomaly_detector, (unsigned)dst_mem_i);
    		XAnomaly_detector_axi_Set_batch(&do_anomaly_detector, dst_SAMPLE_COUNT);

    		XAnomaly_detector_axi_Start(&do_anomaly_detector);

    		/* polling */
    		while (!XAnomaly_detector_axi_IsDone(&do_anomaly_detector));

    		/* get error status */
    		//hw_flags = XAnomaly_detector_axi_Get_return(&do_anomaly_detector);
#ifdef PROFILING
    		XTmrCtr_Stop(&TimerCounterInst, TIMER_CNTR_0);
    		hw_elapsed += XTmrCtr_GetValue(&TimerCounterInst, TIMER_CNTR_0);
#endif

    		src_mem_i += src_FEATURE_COUNT;
    		dst_mem_i += dst_FEATURE_COUNT;
    	}
    }

#ifdef PROFILING
    XTmrCtr_Start(&TimerCounterInst, TIMER_CNTR_0);
#endif
    Xil_DCacheFlushRange((UINTPTR)dst_mem, OUTPUT_N_ELEMENTS * sizeof(unsigned char));
#ifdef PROFILING
    XTmrCtr_Stop(&TimerCounterInst, TIMER_CNTR_0);
    cache_elapsed += XTmrCtr_GetValue(&TimerCounterInst, TIMER_CNTR_0);
#endif

    /* ****** VALIDATION ****** */

#ifdef __DEBUG__
    xil_printf("INFO: ================== Validation =================\n\r");
    xil_printf("INFO: Dump data\n\r");
    dump_data("src", (unsigned char*)src_mem, src_SAMPLE_COUNT, src_FEATURE_COUNT, 1, 0);
    dump_data("sw_dst", (unsigned char*)gld_mem, dst_SAMPLE_COUNT, dst_FEATURE_COUNT, 1, 0);
    dump_data("hw_dst", (unsigned char*)dst_mem, dst_SAMPLE_COUNT, dst_FEATURE_COUNT, 1, 0);
#endif
#ifdef PROFILING
    //xil_printf("INFO: Software execution time: %u clk, %u ns\n\r", sw_elapsed, get_elapsed_time_ns(sw_elapsed));
    xil_printf("INFO: Accelerator execution time: %u clk, %u ns\n\r", hw_elapsed, get_elapsed_time_ns(hw_elapsed));
    //xil_printf("INFO: Cache flush time: %u clk, %u ns\n\r", cache_elapsed, get_elapsed_time_ns(cache_elapsed));
    //xil_printf("INFO: Accelerator/software speedup (the sofware is fake so this does not count...): %.2f X\n\r", (sw_elapsed >= (hw_elapsed+cache_elapsed))?(sw_elapsed/(hw_elapsed+cache_elapsed)):-((hw_elapsed+cache_elapsed)/sw_elapsed));
#endif

    /* Accelerator validation */
    hw_errors = 0;
    for (int i = 0; i < OUTPUT_N_ELEMENTS; i++)
    {
        if (dst_mem[i] != gld_mem[i])
        {
            xil_printf("ERROR: [%d]: Accelerator hw %02X != sw %02X\n\r", i, dst_mem[i], gld_mem[i]);
            hw_errors++;
        }
    }
    xil_printf("INFO: Total errors = %d (out of %d elements)\n\r", hw_errors, OUTPUT_N_ELEMENTS);
    if (hw_errors > 0)
        xil_printf("INFO: Accelerator validation: FAIL\n\r");
    else
        xil_printf("INFO: Accelerator validation: PASS!\n\r");

    xil_printf("INFO: done!\n\r");
    xil_printf("INFO: ===============================================\n\r");

    cleanup_platform();

    return 0;
}


