/*
 *  dht.c:
 *      read temperature and humidity from DHT11 or DHT22 sensor
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_TIMINGS     85
#define DHT_PIN         3       /* GPIO-22 */

int data[5] = {0, 0, 0, 0, 0};

float *read_dht_data() {
    static float result[2] = {0.0, 0.0};

    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    /* pull pin down for 18 milliseconds */
    pinMode(DHT_PIN, OUTPUT);
    digitalWrite(DHT_PIN, LOW);
    delay(18);

    /* prepare to read the pin */
    pinMode(DHT_PIN, INPUT);

    /* detect change and read data */
    for (i = 0; i < MAX_TIMINGS; i++) {
        counter = 0;
        while (digitalRead(DHT_PIN) == laststate) {
            counter++;
            delayMicroseconds(1);
            if (counter == 255) {
                break;
            }
        }
        laststate = digitalRead(DHT_PIN);

        if (counter == 255)
            break;

        /* ignore first 3 transitions */
        if ((i >= 4) && (i % 2 == 0)) {
            /* shove each bit into the storage bytes */
            data[j / 8] <<= 1;
            if (counter > 16)
                data[j / 8] |= 1;
            j++;
        }
    }

    /*
     * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
     * print it out if data is good
     */
    if ((j >= 40) &&
        (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        float h = (float) ((data[0] << 8) + data[1]) / 10;
        if (h > 100) {
            h = data[0];    // for DHT11
        }
        float c = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
        if (c > 125) {
            c = data[2];    // for DHT11
        }
        if (data[2] & 0x80) {
            c = -c;
        }
        float f = c * 1.8f + 32;
        printf("Humidity = %.1f %% Temperature = %.1f *C (%.1f *F)\n", h, c, f);
        result[0] = h;
        result[1] = c;
    } else {
        printf("Data not good, skip\n");
    }
    return result;
}

int main(void) {
    printf("Raspberry Pi DHT11/DHT22 temperature/humidity test\n");
    float *indicators;
    float current_humidity = 0.0;
    float current_temperature = 0.0;
    FILE *fp;
    uint8_t i;

    if (wiringPiSetup() == -1)
        exit(1);

    for (i = 0; i < 6; i++) {
        indicators = read_dht_data();
        if (*(indicators) != 0.0) {
            current_humidity = *(indicators);
        }
        if (*(indicators + 1) != 0.0) {
            current_temperature = *(indicators + 1);
        }
        delay(2000); /* wait 2 seconds before next read */
    }

    if (current_humidity != 0.0) {
        fp = fopen("/home/pi/current_humidity.txt", "w");
        fprintf(fp, "%.1f", current_humidity);
        fclose(fp);
        printf("Current Humidity = %.1f written to file\n", current_humidity);
    }

    if (current_temperature != 0.0) {
        fp = fopen("/home/pi/current_temperature.txt", "w");
        fprintf(fp, "%.1f", current_temperature);
        fclose(fp);
        printf("Current Temperature = %.1f written to file\n", current_temperature);
    }

    return (0);
}
