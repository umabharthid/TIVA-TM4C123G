#ifndef UART_H
#define UART_H

#include <stdint.h>

/* ================= USER FUNCTIONS ================= */

/* initialize UART0 */
void uart_init(void);

/* print string to terminal */
void printuart(char *str);

/* check for typed command (returns 1 only once) */
uint8_t istypeduart(char *cmd);

/* update UART buffer (non-blocking polling) */
void uart_update(void);
char* uart_get_buffer(void);
uint8_t uart_command_ready(void);
void uart_clear_command(void);
#endif
