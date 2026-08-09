#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "pico/time.h"
#include "lcd_display.hpp"

//Wrap this include in extern "C" to prevent linker errors
extern "C" {
    #include "pico_keypad4x4.h"
}

#define BUTTON_ENTER 'A'
#define BUTTON_STOP  'B'
#define BUTTON_RESET 'C'
#define BUZZER_PIN 17

LCDdisplay lcd(0, 1, 2, 3, 4, 5, 6, 16, 2); 

// pins: R1-R4 -> 8,9,10,11 | C1-C4 -> 12,13,14,15
uint keypad_rows[4] = {8, 9, 10, 11};
uint keypad_columns[4] = {12, 13, 14, 15};

char keypad_values[16] = {
    '1','2','3','A',
    '4','5','6','B',
    '7','8','9','C',
    '*','0','#','D',
};

int countdown_time = 0;
bool timer_running = false;
char last_key = 0; // For debouncing

void beep_buzzer(int duration_ms) {
    gpio_put(BUZZER_PIN, 1);
    sleep_ms(duration_ms);
    gpio_put(BUZZER_PIN, 0);
}


void start_timer() {
    while (countdown_time > 0 && timer_running) {
        int minutes = countdown_time / 60;
        int seconds = countdown_time % 60;

        char time_str[10];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);

        lcd.clear();
        lcd.goto_pos(0, 0);
        lcd.print("Timer: ");
        lcd.print(time_str);

        // Check keypad 10 times during the 1-second sleep for better responsiveness
        for(int i = 0; i < 10; i++) {
            char key = pico_keypad_get_key();
            if (key == BUTTON_STOP) {
                timer_running = false;
                break;
            }
            if (key == BUTTON_RESET) {
                countdown_time = 0;
                timer_running = false;
                break;
            }
            sleep_ms(100); 
            if (timer_running) countdown_time--;
    }

    //TIME'S UP
    if (countdown_time <= 0 && timer_running) {
        lcd.clear(); 
        lcd.goto_pos(0, 0);
        lcd.print("Time's up!");

        // Beep buzzer 3 times
        for (int i = 0; i < 3; i++) {
            beep_buzzer(300);
            sleep_ms(200);
        }

        timer_running = false;
        }

        if (timer_running) countdown_time--;
    }

    if (countdown_time <= 0 && timer_running) {
        lcd.clear();
        lcd.goto_pos(0, 0);
        lcd.print("Time's up!");
        timer_running = false;
    }
}

void handle_keypad_input() {
    char current_key = pico_keypad_get_key();
    
    // Only process the key if it's a new press (debounce/single-press logic)
    if (current_key != 0 && current_key != last_key) {
        if (current_key >= '0' && current_key <= '9') {
            // Shift digits (standard microwave/timer logic)
            countdown_time = (countdown_time * 1) + (current_key - '0');
            if (countdown_time > 5999) countdown_time = 5999; // Cap at 99:59

            int minutes = countdown_time / 100; // Treating as MMSS input
            int seconds = countdown_time % 100;
            
            // Convert MMSS to total seconds for the actual timer logic
            // (Note: This logic is simplified for user input)
            
            char time_str[10];
            snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
            lcd.goto_pos(0, 1);
            lcd.print("Set: ");
            lcd.print(time_str);

        } else if (current_key == BUTTON_ENTER) {
            if (countdown_time > 0) {
                // Convert the "MMSS" style input to actual total seconds
                int m = countdown_time / 100;
                int s = countdown_time % 100;
                countdown_time = (m * 60) + s;

                lcd.clear();
                timer_running = true;
                start_timer();
            }
        } else if (current_key == BUTTON_RESET) {
            countdown_time = 0;
            timer_running = false;
            lcd.clear();
            lcd.print("Reset");
            sleep_ms(500);
            lcd.clear();
            lcd.print("Ready...");
        }
    }
    last_key = current_key;
}

int main() {
    stdio_init_all();
    pico_keypad_init(keypad_columns, keypad_rows, keypad_values);
    
    lcd.init();
    lcd.set_backlight(80); 

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);


    lcd.print("Pico 2 W Timer");
    lcd.goto_pos(0, 1);
    lcd.print("Ready...");

    while (true) {
        handle_keypad_input();
        sleep_ms(20); // delay for buttons input 
    }
}