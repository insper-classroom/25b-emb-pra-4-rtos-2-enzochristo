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
int const delay = 250;

// == funcoes de inicializacao ===
// void btn_callback(uint gpio, uint32_t events) {
//     if (events & GPIO_IRQ_EDGE_FALL) xQueueSendFromISR(xQueueBtn, &gpio, 0);
// }

void pin_callback(uint gpio, uint32_t events) {
    int static final,start;


    // printf("evento: 0x%x\n", events);  
    if(events == 0x8) {  // rise edge
        start = to_us_since_boot(get_absolute_time()); 

        // printf("time rise : %d\n", start);

        // printf("RISE\n");      
    } else if (events ==  GPIO_IRQ_EDGE_FALL) {         // fall edge
        final = to_us_since_boot(get_absolute_time());

        int dt = final - start;
        xQueueSendFromISR(xQueueTime, &dt, 0);
        
    }
}

void echo_task(void *p){
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    


    int dt = 0;
    double distancia;    
    while(1){
       
        if (xQueueReceive(xQueueTime, &dt,  pdMS_TO_TICKS(delay))) {

            printf("dt : %d\n", dt);
            distancia = dt*V_SOM; 

            printf("distancia : %lf cm \n", distancia);

            if (xQueueSend(XQueueDistance, &distancia, 10)){
                printf("enviou o elemento!");
            }else{
                printf("fila esta cheia.");
            }
        }
        else{
            printf("erro na medicao!");
        }
        
    }
}

void oled_task(void *p){
    
}


void trigger_task(void *p ){

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN,0);

    while(1){
        gpio_put(TRIG_PIN, 1);
        // vTaskDelay(pdMS_TO_TICKS(0.0001));
        sleep_us(10);
        gpio_put(TRIG_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(delay));
        
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
    XQueueDistance = xQueueCreate(64, sizeof(double));
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    // xTaskCreate(task_1, "Task 1", 8192, NULL, 1, NULL);
    xTaskCreate(trigger_task, "Task 2", 256, NULL, 1, NULL);
    xTaskCreate(echo_task, "Task 3", 256, NULL, 1, NULL);
    xTaskCreate(oled_task, "Task 4", 256, NULL, 1, NULL);




    vTaskStartScheduler();

    while (true);
}
