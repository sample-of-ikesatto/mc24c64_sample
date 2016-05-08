/******************************************************************************
 * Copyright (c) 2014 Satoshi Ikeda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

/**
 * Note:
 * - This codes use 48MHz for CPU clock.
 *   If you change clock, change SSPADD value.
 * - i2c speed is 100KHz
 */

#include <xc.h>
#include "i2c.h"

int ack_flag;

/**
 * !@brief Check the idle status
 *
 * Escape this function when ACKEN, RCEN, PEN, RSEN, SEN, R/W and BF registers
 * were all zero.
 * @param[in] mask Mask for SSPSTAT register
 */
void i2c_check_idle(char mask)
{
    while ((SSPCON2 & 0x1F ) | (SSPSTAT & mask));
}

/**
 * !@brief Interrupt fucntion for i2c
 *
 * Call this function from global interrupt function in main.c.
 */
void i2c_interrupt(void)
{
    if (SSPIF == 1) {
        if (ack_flag == 1) {
          ack_flag = 0;
        }
        SSPIF = 0;
    }
    if (BCLIF == 1) {
        BCLIF = 0;         // Clear only
    }
}

/**
 * !@brief Initialize as i2c master
 *
 * Note: This codes use 48MHz for CPU clock.
 *       If you change clock, change SSPADD value.
 */
void i2c_init_master(void)
{
    SSPCON1= 0b00101000;   // master mode
    SSPSTAT= 0b00000000;
    SSPADD = 0x77;       // normal speed, 100kHz.
    //SSPADD = 0x1D;         // fast mode, 400kHz
    //SSPADD = 0xB;       // more fast mode, 1MHz
                           // clock=FOSC/((SSPADD + 1)*4)
                           //   100kHz = 48MHz/((SSPADD + 1)*4)
                           //     SSPADD = ((48*1000/4) / 100) - 1
                           //            = 119 = 0x77
                           //   400kHz = 48MHz/((SSPADD + 1)*4)
                           //     SSPADD = ((48*1000/4) / 400) - 1
                           //            = 29 = 0x1D
                           //   1MHz = 48MHz/((SSPADD + 1)*4)
                           //     SSPADD = ((48/4) / 1) - 1
                           //            = 11 = 0xB

    SSPIE = 1;             // Enable SSP(I2C) interrupt
    BCLIE = 1;             // Enable MSSP(I2C) conflict interrupt
    PEIE  = 1;             // Enable Peripheral interrupt
    GIE   = 1;             // Enable Global interrupt
    SSPIF = 0;             // Clear SSP(I2C) interrupt flag
    BCLIF = 0;             // Clear MSSP(I2C) interrupt flag
}

/**
 * !@brief Send start condition to slave
 *
 * @param[in] adrs Slave address
 * @param[in] rw 0:write 1:read
 * @return Return the result. 0:success 1:failure
 */
int i2c_start(unsigned char adrs, unsigned char rw)
{
    // Set start condition
    i2c_check_idle(0x5);
    SSPCON2bits.SEN = 1;

    // Set slave address and rw mode
    i2c_check_idle(0x5);
    ack_flag = 1;
    SSPBUF = (unsigned char)((adrs<<1)+rw);
    while (ack_flag);       // Wait ACK
    return SSPCON2bits.ACKSTAT;
}

/**
 * !@brief Send repeated start condtion command
 *
 * @param[in] adrs Slave address
 * @param[in] rw 0:write 1:read
 * @return Return the result. 0:success 1:failure
 */
int i2c_rstart(unsigned char adrs, unsigned char rw)
{
    // Set repeated start condition
    i2c_check_idle(0x5);
    SSPCON2bits.RSEN = 1;

    // Set slave address and rw mode
    i2c_check_idle(0x5);
    ack_flag = 1;
    SSPBUF = (char)((adrs<<1)+rw);
    while (ack_flag);       // Wait ACK
    return SSPCON2bits.ACKSTAT;
}

/**
 * !@brief Send stop condition
 */
void i2c_stop(void)
{
    i2c_check_idle(0x5);
    SSPCON2bits.PEN = 1;
}

/**
 * !@brief Send data
 *
 * @param[in] dt Data to send
 * @return Return the result. 0:success 1:failure
 */
int i2c_send(char dt)
{
    i2c_check_idle(0x5);
    ack_flag = 1;
    SSPBUF = dt;
    while (ack_flag);       // Wait ACK
    return SSPCON2bits.ACKSTAT;
}

/**
 * !@brief Receive data from slave
 *
 * @param[in] ack ACK data after received
 * @return Received data
 */
char i2c_receive(int ack)
{
    char dt;

    i2c_check_idle(0x5);
    SSPCON2bits.RCEN = 1;      // Enable receive
    i2c_check_idle(0x4);
    dt = SSPBUF;               // Receive data
    i2c_check_idle(0x5);
    SSPCON2bits.ACKDT = ack;
    SSPCON2bits.ACKEN = 1;     // Response ACK
    return dt;
}
