/*********************************************************************
*                SEGGER Microcontroller GmbH & Co. KG                *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2017 SEGGER Microcontroller GmbH & Co. KG         *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : Main.c
Purpose : Generic SEGGER application start
--------  END-OF-HEADER  ---------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <nrf.h>
#include <nrfx_uart.h>
#include <core_cm4.h>

#define UART_TX_PIN 6
#define UART_RX_PIN 8
#define UART_BAUD_RATE NRF_UART_BAUDRATE_115200

#define MAX_RETRIES 3
#define RETRY_DELAY_MS 2000
#define SERVER_URL "example.com"      
#define SERVER_PORT 443              
#define HTTPS 1                       
#define API_PATH "/api/telemetry"     


nrfx_uart_config_t uart_config = NRFX_UART_DEFAULT_CONFIG;

// Define the UART instance
#define NRFX_UART0_INSTANCE   NRF_UART0


void uart_init(void);
static bool uart_send_command(const char *cmd, char *response, size_t response_size);
static bool check_sim_status(void);
static bool connect_network(void);
static bool send_telemetry(void);
static void manage_connection(void);
static void delay_ms(int ms);

// UART Initialization
void uart_init(void) {
    uart_config.baudrate = UART_BAUD_RATE;
    uart_config.hwfc = NRF_UART_HWFC_DISABLED;
    uart_config.parity = NRF_UART_PARITY_EXCLUDED;

    
    nrfx_uart_init(NRFX_UART0_INSTANCE, &uart_config, NULL);
}

// Send AT command and get response
bool uart_send_command(const char *cmd, char *response, size_t response_size) {
    printf("[UART] Sending command: %s\n", cmd);
    nrfx_uart_tx(NRFX_UART0_INSTANCE, (const uint8_t*)cmd, strlen(cmd));
    nrfx_uart_tx(NRFX_UART0_INSTANCE, (const uint8_t*)"\r\n", 2); // Send a newline after the command
    
    int i = 0;
    while (i < (int)response_size) {
        if (nrfx_uart_rx(NRFX_UART0_INSTANCE, (uint8_t*)&response[i], 1) == NRFX_SUCCESS) {
            if (response[i] == '\n') break;  // Stop on newline
            i++;
        }
    }
    response[i] = '\0';  
    printf("[UART] Response: %s\n", response);
    return true;
}
// Check SIM card status
bool check_sim_status(void) {
    char response[256];
    uart_send_command("AT#XCCID", response, sizeof(response));  // Check SIM status
    if (strstr(response, "+CCID") == NULL) {
        printf("[ERROR] SIM card not detected\n");
        return false;
    }
    return true;
}

// Connect to the network
bool connect_network(void) {
    char response[256];

    // Check SIM card 
    if (!check_sim_status()) {
        return false;
    }

    // Attach to GPRS
    uart_send_command("AT#XCGATT=1", response, sizeof(response));
    if (strstr(response, "OK") == NULL) {
        printf("[ERROR] Failed to attach to the network\n");
        return false;
    }

    // Automatic operator selection
    uart_send_command("AT#XCOPS=0", response, sizeof(response));  
    uart_send_command("AT#XCOPS=1", response, sizeof(response));  // Register to network
    return true;
}

// Send HTTP connection request
bool send_telemetry(void) {
    char response[256];
    char cmd[512];  

    // connect to the HTTP server
    snprintf(cmd, sizeof(cmd), "AT#XHTTPCCON=\"%s\", %d, %d", SERVER_URL, SERVER_PORT, HTTPS);

    // Send the HTTP connection 
    uart_send_command(cmd, response, sizeof(response));
    if (strstr(response, "OK") == NULL) {
        printf("[ERROR] HTTP connection failed\n");
        return false;
    }

    //  HTTP POST request
    const char *body = "{\"count\":60}";  
    size_t body_length = strlen(body);
    char header[256];
    snprintf(header, sizeof(header), "POST " API_PATH " HTTP/1.1\r\nContent-Length: %zu\r\n\r\n", body_length);

    
    uart_send_command(header, response, sizeof(response)); 
    uart_send_command(body, response, sizeof(response));    

    // End the request 
    uart_send_command("AT#XHTTPCREQ=0", response, sizeof(response));  
    return strstr(response, "HTTP/1.1 200 OK") != NULL;
}


// Manage the connection with retries
void manage_connection(void) {
    int retries = 0;
    while (retries < MAX_RETRIES) {
        if (connect_network()) {
            printf("[NETWORK] Connected to network.\n");
            return;
        } else {
            printf("[NETWORK] Connection failed, retrying...\n");
            retries++;
            delay_ms(RETRY_DELAY_MS);
        }
    }

    //  system reset if network connection fails after retries
    printf("[ERROR] Too many retries. Resetting system...\n");
    NVIC_SystemReset();
}

// Delay function 
void delay_ms(int ms) {
    for (int i = 0; i < ms * 1000; i++) {
        __NOP();  
    }
}

int main(void) {
    uart_init();  

    // Try to manage the network connection and reconnect if needed
    manage_connection();

    while (1) {
        printf("[TELEMETRY] Sending telemetry...\n");

        
        if (!send_telemetry()) {
            printf("[ERROR] Telemetry failed. Reconnecting...\n");
            manage_connection();
        } else {
            printf("[TELEMETRY] Telemetry sent successfully.\n");
        }

        delay_ms(10000);  
    }

    return 0;
}





/****** End Of File *************************************************/
