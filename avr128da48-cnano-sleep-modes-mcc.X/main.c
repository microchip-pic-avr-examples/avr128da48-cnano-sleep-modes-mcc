/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "avr/sleep.h"
#include "util/delay.h"

#define CMD_INITIAL                    0
#define BUFFER_TIME                    2
#define FLAG_SET                       true
#define FLAG_CLEAR                     false
#define SET_MODE_AND_SLEEP(x)          set_sleep_mode(x);  sleep_mode()
#define ENABLE_WAKEUP()                USART1.CTRLA = 0xD0; PORTC.PIN7CTRL = 0x09
#define DISABLE_WAKEUP()               USART1.CTRLA = 0x40; PORTC.PIN7CTRL = 0x00
#define PIT_DISABLE()                  RTC.PITCTRLA = 0x00     
#define PIT_ENABLE()                   RTC.PITCTRLA = 0x59

volatile bool PIT_flag = FLAG_CLEAR;

typedef uint8_t (*p_func_t)(void);

uint8_t getch(void);
void main_print_menu(void);
p_func_t main_parse_command(uint8_t cmd);
uint8_t cmd_active(void);
uint8_t cmd_idle(void);
uint8_t cmd_standby(void);
uint8_t cmd_power_down(void);
uint8_t cmd_cycle(void);
uint8_t cmd_wakeup(void);
uint8_t cmd_default(void);
void PIT_ISR(void);


/*
    Main application
*/
int main(void)
{
    uint8_t ch = CMD_INITIAL;
    p_func_t pExec = cmd_default;
    
    /* Initializes MCU, drivers and middleware */
    SYSTEM_Initialize();
    RTC_SetPITIsrCallback(PIT_ISR);

    /* Replace with your application code */
    while (1){
        if(ch == CMD_INITIAL)
        {
            main_print_menu();
            ch = getch();
        }
        pExec = main_parse_command(ch);
        ch = pExec();
        
    }
}

uint8_t getch(void)
{
    return Terminal_Read();
}

void main_print_menu(void)
{
    printf("\n\r");
    printf("*****************************\n\r");
    printf("Sleep Modes example\n\r");
    printf("Press a key corresponding to a sleep mode\n\r");
    printf("(case insensitive):\n\r");
    printf(" A - Active mode (not in sleep)\n\r");
    printf(" I - Idle Sleep mode\n\r");
    printf(" S - Standby Sleep mode (wake up through button)\n\r");
    printf(" P - Power Down Sleep mode (wake up through button)\n\r");
    printf(" C - Cycle through the modes\n\r");
    printf(" W - Deep sleep and short wake-up to simulate reading sensors\n\r");
    printf("*****************************\n\r");
    printf("Press any key or the button on board to stop the current demo.\n\r");
}

p_func_t main_parse_command(uint8_t cmd)
{
    switch(cmd)
    {
        case 'a':
        case 'A':  return cmd_active;
        case 'i':
        case 'I':  return cmd_idle;
        case 's':
        case 'S':  return cmd_standby;
        case 'p':
        case 'P':  return cmd_power_down;
        case 'c':
        case 'C':  return cmd_cycle;
        case 'w':
        case 'W':  return cmd_wakeup;
        default :  return cmd_default;
    }
}

uint8_t cmd_active(void)
{
    printf("\n\r");
    printf("Active mode.\n\r");
    printf("Press a key corresponding to a mode to enter that mode or any other key to see the menu again.\n\r");
    return getch();
}

uint8_t cmd_idle(void)
{
    printf("\n\r");
    printf("Entering idle sleep mode.\n\r");
    printf("Press a key corresponding to a mode to enter that mode or any other key to see the menu again.\n\r");
    _delay_ms(BUFFER_TIME);
    SET_MODE_AND_SLEEP(SLEEP_MODE_IDLE);
    return getch();
}

uint8_t cmd_standby(void)
{
    printf("\n\r");
    printf("Entering standby sleep mode. You can only exit this mode with a button press.\n\r");
    _delay_ms(BUFFER_TIME);
    SET_MODE_AND_SLEEP(SLEEP_MODE_STANDBY);
    printf("\n\r");
    printf("Press a key corresponding to a mode to enter that mode or any other key to see the menu again.\n\r");
    return getch();
}

uint8_t cmd_power_down(void)
{
    printf("\n\r");
    printf("Entering power down sleep mode. You can only exit this mode with a button press.\n\r");
    _delay_ms(BUFFER_TIME);
    SET_MODE_AND_SLEEP(SLEEP_MODE_PWR_DOWN);
    printf("\n\r");
    printf("Press a key corresponding to a mode to enter that mode or any other key to see the menu again.\n\r");
    return getch();
}

uint8_t cmd_cycle(void)
{
    uint8_t counter = 0;
    
    printf("\n\r");
    printf("Entering mode cycling program. Wait for the program to end before sending new data.\n\r");
    _delay_ms(BUFFER_TIME);
    
    DISABLE_WAKEUP();
    PIT_ENABLE();
    
    PIT_flag = FLAG_CLEAR;
    while(counter < 5)
    {
        if(PIT_flag == FLAG_SET)
        {
            PIT_flag = FLAG_CLEAR;
            counter ++;
            printf("\n\r");
            switch(counter)
            {
                case 1:   printf("Entering Active mode.\n\r");
                          break;
                case 2:   printf("Entering Idle sleep mode.\n\r");
                          _delay_ms(BUFFER_TIME);
                          SET_MODE_AND_SLEEP(SLEEP_MODE_IDLE);
                          break;
                case 3:   printf("Entering Standby sleep mode.\n\r");
                          _delay_ms(BUFFER_TIME);
                          SET_MODE_AND_SLEEP(SLEEP_MODE_STANDBY);
                          break;
                case 4:   printf("Entering Power Down sleep mode.\n\r");
                          _delay_ms(BUFFER_TIME);
                          SET_MODE_AND_SLEEP(SLEEP_MODE_PWR_DOWN);
                          break;
                default:  printf("Finished cycling through the modes.\n\r");
                          _delay_ms(BUFFER_TIME);
            }
        }
    }
    
    PIT_DISABLE();
    ENABLE_WAKEUP();
    
    printf("Press a key corresponding to a mode to enter that mode or any other key to see the menu again.\n\r");
    return getch();
}

uint8_t cmd_wakeup(void)
{
    uint8_t counter = 0;
    
    printf("\n\r");
    printf("Entering deep sleep and short wake up for sensor reading program.\n\r");
    printf("The program will run for 4 reading cycles. Wait for the program to finish before issuing new commands.\n\r");
    _delay_ms(BUFFER_TIME);
    
    DISABLE_WAKEUP();
    PIT_ENABLE();
    
    while(counter < 4)
    {
        if(PIT_flag == FLAG_SET)
        {
            PIT_flag = FLAG_CLEAR;
            counter++;
            SET_MODE_AND_SLEEP(SLEEP_MODE_PWR_DOWN);
            printf("Reading sensors %d.\n\r", counter);
            _delay_ms(BUFFER_TIME);
        }
    }
    
    PIT_DISABLE();
    ENABLE_WAKEUP();
    
    printf("Press a key corresponding to a mode to enter that mode or any other key to see the menu again.\n\r");
    return getch();
}

uint8_t cmd_default(void)
{
    main_print_menu();
    return getch();
}

void PIT_ISR(void)
{
    PIT_flag = FLAG_SET;
}


/**
    End of File
*/