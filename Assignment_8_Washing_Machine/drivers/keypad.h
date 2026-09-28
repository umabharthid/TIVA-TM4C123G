#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

/*
============================================================
                KEYPAD DRIVER (4x4 MATRIX)
============================================================

Hardware configuration (based on your setup):
- Rows    : PE0 – PE3  (outputs, open-drain)
- Columns : PC4 – PC7  (inputs, pull-up)

Keypad layout returned by driver:

    [1] [2] [3] [A]
    [4] [5] [6] [B]
    [7] [8] [9] [C]
    [*] [0] [#] [D]

============================================================
                    HOW TO USE
============================================================

1. Call once at startup:
        Keypad_Init();

------------------------------------------------------------

2. NON-BLOCKING MODE (recommended for most cases)

        char key = Keypad_Scan();

        if(key != 0) {
            // A key is pressed
        }

- Returns:
    '1'–'9', '0', 'A'–'D', '*', '#'
    0 - no key pressed

- Use when:
     You don’t want program to stop
     Real-time systems (PWM, ADC, UART, etc.)

------------------------------------------------------------

3. BLOCKING MODE (waits for input)

        char key = Keypad_GetKey();

- Returns:
    Always a valid key (waits until pressed)

- Use when:
     Menu selection
     Password entry
     User input required

------------------------------------------------------------

IMPORTANT NOTES:
 Debouncing is handled internally
 Function returns key ONLY once per press
 Waits for key release (no repeated triggers)

============================================================
*/

void Keypad_Init(void);

/*
------------------------------------------------------------
Keypad_Scan()

Non-blocking scan.

Returns:
    char - pressed key
    0    - no key pressed
------------------------------------------------------------
*/
char Keypad_Scan(void);

/*
------------------------------------------------------------
Keypad_GetKey()

Blocking function.

Waits until a key is pressed.

Returns:
    char - pressed key
------------------------------------------------------------
*/
char Keypad_GetKey(void);

#endif
