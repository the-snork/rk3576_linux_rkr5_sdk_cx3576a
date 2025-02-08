/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      userial_vendor.h
 *
 *  Description:   Contains vendor-specific definitions used in serial port
 *                 controls
 *
 ******************************************************************************/

#ifndef _USERIAL_VENDOR_H_
#define _USERIAL_VENDOR_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
**  Constants & Macros
******************************************************************************/

/**** baud rates ****/
#define USERIAL_BAUD_300        300
#define USERIAL_BAUD_600        600
#define USERIAL_BAUD_1200       1200
#define USERIAL_BAUD_2400       2400
#define USERIAL_BAUD_9600       9600
#define USERIAL_BAUD_19200      19200
#define USERIAL_BAUD_38400      38400
#define USERIAL_BAUD_57600      57600
#define USERIAL_BAUD_115200     115200
#define USERIAL_BAUD_230400     230400
#define USERIAL_BAUD_460800     460800
#define USERIAL_BAUD_921600     921600
#define USERIAL_BAUD_1M         1000000
#define USERIAL_BAUD_1_5M       1500000
#define USERIAL_BAUD_2M         2000000
#define USERIAL_BAUD_3M         3000000
#define USERIAL_BAUD_4M         4000000
#define USERIAL_BAUD_AUTO       0

/**** Data Format ****/
/* Stop Bits */
#define USERIAL_STOPBITS_1      1
#define USERIAL_STOPBITS_1_5    (1<<1)
#define USERIAL_STOPBITS_2      (1<<2)

/* Parity Bits */
#define USERIAL_PARITY_NONE     (1<<3)
#define USERIAL_PARITY_EVEN     (1<<4)
#define USERIAL_PARITY_ODD      (1<<5)

/* Data Bits */
#define USERIAL_DATABITS_5      (1<<6)
#define USERIAL_DATABITS_6      (1<<7)
#define USERIAL_DATABITS_7      (1<<8)
#define USERIAL_DATABITS_8      (1<<9)


/******************************************************************************
**  Type definitions
******************************************************************************/

/* Structure used to configure serial port during open */
typedef struct
{
    uint16_t fmt;       /* Data format */
    uint8_t  baud;      /* Baud rate */
} tUSERIAL_CFG;

typedef enum {
    USERIAL_OP_NOP,
} userial_vendor_ioctl_op_t;

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

/******************************************************************************
**  Functions
******************************************************************************/

/**
 * @brief Opens a UART serial port with or without additional format options, based on the number of parameters.
 * 
 * This macro provides an overloaded interface for opening a UART device. Depending on the number of arguments 
 * passed, it calls the appropriate function:
 * 
 * - If 3 arguments are provided, it calls `userial_uart_open_with_opt`:
 *   ```c
 *   int userial_uart_open_with_opt(char *dev, uint32_t speed, uint16_t fmt);
 *   ```
 *   This version allows specifying the UART format in addition to the device and speed.
 * 
 * - If 2 arguments are provided, it calls `userial_uart_open_without_opt`:
 *   ```c
 *   int userial_uart_open_without_opt(char *dev, uint32_t speed);
 *   ```
 *   This version only requires the device and speed, using default UART format settings.
 * 
 * Usage examples:
 * 
 * ```c
 * // Open with format options
 * int fd = userial_vendor_open("/dev/ttyS0", 115200, 0x3);
 * 
 * // Open without format options
 * int fd = userial_vendor_open("/dev/ttyS0", 115200);
 * ```
 * 
 * Implementation:
 * This macro uses a variadic argument list (`__VA_ARGS__`) and a helper macro (`UART_OVERLOAD`) to select 
 * the appropriate function based on the number of arguments provided.
 * 
 * @note Ensure the arguments passed match the expected count and types to avoid compilation errors.
 * 
 * @param ... Variable number of arguments:
 *            - 2 arguments: device name (`char *`) and speed (`uint32_t`).
 *            - 3 arguments: device name (`char *`), speed (`uint32_t`), and format (`uint16_t`).
 * 
 * @return The return value of the respective function:
 *         - On success: A file descriptor (`int`) for the opened UART device.
 *         - On failure: A negative error code.
 */
int userial_uart_open_with_opt(char *dev, uint32_t speed, uint16_t fmt);
int userial_uart_open_without_opt(char *dev, uint32_t speed);
#define UART_OVERLOAD(_1, _2, _3, NAME, ...) NAME
#define userial_vendor_open(...) UART_OVERLOAD(__VA_ARGS__, userial_uart_open_with_opt, userial_uart_open_without_opt)(__VA_ARGS__)
/*******************************************************************************
**
** Function        userial_vendor_close
**
** Description     Conduct vendor-specific close work
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_close(int fd);

/*******************************************************************************
**
** Function        userial_vendor_set_baud
**
** Description     Set new baud rate
**
** Returns         None
**
*******************************************************************************/
void userial_vendor_set_baud(int fd, uint8_t userial_baud);

void userial_dump_data(uint8_t *data, uint16_t total);
uint16_t userial_vendor_send_data(int fd, uint8_t *data, uint16_t total);
#ifdef __cplusplus
}
#endif
#endif /* USERIAL_VENDOR_H */

