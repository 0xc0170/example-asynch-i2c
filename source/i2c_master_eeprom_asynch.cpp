/* mbed Microcontroller Library
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <TestHarness.h>
#include <mbed.h>
#include <stdio.h>

/* EEPROM 24LC256 Test Unit, to test I2C asynchronous communication.
 */

#if !DEVICE_I2C || !DEVICE_I2C_ASYNCH
#error i2c_master_eeprom_asynch requires asynch I2C
#endif


#if defined(TARGET_K64F)
#define TEST_SDA_PIN PTE25
#define TEST_SCL_PIN PTE24
#else
#error Target not supported
#endif

#define PATTERN_MASK 0x66, ~0x66, 0x00, 0xFF, 0xA5, 0x5A, 0xF0, 0x0F

volatile int why;
volatile bool complete;
void cbdone(int event) {
    complete = true;
    why = event;
}

const unsigned char pattern[] = { PATTERN_MASK };

TEST_GROUP(I2C_Master_EEPROM_Asynchronous)
{
    I2C *obj;
    const int eeprom_address = 0xA0;

    void setup() {
        obj = new I2C(TEST_SDA_PIN, TEST_SCL_PIN);
        obj->frequency(400000);
        complete = false;
        why = 0;
    }

    void teardown() {
        delete obj;
        obj = NULL;
    }

};

TEST(I2C_Master_EEPROM_Asynchronous, tx_rx_one_byte_separate_transactions)
{
    int rc;
    char data[] = { 0, 0, 0x66};

    obj->transfer(eeprom_address, data, sizeof(data), NULL, 0, cbdone, I2C_EVENT_ALL, false);
    while(!complete);

    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);

    while (obj->write(eeprom_address, NULL, 0)); //wait till slave is ready

    // write the address for reading (0,0) then start reading data
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    why = 0;
    complete = 0;
    obj->transfer(eeprom_address, data, 2, NULL, 0, cbdone, I2C_EVENT_ALL, true);
    while(!complete);
    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);

    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    why = 0;
    complete = 0;
    obj->transfer(eeprom_address, NULL, 0, data, 1, cbdone, I2C_EVENT_ALL, false);
    while(!complete);
    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);
    CHECK_EQUAL(data[0], 0x66);
}

TEST(I2C_Master_EEPROM_Asynchronous, tx_rx_one_byte_one_transactions)
{
    int rc;
    char send_data[] = { 0, 0, 0x66};
    rc = obj->transfer(eeprom_address, send_data, sizeof(send_data), NULL, 0, cbdone, I2C_EVENT_ALL, false);
    CHECK_EQUAL(0, rc)

    while(!complete);

    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);

    while (obj->write(eeprom_address, NULL, 0)); //wait till slave is ready

    send_data[0] = 0;
    send_data[1] = 0;
    send_data[2] = 0;
    char receive_data[1] = {0};
    why = 0;
    complete = 0;
    obj->transfer(eeprom_address, send_data, 2, receive_data, 1, cbdone, I2C_EVENT_ALL, false);
    while(!complete);

    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);
    CHECK_EQUAL(receive_data[0], 0x66);
}

TEST(I2C_Master_EEPROM_Asynchronous, tx_rx_pattern)
{
    int rc;
    char data[] = { 0, 0, PATTERN_MASK};
    // write 8 bytes to 0x0, then read them
    rc = obj->transfer(eeprom_address, data, sizeof(data), NULL, 0, cbdone, I2C_EVENT_ALL, false);
    CHECK_EQUAL(0, rc);

    while (!complete);
    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);
    while (obj->write(eeprom_address, NULL, 0)); //wait till slave is ready

    complete = 0;
    why = 0;
     char rec_data[8] = {0};
    obj->transfer(eeprom_address, rec_data, 2, NULL, 0, cbdone, I2C_EVENT_ALL, true);
    while (!complete);
    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);

    complete = 0;
    why = 0;
    obj->transfer(eeprom_address, NULL, 0, rec_data, 8, cbdone, I2C_EVENT_ALL, false);

    while (!complete);
    CHECK_EQUAL(why, I2C_EVENT_TRANSFER_COMPLETE);

    // received buffer match with pattern
    rc = memcmp(pattern, rec_data, sizeof(rec_data));
    CHECK_EQUAL(0, rc);
}