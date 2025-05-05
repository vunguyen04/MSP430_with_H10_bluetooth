#include <msp430.h>
#include "LiquidCrystal_I2C.h"
#include <stdlib.h>  // For atoi()

char input_buffer[5] = "";   // Buffer for received characters
int input_index = 0;

void setup_uart(void);
void setup_pwm(void);
void setup_lcd(void);
void update_pwm_and_display(int duty);

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    P1DIR |= BIT0;  // Set P1.0 (LED) as output
    P1OUT &= ~BIT0; // Turn off LED initially

    // Configure UART RX Pin
    P4SEL1 &= ~BIT2;
    P4SEL0 |= BIT2;
    // Configure UART TX Pin
    P4SEL1 &= ~BIT3;
    P4SEL0 |= BIT3;
    setup_uart();


    setup_pwm();
    setup_lcd();

    UCA1IE |= UCRXIE;
    __enable_interrupt();       // Enable global interrupts

    while(1)
    {
        //echo_uart();  // Continuously echo received data
        // Nothing to do here; work is interrupt-driven
    }
}

// UART setup
void setup_uart(void)
{
    // Configure UART
    UCA1CTLW0 |= UCSWRST;           // Put eUSCI in reset
    UCA1CTLW0 |= UCSSEL__SMCLK;      // Use SMCLK
    UCA1BR0 = 104;                  // 1MHz 9600 baud
    UCA1MCTLW = 0x1100;             // Modulation
    UCA1CTLW0 &= ~UCSWRST;          // Initialize eUSCI
}


// PWM setup
void setup_pwm(void)
{
    P6DIR |= BIT1;     // P6.1 output
    P6SEL0 |= BIT1;    // TB3.2 function
    P6SEL1 &= ~BIT1;

    TB3CCR0 = 655;                  // PWM Period (100% duty)
    TB3CCTL2 = OUTMOD_7;             // Reset/Set output
    TB3CCR2 = 0;                     // Start with 0% duty
    TB3CTL = TBSSEL_1 | MC_1 | TBCLR; // ACLK, Up mode, Clear
}

// LCD setup
void setup_lcd(void)
{
    I2C_Init(0x27);       // LCD I2C address
    LCD_Setup();
    LCD_SetCursor(0, 0);
    LCD_Write("Waiting Input...");
}

void echo_uart(void)
{
    if (UCA0IFG & UCRXIFG) {
        char received = UCA0RXBUF;
        UCA0TXBUF = received;  // Echo the received character
    }
}
// UART ISR
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    if (UCA0IFG & UCRXIFG)
    {
        //P1OUT ^= BIT0;  // Toggle LED when a character is received (for debugging)

        char received = UCA0RXBUF;

        if (received == 't')
        {
            P1OUT ^= BIT0;  // Toggle LED
        }
        if (received == '\n' || received == '\r') // End of input
        {
            input_buffer[input_index] = '\0';  // Null terminate
            int pwm_val = atoi(input_buffer);  // Convert to integer

            if (pwm_val > 100)
                pwm_val = 100;  // Clamp to 100%

            update_pwm_and_display(pwm_val);

            input_index = 0;    // Reset buffer for next input
            input_buffer[0] = '\0';
        }
        else
        {
            if (input_index < sizeof(input_buffer) - 1)
            {
                input_buffer[input_index++] = received; // Store character
            }
        }
    }
}

// Function to update PWM and display the value
void update_pwm_and_display(int duty)
{
    TB3CCR2 = (duty * TB3CCR0) / 100;  // Update PWM duty cycle

    LCD_ClearDisplay();
    LCD_SetCursor(0, 0);
    LCD_Write("PWM:");
    LCD_WriteNum(duty);
    LCD_Write("%");
}
