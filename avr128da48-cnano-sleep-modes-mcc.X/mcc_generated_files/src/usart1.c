/**
  @Company
    Microchip Technology Inc.

  @Description
    This Source file provides APIs.
    Generation Information :
    Driver Version    :   1.0.0
*/
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


#include "../include/usart1.h"

#if defined(__GNUC__)

int Terminal_printCHAR(char character, FILE *stream)
{
    Terminal_Write(character);
    return 0;
}

FILE Terminal_stream = FDEV_SETUP_STREAM(Terminal_printCHAR, NULL, _FDEV_SETUP_WRITE);

#elif defined(__ICCAVR__)

int putchar(int outChar)
{
    Terminal_Write(outChar);
    return outChar;
}
#endif

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          Terminal_rxbuf[Terminal_RX_BUFFER_SIZE];
static volatile uint8_t Terminal_rx_head;
static volatile uint8_t Terminal_rx_tail;
static volatile uint8_t Terminal_rx_elements;
static uint8_t          Terminal_txbuf[Terminal_TX_BUFFER_SIZE];
static volatile uint8_t Terminal_tx_head;
static volatile uint8_t Terminal_tx_tail;
static volatile uint8_t Terminal_tx_elements;

void (*Terminal_rx_isr_cb)(void) = &Terminal_DefaultRxIsrCb;

void (*Terminal_tx_isr_cb)(void) = &Terminal_DefaultTxIsrCb;

void Terminal_DefaultRxIsrCb(void)
{
    uint8_t data;
    uint8_t tmphead;

    /* Read the received data */
    data = USART1.RXDATAL;
    /* Calculate buffer index */
    tmphead = (Terminal_rx_head + 1) & Terminal_RX_BUFFER_MASK;
        
    if (tmphead == Terminal_rx_tail) {
            /* ERROR! Receive buffer overflow */
    }else {
    /*Store new index*/
    Terminal_rx_head = tmphead;
    
    /* Store received data in buffer */
    Terminal_rxbuf[tmphead] = data;
    Terminal_rx_elements++;
    }
}

void Terminal_DefaultTxIsrCb(void)
{
    uint8_t tmptail;

    /* Check if all data is transmitted */
    if (Terminal_tx_elements != 0) {
        /* Calculate buffer index */
        tmptail = (Terminal_tx_tail + 1) & Terminal_TX_BUFFER_MASK;
        /* Store new index */
        Terminal_tx_tail = tmptail;
        /* Start transmission */
        USART1.TXDATAL = Terminal_txbuf[tmptail];
        
        Terminal_tx_elements--;
    }

    if (Terminal_tx_elements == 0) {
            /* Disable Tx interrupt */
            USART1.CTRLA &= ~(1 << USART_DREIE_bp);
    }
}

void Terminal_SetISRCb(usart_callback cb, terminal_cb_t type)
{
    switch (type) {
        case Terminal_RX_CB:
                Terminal_rx_isr_cb = cb;
                break;
        case Terminal_TX_CB:
                Terminal_tx_isr_cb = cb;
                break;
        default:
                // do nothing
                break;
    }
}

void Terminal_SetRXISRCb(usart_callback cb)
{
    Terminal_SetISRCb(cb,Terminal_RX_CB);
}

void Terminal_SetTXISRCb(usart_callback cb)
{
    Terminal_SetISRCb(cb,Terminal_TX_CB);
}

/* Interrupt service routine for RX complete */
ISR(USART1_RXC_vect)
{
    if (Terminal_rx_isr_cb != NULL)
    {
        (*Terminal_rx_isr_cb)();
    }
}

/* Interrupt service routine for Data Register Empty */
ISR(USART1_DRE_vect)
{
    if (Terminal_tx_isr_cb != NULL)
    {
        (*Terminal_tx_isr_cb)();
    }
}

ISR(USART1_TXC_vect)
{
    USART1.STATUS |= USART_TXCIF_bm;
}

bool Terminal_IsTxReady()
{
    return (Terminal_tx_elements != Terminal_TX_BUFFER_SIZE);
}

bool Terminal_IsRxReady()
{
    return (Terminal_rx_elements != 0);
}

bool Terminal_IsTxBusy()
{
    return (!(USART1.STATUS & USART_TXCIF_bm));
}

bool Terminal_IsTxDone()
{
    return (USART1.STATUS & USART_TXCIF_bm);
}

uint8_t Terminal_Read(void)
{
    uint8_t tmptail;

    /* Wait for incoming data */
    while (Terminal_rx_elements == 0)
            ;
    /* Calculate buffer index */
    tmptail = (Terminal_rx_tail + 1) & Terminal_RX_BUFFER_MASK;
    /* Store new index */
    Terminal_rx_tail = tmptail;
    ENTER_CRITICAL(R);
    Terminal_rx_elements--;
    EXIT_CRITICAL(R);

    /* Return data */
    return Terminal_rxbuf[tmptail];
}

void Terminal_Write(const uint8_t data)
{
    uint8_t tmphead;

    /* Calculate buffer index */
    tmphead = (Terminal_tx_head + 1) & Terminal_TX_BUFFER_MASK;
    /* Wait for free space in buffer */
    while (Terminal_tx_elements == Terminal_TX_BUFFER_SIZE)
            ;
    /* Store data in buffer */
    Terminal_txbuf[tmphead] = data;
    /* Store new index */
    Terminal_tx_head = tmphead;
    ENTER_CRITICAL(W);
    Terminal_tx_elements++;
    EXIT_CRITICAL(W);
    /* Enable Tx interrupt */
    USART1.CTRLA |= (1 << USART_DREIE_bp);
}

void Terminal_Initialize()
{
    //set baud rate register
    USART1.BAUD = (uint16_t)USART1_BAUD_RATE(115200);
	
    //RXCIE enabled; TXCIE enabled; DREIE disabled; RXSIE enabled; LBME disabled; ABEIE disabled; RS485 OFF; 
    USART1.CTRLA = 0xD0;
	
    //RXEN enabled; TXEN enabled; SFDEN disabled; ODME disabled; RXMODE NORMAL; MPCM disabled; 
    USART1.CTRLB = 0xC0;
	
    //CMODE ASYNCHRONOUS; PMODE DISABLED; SBMODE 1BIT; CHSIZE 8BIT; UDORD disabled; UCPHA disabled; 
    USART1.CTRLC = 0x03;
	
    //DBGCTRL_DBGRUN
    USART1.DBGCTRL = 0x00;
	
    //EVCTRL_IREI
    USART1.EVCTRL = 0x00;
	
    //RXPLCTRL_RXPL
    USART1.RXPLCTRL = 0x00;
	
    //TXPLCTRL_TXPL
    USART1.TXPLCTRL = 0x00;
	

    uint8_t x;

    /* Initialize ringbuffers */
    x = 0;

    Terminal_rx_tail     = x;
    Terminal_rx_head     = x;
    Terminal_rx_elements = x;
    Terminal_tx_tail     = x;
    Terminal_tx_head     = x;
    Terminal_tx_elements = x;

#if defined(__GNUC__)
    stdout = &Terminal_stream;
#endif

}

void Terminal_Enable()
{
    USART1.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}

void Terminal_EnableRx()
{
    USART1.CTRLB |= USART_RXEN_bm;
}

void Terminal_EnableTx()
{
    USART1.CTRLB |= USART_TXEN_bm;
}

void Terminal_Disable()
{
    USART1.CTRLB &= ~(USART_RXEN_bm | USART_TXEN_bm);
}

uint8_t Terminal_GetData()
{
    return USART1.RXDATAL;
}