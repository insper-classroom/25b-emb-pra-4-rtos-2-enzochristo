#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdio.h>
#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "ssd1306.h"
#define V_SOM 0.017015 

// === Definições para SSD1306 ===
ssd1306_t disp;

QueueHandle_t xQueueBtn, xQueueTime, XQueueDistance;
SemaphoreHandle_t xSemaphoreTrigger;
int final,start;

// == funcoes de inicializacao ===
// void btn_callback(uint gpio, uint32_t events) {
//     if (events & GPIO_IRQ_EDGE_FALL) xQueueSendFromISR(xQueueBtn, &gpio, 0);
// }

void pin_callback(uint gpio, uint32_t events) {

    // printf("evento: 0x%x\n", events);  
    if (events ==  GPIO_IRQ_EDGE_FALL) {         // fall edge
        final = to_us_since_boot(get_absolute_time());
        // printf("time fall : %d\n", final);
        // printf("FALL\n");  
        xQueueSendFromISR(xQueueTime, &final, 0);

        // printf("subtracao fall - rise: %lf: \n",(double)((final - start)*V_SOM));
        
    } else if(events == 0x8) {  // rise edge
        start = to_us_since_boot(get_absolute_time()); 
        xQueueSendFromISR(xQueueTime, &start, 0);

        // printf("time rise : %d\n", start);

        // printf("RISE\n");      
    }
}

void echo_task(void *p){
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    


    int d = 0;
    int time;
    // int recebeu = 0;
    
    while(1){
       
        if (xQueueReceive(xQueueTime, &time,  pdMS_TO_TICKS(10))) {
            d = time;
            // printf("tempo 1 : %d\n", time);
            // printf("tempo em d : %d\n", d);
            // printf("time: %ld\n", time);
            if (xQueueReceive(xQueueTime, &time,  pdMS_TO_TICKS(10))){
                // printf("time2: %d\n", time);
            }
                d = time - d ;
                printf("diferenca de tempos : %d\n", d);
            //     // d = (double) d*V_SOM;
            //     // printf("distancia: %lf", d);
            // }
            // else{
            //     printf("erro na segunda medicao!");
            // }
        }else{
            printf("erro na primeira medicao!");
        }
        
    }
}


void trigger_task(void *p ){

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN,0);

    while(1){
        gpio_put(TRIG_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_put(TRIG_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        
        if (xSemaphoreGive(xSemaphoreTrigger) == pdTRUE) {
            // printf("Semaforo dado com sucesso!\n");
            printf("\n");
        } else {
            // printf("Falha ao dar o semaforo!\n");
            printf("\n");
        }
    }


}
int main() {
    stdio_init_all();
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL , true, &pin_callback);

    // xQueueBtn = xQueueCreate(64, sizeof(uint));
    xQueueTime = xQueueCreate(64, sizeof(uint));
    // XQueueDistance = xQueueCreate(64, sizeof(double));
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    // xTaskCreate(task_1, "Task 1", 8192, NULL, 1, NULL);
    xTaskCreate(trigger_task, "Task 2", 256, NULL, 1, NULL);
    xTaskCreate(echo_task, "Task 3", 256, NULL, 1, NULL);



    vTaskStartScheduler();

    while (true);
}


/*
     if (xQueueReceive(xQueueTime, &time,  pdMS_TO_TICKS(100))){
                printf("time2: %lld\n", time);
                time = time - time2;
                printf("diferenca de tempos : %lld\n", time);
                d = time*V_SOM;
                // d = d*V_SOM;
                printf("distancia: %lld", d);
            }
            else{
                printf("Falha na medicao do segundo dado!");
            }
        }
        else{
            printf("Falha na medicao do primeiro dado!");

        }
     

*/