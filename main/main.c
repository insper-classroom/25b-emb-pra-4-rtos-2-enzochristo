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
    double distancia = 0;    
    while(1){
       
        if (xQueueReceive(xQueueTime, &dt,  pdMS_TO_TICKS(1000))) {

            // printf("dt : %d\n", dt);
            distancia = dt*V_SOM; 

            // printf("distancia : %lf cm \n", distancia);

            if (xQueueSend(XQueueDistance, &distancia, 10)){
                // printf("enviou o elemento!");
            }else{
                // printf("fila esta cheia.");
            }
        }
        else{
            printf("erro na medicao!");
        }
        
    }
}

void oled_display_init(void) {
    i2c_init(i2c1, 400000);
    gpio_set_function(2, GPIO_FUNC_I2C);
    gpio_set_function(3, GPIO_FUNC_I2C);
    gpio_pull_up(2);
    gpio_pull_up(3);

    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
}

void led_rgb_init(void) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 1);

    gpio_init(LED_PIN_G);
    gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_put(LED_PIN_G, 1);

    gpio_init(LED_PIN_B);
    gpio_set_dir(LED_PIN_B, GPIO_OUT);
    gpio_put(LED_PIN_B, 1);
}

void oled_task(void *p){
    led_rgb_init();
    oled_display_init();
   
    double distance = 0;
    char s_distance[20];

    while(1){

        if(xSemaphoreTake(xSemaphoreTrigger, pdMS_TO_TICKS(100)) == pdTRUE){
            
            if (xQueueReceive(XQueueDistance, &distance, 100)){
                printf("Distancia recebida! %lf", distance);

                if (distance < 100){
                    gpio_put(LED_PIN_G,0);
                    gpio_put(LED_PIN_B,1);
                    gpio_put(LED_PIN_R,0);
                }
                else{
                    gpio_put(LED_PIN_G,0);
                    gpio_put(LED_PIN_B,1);
                    gpio_put(LED_PIN_R,1);
                }

                // escrevendo na OLED
                snprintf(s_distance, sizeof(s_distance), "%.2f", distance); // escreve no s_distance o valor de distance
                ssd1306_clear(&disp);
                ssd1306_draw_string(&disp, 0,0,2, "Distancia:");
                ssd1306_show(&disp);
                ssd1306_draw_string(&disp, 8,32,2, s_distance);
                ssd1306_show(&disp);

            }else{
                gpio_put(LED_PIN_G,1);
                gpio_put(LED_PIN_B,1);
                gpio_put(LED_PIN_R,0);  

                // escrevendo na OLED
                ssd1306_clear(&disp);
                ssd1306_draw_string(&disp, 24,0,2, "Erro!");
                ssd1306_show(&disp);
            }

                            
        }
        
    }

}

void trigger_task(void *p ){

    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_put(TRIG_PIN,0);

    while(1){
        gpio_put(TRIG_PIN, 1);
        // vTaskDelay(pdMS_TO_TICKS(0.0001));
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_put(TRIG_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        
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

    xQueueTime = xQueueCreate(2, sizeof(uint));
    XQueueDistance = xQueueCreate(2, sizeof(double));
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    xTaskCreate(trigger_task, "Task 2", 256, NULL, 1, NULL);
    xTaskCreate(echo_task, "Task 3", 256, NULL, 1, NULL);
    xTaskCreate(oled_task, "Task 4", 256, NULL, 1, NULL);




    vTaskStartScheduler();

    while (true);
}
