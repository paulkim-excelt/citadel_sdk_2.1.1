/* Generated by Kconfiglib (https://github.com/ulfalizer/Kconfiglib) */
#define CONFIG_BOARD "citadel_svk_58201"
#define CONFIG_UART_NS16550_PORT_0 1
#define CONFIG_UART_NS16550_PORT_0_NAME "UART_0"
#define CONFIG_UART_NS16550_PORT_0_IRQ_PRI 0
#define CONFIG_UART_NS16550_PORT_0_BAUD_RATE 115200
#define CONFIG_UART_NS16550_PORT_0_OPTIONS 0
#define CONFIG_DMA_IRQ_PRI 0
#define CONFIG_SOC "brcm_bcm5820x"
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 26000000
#define CONFIG_UART_NS16550 1
#define CONFIG_ISR_STACK_SIZE 2048
#define CONFIG_SYS_CLOCK_TICKS_PER_SEC 1000
#define CONFIG_TEXT_SECTION_OFFSET 0x0
#define CONFIG_FLASH_SIZE 8192
#define CONFIG_FLASH_BASE_ADDRESS 0x63000000
#define CONFIG_SRAM_SIZE 1024
#define CONFIG_SRAM_BASE_ADDRESS 0x24000000
#define CONFIG_BOARD_CITADEL_SVK_58201 1
#define CONFIG_EXTRA_ROM_CODE_OFFSET 0x0
#define CONFIG_SOC_BCM5820X 1
#define CONFIG_RAM_RESERVED_FOR_POWER_MGMT 64
#define CONFIG_SOC_LOG_LEVEL_INF 1
#define CONFIG_SOC_LOG_LEVEL 3
#define CONFIG_CPU_CORTEX 1
#define CONFIG_CPU_CORTEX_A 1
#define CONFIG_FP_HARDABI 1
#define CONFIG_ISA_THUMB2 1
#define CONFIG_LDREX_STREX_AVAILABLE 1
#define CONFIG_DATA_ENDIANNESS_LITTLE 1
#define CONFIG_STACK_ALIGN_DOUBLE_WORD 1
#define CONFIG_FAULT_DUMP 2
#define CONFIG_XIP 1
#define CONFIG_GEN_ISR_TABLES 1
#define CONFIG_ARMV7_A 1
#define CONFIG_ARM_GIC_400 1
#define CONFIG_GIC_INIT_PRIORITY 0
#define CONFIG_CPU_CORTEX_A7 1
#define CONFIG_BOOT_LOADER_IN_ARM_STATE 1
#define CONFIG_STACK_DUMP_SIZE_ON_FAULT 0
#define CONFIG_ARM_CORE_MMU 1
#define CONFIG_DATA_CACHE_SUPPORT 1
#define CONFIG_ARCH_HAS_THREAD_ABORT 1
#define CONFIG_ARCH "arm"
#define CONFIG_ARM 1
#define CONFIG_ARCH_LOG_LEVEL_INF 1
#define CONFIG_ARCH_LOG_LEVEL 3
#define CONFIG_MPU_LOG_LEVEL_INF 1
#define CONFIG_MPU_LOG_LEVEL 3
#define CONFIG_GEN_SW_ISR_TABLE 1
#define CONFIG_GEN_IRQ_START_VECTOR 0
#define CONFIG_ARCH_HAS_RAMFUNC_SUPPORT 1
#define CONFIG_CPU_HAS_FPU 1
#define CONFIG_FLOAT 1
#define CONFIG_KERNEL_LOG_LEVEL_INF 1
#define CONFIG_KERNEL_LOG_LEVEL 3
#define CONFIG_MULTITHREADING 1
#define CONFIG_NUM_COOP_PRIORITIES 16
#define CONFIG_NUM_PREEMPT_PRIORITIES 15
#define CONFIG_MAIN_THREAD_PRIORITY 0
#define CONFIG_COOP_ENABLED 1
#define CONFIG_PREEMPT_ENABLED 1
#define CONFIG_PRIORITY_CEILING 0
#define CONFIG_NUM_METAIRQ_PRIORITIES 0
#define CONFIG_MAIN_STACK_SIZE 512
#define CONFIG_IDLE_STACK_SIZE 320
#define CONFIG_ERRNO 1
#define CONFIG_SCHED_DUMB 1
#define CONFIG_WAITQ_DUMB 1
#define CONFIG_INIT_STACKS 1
#define CONFIG_BOOT_BANNER 1
#define CONFIG_BOOT_DELAY 0
#define CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE 1024
#define CONFIG_SYSTEM_WORKQUEUE_PRIORITY -1
#define CONFIG_OFFLOAD_WORKQUEUE_STACK_SIZE 1024
#define CONFIG_OFFLOAD_WORKQUEUE_PRIORITY -1
#define CONFIG_ATOMIC_OPERATIONS_BUILTIN 1
#define CONFIG_TIMESLICING 1
#define CONFIG_TIMESLICE_SIZE 0
#define CONFIG_TIMESLICE_PRIORITY 0
#define CONFIG_POLL 1
#define CONFIG_NUM_MBOX_ASYNC_MSGS 10
#define CONFIG_NUM_PIPE_ASYNC_MSGS 10
#define CONFIG_HEAP_MEM_POOL_SIZE 81920
#define CONFIG_HEAP_MEM_POOL_MIN_SIZE 64
#define CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN 1
#define CONFIG_SYS_CLOCK_EXISTS 1
#define CONFIG_KERNEL_INIT_PRIORITY_OBJECTS 30
#define CONFIG_KERNEL_INIT_PRIORITY_DEFAULT 40
#define CONFIG_KERNEL_INIT_PRIORITY_DEVICE 50
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#define CONFIG_STACK_POINTER_RANDOM 0
#define CONFIG_MP_NUM_CPUS 1
#define CONFIG_HAS_DTS 1
#define CONFIG_UART_CONSOLE_ON_DEV_NAME "UART_0"
#define CONFIG_CONSOLE 1
#define CONFIG_CONSOLE_INPUT_MAX_LINE_LEN 128
#define CONFIG_CONSOLE_HAS_DRIVER 1
#define CONFIG_CONSOLE_HANDLER 1
#define CONFIG_UART_CONSOLE 1
#define CONFIG_UART_CONSOLE_INIT_PRIORITY 60
#define CONFIG_UART_CONSOLE_DEBUG_SERVER_HOOKS 1
#define CONFIG_SERIAL 1
#define CONFIG_SERIAL_HAS_DRIVER 1
#define CONFIG_SERIAL_SUPPORT_INTERRUPT 1
#define CONFIG_UART_INTERRUPT_DRIVEN 1
#define CONFIG_UART_LINE_CTRL 1
#define CONFIG_UART_DRV_CMD 1
#define CONFIG_CORTEX_A_TIMER 1
#define CONFIG_SYSTEM_CLOCK_INIT_PRIORITY 0
#define CONFIG_SPI_LEGACY_API 1
#define CONFIG_USB 1
#define CONFIG_USB_DEVICE_DRIVER 1
#define CONFIG_USB_DRIVER_LOG_LEVEL_INF 1
#define CONFIG_USB_DRIVER_LOG_LEVEL 3
#define CONFIG_BRCM 1
#define CONFIG_BRCM_DRIVER_ADC 1
#define CONFIG_BRCM_DRIVER_BBL 1
#define CONFIG_BRCM_DRIVER_CRYPTO 1
#define CONFIG_CRYPTO_DPA 1
#define CONFIG_BRCM_DRIVER_DMA 1
#define CONFIG_BRCM_DRIVER_DMU 1
#define CONFIG_BRCM_DRIVER_FT 1
#define CONFIG_BRCM_DRIVER_GPIO 1
#define CONFIG_BRCM_DRIVER_I2C 1
#define CONFIG_BRCM_DRIVER_MSR 1
#define CONFIG_BRCM_DRIVER_LCD 1
#define CONFIG_LCD_8080_MODE_DC_GPIO 12
#define CONFIG_ALLOW_IOMUX_OVERRIDE 1
#define CONFIG_BRCM_DRIVER_PM 1
#define CONFIG_BRCM_DRIVER_PWM 1
#define CONFIG_BRCM_DRIVER_QSPI 1
#define CONFIG_USE_BSPI_FOR_READ 1
#define CONFIG_USE_WSPI_FOR_WRITE 1
#define CONFIG_USE_CPU_MEMCPY_FOR_WRITE 1
#define CONFIG_ENABLE_CACHE_INVAL_ON_WRITE 1
#define CONFIG_QSPI_QUAD_IO_ENABLED 1
#define CONFIG_SECONDARY_FLASH 1
#define CONFIG_SEC_FLASH_CLOCK_FREQUENCY 25000000
#define CONFIG_BRCM_DRIVER_RTC 1
#define CONFIG_BRCM_DRIVER_SCC 1
#define CONFIG_BRCM_DRIVER_SDIO 1
#define CONFIG_SDIO_DMA_MODE 1
#define CONFIG_BRCM_DRIVER_SMAU 1
#define CONFIG_MIN_SMAU_WINDOW_SIZE 0x100000
#define CONFIG_BRCM_DRIVER_SOTP 1
#define CONFIG_BRCM_DRIVER_SPI 1
#define CONFIG_SPI_DMA_MODE 1
#define CONFIG_SPI1_ENABLE 1
#define CONFIG_SPI1_USES_MFIO_6_TO_9 1
#define CONFIG_SPI2_ENABLE 1
#define CONFIG_SPI2_USES_MFIO_10_TO_13 1
#define CONFIG_SPI3_ENABLE 1
#define CONFIG_SPI3_USES_MFIO_14_TO_17 1
#define CONFIG_SPI5_ENABLE 1
#define CONFIG_BRCM_DRIVER_TIMERS 1
#define CONFIG_BRCM_DRIVER_UNICAM 1
#define CONFIG_USE_STATIC_MEMORY_FOR_BUFFER 1
#define CONFIG_UNICAM_PHYS_INTERFACE_CSI 1
#define CONFIG_CAM_I2C_PORT 1
#define CONFIG_LI_V024M_CAMERA 1
#define CONFIG_BRCM_DRIVER_USBH 1
#define CONFIG_BRCM_DRIVER_USBD 1
#define CONFIG_BRCM_DRIVER_WDT 1
#define CONFIG_STDOUT_CONSOLE 1
#define CONFIG_MINIMAL_LIBC_MALLOC_ARENA_SIZE 0
#define CONFIG_RING_BUFFER 1
#define CONFIG_POSIX_MAX_FDS 4
#define CONFIG_CONSOLE_SUBSYS 1
#define CONFIG_CONSOLE_GETCHAR 1
#define CONFIG_CONSOLE_GETCHAR_BUFSIZE 16
#define CONFIG_CONSOLE_PUTCHAR_BUFSIZE 16
#define CONFIG_STD_CPP11 1
#define CONFIG_PRINTK 1
#define CONFIG_EARLY_CONSOLE 1
#define CONFIG_ASSERT 1
#define CONFIG_ASSERT_LEVEL 2
#define CONFIG_OBJECT_TRACING 1
#define CONFIG_LOG 1
#define CONFIG_LOG_RUNTIME_FILTERING 1
#define CONFIG_LOG_DEFAULT_LEVEL 2
#define CONFIG_LOG_OVERRIDE_LEVEL 0
#define CONFIG_LOG_MAX_LEVEL 4
#define CONFIG_LOG_FUNC_NAME_PREFIX_DBG 1
#define CONFIG_LOG_IMMEDIATE 1
#define CONFIG_LOG_DOMAIN_ID 0
#define CONFIG_LOG_CMDS 1
#define CONFIG_SHELL 1
#define CONFIG_SHELL_LOG_LEVEL_INF 1
#define CONFIG_SHELL_LOG_LEVEL 3
#define CONFIG_SHELL_BACKENDS 1
#define CONFIG_SHELL_BACKEND_SERIAL 1
#define CONFIG_UART_SHELL_ON_DEV_NAME "UART_0"
#define CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN 1
#define CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE 8
#define CONFIG_SHELL_BACKEND_SERIAL_RX_RING_BUFFER_SIZE 64
#define CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT 100
#define CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE 10
#define CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL_DEFAULT 1
#define CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL 5
#define CONFIG_SHELL_STACK_SIZE 6144
#define CONFIG_SHELL_BACKSPACE_MODE_DELETE 1
#define CONFIG_SHELL_CMD_BUFF_SIZE 256
#define CONFIG_SHELL_PRINTF_BUFF_SIZE 30
#define CONFIG_SHELL_ARGC_MAX 12
#define CONFIG_SHELL_WILDCARD 1
#define CONFIG_SHELL_ECHO_STATUS 1
#define CONFIG_SHELL_VT100_COLORS 1
#define CONFIG_SHELL_METAKEYS 1
#define CONFIG_SHELL_HELP 1
#define CONFIG_SHELL_HELP_ON_WRONG_ARGUMENT_COUNT 1
#define CONFIG_SHELL_HISTORY 1
#define CONFIG_SHELL_HISTORY_BUFFER 1024
#define CONFIG_SHELL_MAX_LOG_MSG_BUFFERED 8
#define CONFIG_SHELL_STATS 1
#define CONFIG_SHELL_CMDS 1
#define CONFIG_SHELL_CMDS_RESIZE 1
#define CONFIG_SHELL_LOG_BACKEND 1
#define CONFIG_KERNEL_SHELL 1
#define CONFIG_USB_DEVICE_STACK 1
#define CONFIG_USB_DEVICE_LOG_LEVEL_INF 1
#define CONFIG_USB_DEVICE_LOG_LEVEL 3
#define CONFIG_USB_DEVICE_VID 0x8086
#define CONFIG_USB_DEVICE_PID 0xF8A1
#define CONFIG_USB_DEVICE_MANUFACTURER "ZEPHYR"
#define CONFIG_USB_DEVICE_PRODUCT "USB-DEV"
#define CONFIG_USB_DEVICE_SN "0.01"
#define CONFIG_USB_CDC_ACM 1
#define CONFIG_USB_CDC_ACM_RINGBUF_SIZE 1024
#define CONFIG_CDC_ACM_PORT_NAME_0 "CDC_ACM_0"
#define CONFIG_CDC_ACM_INTERRUPT_EP_MPS 16
#define CONFIG_CDC_ACM_BULK_EP_MPS 64
#define CONFIG_USB_CDC_ACM_LOG_LEVEL_INF 1
#define CONFIG_USB_CDC_ACM_LOG_LEVEL 3
#define CONFIG_ZTEST 1
#define CONFIG_ZTEST_STACKSIZE 1024
#define CONFIG_ZTEST_ASSERT_VERBOSE 1
#define CONFIG_TEST 1
#define CONFIG_TEST_EXTRA_STACKSIZE 29000
#define CONFIG_ZBAR 1
#define CONFIG_ZBAR_USE_STATIC_BUFFER 1
#define CONFIG_ZBAR_DEBUG 1
#define CONFIG_FNMATCH 1
#define CONFIG_OPENAMP_SRC_PATH "open-amp"
#define CONFIG_LINKER_ORPHAN_SECTION_WARN 1
#define CONFIG_KERNEL_ENTRY "__start"
#define CONFIG_CHECK_LINK_MAP 1
#define CONFIG_SIZE_OPTIMIZATIONS 1
#define CONFIG_COMPILER_OPT ""
#define CONFIG_KERNEL_BIN_NAME "zephyr"
#define CONFIG_OUTPUT_STAT 1
#define CONFIG_OUTPUT_DISASSEMBLY 1
#define CONFIG_OUTPUT_PRINT_MEMORY_USAGE 1
#define CONFIG_BUILD_OUTPUT_BIN 1