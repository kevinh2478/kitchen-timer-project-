/*
|| @file pico_keypad4x4.c
|| @version 0.2
|| @author Oswaldo Hernandez, Owen Jauregui
|| @contact oswahdez00@gmail.com
||
|| @description
|| | This library provides a simple interface for using a 4x4 matrix
|| | keypad with the Raspberry Pi Pico.
*/
// Updated code 12.18.2025 for Kitchen timer 

#include "pico_keypad4x4.h"

#define GPIO_INPUT false
#define GPIO_OUTPUT true

uint _columns[4];
uint _rows[4];
char _matrix_values[16];

uint all_columns_mask = 0x0;
uint column_mask[4];

/**
 * @brief Set up the keypad
 */
void pico_keypad_init(uint columns[4], uint rows[4], char matrix_values[16]) {
    // Reset mask in case of re-init
    all_columns_mask = 0x0;

    for (int i = 0; i < 16; i++) {
        _matrix_values[i] = matrix_values[i];
    }

    for (int i = 0; i < 4; i++) {
        _columns[i] = columns[i];
        _rows[i] = rows[i];

        // Initialize Row Pins
        gpio_init(_rows[i]);
        gpio_set_dir(_rows[i], GPIO_OUTPUT);
        gpio_put(_rows[i], 1); // Set high by default

        // Initialize Column Pins
        gpio_init(_columns[i]);
        gpio_set_dir(_columns[i], GPIO_INPUT);
        
        // IMPORTANT: Enable internal pull-down resistors so columns 
        // stay at 0 unless a Row (High) is connected via a button press.
        gpio_pull_down(_columns[i]);

        // Build masks for bitwise comparison
        all_columns_mask |= (1 << _columns[i]);
        column_mask[i] = (1 << _columns[i]);
    }
}

/**
 * @brief Scan and get the pressed key.
 */
char pico_keypad_get_key(void) {
    uint32_t cols;
    int row = -1;

    // Quick check: Is any key pressed at all?
    cols = gpio_get_all() & all_columns_mask;
    if (cols == 0x0) {
        return 0;
    }

    // Prepare for scanning: Set all rows low
    for (int j = 0; j < 4; j++) {
        gpio_put(_rows[j], 0);
    }

    // Scan each row individually
    for (int r = 0; r < 4; r++) {
        gpio_put(_rows[r], 1); // Bring one row high
        busy_wait_us(50);      // Tiny delay for electrical settling

        cols = gpio_get_all() & all_columns_mask;
        
        if (cols != 0x0) {
            row = r;
            // Key found, keep the state and break
            break; 
        }
        gpio_put(_rows[r], 0); // Bring it back low
    }

    // Reset all rows to High for the next "quick check"
    for (int i = 0; i < 4; i++) {
        gpio_put(_rows[i], 1);
    }

    // If no row was identified, return 0
    if (row == -1) return 0;

    // Determine which column was triggered
    for (int c = 0; c < 4; c++) {
        if (cols == column_mask[c]) {
            return _matrix_values[row * 4 + c];
        }
    }

    return 0;
}

void pico_keypad_irq_enable(bool enable, gpio_irq_callback_t callback) {
    for (int i = 0; i < 4; i++) {
        // 0x8 corresponds to GPIO_IRQ_EDGE_RISE
        gpio_set_irq_enabled_with_callback(_columns[i], GPIO_IRQ_EDGE_RISE, enable, callback);
    }
}