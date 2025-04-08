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
#include <time.h>
#include <nrf.h>              
#include <nrfx_uart.h>        
#include <core_cm4.h>
#include <nrfx_uart.h>  
#include <nrf.h>         


#define MAX_RETRIES 3
#define RETRY_DELAY_MS 2000


#define UART_TX_PIN 6
#define UART_RX_PIN 8

// UART configuration
#define UART_BAUD_RATE NRF_UART_BAUDRATE_115200
#define UART_RXD_READY_Msk (1UL << NRF_UART_EVENT_RXDRDY) 


// Function prototypes
static bool uart_send_command(const char *cmd, char *response, size_t response_size);
static bool check_sim_status(void);
static bool connect_network(void);
static void disconnect_network(void);
static bool send_telemetry(void);
static void delay_ms(int ms);
static void manage_connection(void);
void uart_init(void);  



// UART initialization
void uart_init(void) {
    nrfx_uart_config_t config = NRFX_UART_DEFAULT_CONFIG;
    config.baudrate = NRF_UART_BAUDRATE_115200;
    config.hwfc = NRF_UART_HWFC_DISABLED;
    config.parity = NRF_UART_PARITY_EXCLUDED;
}

// Send AT command 
bool uart_send_command(const char *cmd, char *response, size_t response_size) {
    
    NRF_UART0->TXD = *cmd;  
    
        int i = 0;
    while (i < response_size && (NRF_UART0->RXD & UART_RXD_READY_Msk) != 0) {
        response[i] = (char)NRF_UART0->RXD;
        i++;
    }

    response[i] = '\0';
    printf("[UART] Sent: %s\n", cmd);
    printf("[UART] Response: %s\n", response);
    return true;
}

// Check SIM status
bool check_sim_status() {
    char response[256];
    uart_send_command("AT+CCID", response, sizeof(response));
    return strstr(response, "+CCID") != NULL;
}

// Connect to the network
bool connect_network() {
    char response[256];
    uart_send_command("AT+COPS=0", response, sizeof(response));
    uart_send_command("AT+CGATT=1", response, sizeof(response));
    uart_send_command("AT+CGACT=1,1", response, sizeof(response));
    uart_send_command("AT+CEREG?", response, sizeof(response));
    return strstr(response, "+CEREG: 1") != NULL;
}

// Disconnect from the network
void disconnect_network() {
    char response[256];
    uart_send_command("AT+CGATT=0", response, sizeof(response));
    uart_send_command("AT+CGACT=0,1", response, sizeof(response));
    uart_send_command("AT+COPS=2", response, sizeof(response));
}

// Send telemetry data
bool send_telemetry() {
    char response[256];
    uart_send_command("AT+HTTPINIT", response, sizeof(response));
    uart_send_command("AT+HTTPPARA=\"CID\",1", response, sizeof(response));
    uart_send_command("AT+HTTPPARA=\"URL\",\"https://eok6huncgyppkot.m.pipedream.net\"", response, sizeof(response));
    uart_send_command("AT+HTTPDATA=50,5000", response, sizeof(response));
    uart_send_command("{\"people_counting\":25}", response, sizeof(response));
    
    uart_send_command("AT+HTTPACTION=1", response, sizeof(response));
    bool success = strstr(response, "200") != NULL;

    uart_send_command("AT+HTTPTERM", response, sizeof(response));
    return success;
}

// Delay function
void delay_ms(int ms) {
    delay_ms(ms);  
}

// Manage network connection 
void manage_connection() {
    int retries = 0;
    while (retries < MAX_RETRIES) {
        if (!check_sim_status()) {
            printf("[SIM] SIM check failed, retrying...\n");
            delay_ms(RETRY_DELAY_MS);
            retries++;
            continue;
        }

        if (connect_network()) {
            printf("[NETWORK] Connected to network.\n");
            return;
        } else {
            printf("[NETWORK] Connection failed, retrying...\n");
            retries++;
            delay_ms(RETRY_DELAY_MS);
        }
    }

    printf("[SYSTEM] Too many failures. Simulate system reset.\n");
    NVIC_SystemReset();  
}

int main(void) {
    // Initialize UART 
    NRF_UART0->BAUDRATE = UART_BAUD_RATE;
    NRF_UART0->PSEL.TXD = UART_TX_PIN;
    NRF_UART0->PSEL.RXD = UART_RX_PIN;

    manage_connection();

    while (1) {
        printf("[TELEMETRY] Sending telemetry...\n");
        if (!send_telemetry()) {
            printf("[ERROR] Telemetry failed! Reconnecting...\n");
            disconnect_network();
            manage_connection();
        } else {
            printf("[TELEMETRY] Success.\n");
        }

        delay_ms(10000); // Send  every 10 seconds
    }

    return 0;
}


/****** End Of File *************************************************/
