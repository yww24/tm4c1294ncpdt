//*****************************************************************************
//
// timers_rtc.c - Timers example.
//
//  this file is modified from Tivaware and Many Shell related code is taken from
//  Kim YoonSeong's shell codes
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#define SIZE_COMMAND 20

uint32_t g_command_len;
uint8_t g_charactor;
uint8_t g_first,g_second;
uint8_t blinkflag;

char g_command[SIZE_COMMAND];
char g_buff_command[SIZE_COMMAND];
int g_i,g_j;


volatile int g_var_H, g_var_M, g_var_S;
volatile int g_var_Year, g_var_Month, g_var_Day;
volatile int g_alarm_H, g_alarm_M, g_alarm_S;

uint32_t g_ui32SysClock;
uint32_t g_rtc_sec;
uint32_t g_rtc_ala;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


void Timer0IntHandler(void) {
    TimerIntClear(TIMER0_BASE, TIMER_RTC_MATCH);

}

void ConfigureUART(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, g_ui32SysClock);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
    IntEnable(INT_UART0);
}
int GetCommand(){          //  알람 시간이     시간 : 분  : 초  로 입력이 됨
    UARTprintf("\r\033[1A");    // Cursor up by 1
    UARTprintf("\033[J");       // Erase the Screen from the current line down
                                // to the bottom of the screen
    uint32_t i,h=0,m=0,s=0;
    g_rtc_ala=0;

    // Strchr : 문자열에서 왼쪽에서 오른쪽으로 특정 문자 검사     <string.h>
    // g_command에서 ':' 로 되는 찾아서, ':'가 있는  포인터(주소)를 알려줌
    // https://dojang.io/mod/page/view.php?id=369
    g_first = strchr(g_command, ':')-g_command; //g_command에서 몇개가 알람 h인지 파악

    // Strrchr : 문자열에서 오른쪽에서 왼쪽으로 ':'가 있는 포인터를 찾아줌  (즉 마지막 ':' 임)    <string.h>
    // 이 예제에서는 g_command에서  두번째 ':' 있는 부분을 찾아서, 두번째 ':'가 있는  포인터(주소)를 알려줌
    // https://dojang.io/mod/page/view.php?id=370
    g_second = strrchr(g_command,':')-g_command;  //g_command에서 몇개가 알람 (h + m) 인지 파악
    if(g_first==g_second){

//        g_reserve=false;
        TimerMatchSet(TIMER0_BASE, TIMER_A, 0xFFFFFFFF);
        g_alarm_H=g_alarm_M=g_alarm_S=0;
        return 0;
    }

    //  alarm input frame is   h   : m  : s  --> 현재에서 얼마 후에 알람을 울릴지를 넣음
    //  getting hour and minute and second for alarm
    //
    for (i = 0; i < g_first; i++) { // 알람용 H에 해당하는 숫자
        h *= 10;                    // 이 부분은  Tera Term 에서 123(문자임)을
        h += g_command[i] - '0';    // 어떻게 숫자 123으로 할 것인지 처리한 코드임
    }
    for (i = g_first + 1; i < g_second; i++) {  // 알람용 M은 g_first 와 g_second 위치
        m *= 10;                    // 이 부분은  Tera Term 에서 123(문자임)을
        m += g_command[i] - '0';    // 어떻게 숫자 123으로 할 것인지 처리한 코드임
    }
    for (i = g_second + 1; i < g_command_len; i++) {
        s *= 10;                    // 이 부분은  Tera Term 에서 123(문자임)을
        s += g_command[i] - '0';    // 어떻게 숫자 123으로 할 것인지 처리한 코드임
    }
    g_rtc_ala=(h*3600)+(m*60)+s;    // match register에 넣을 값
    TimerMatchSet(TIMER0_BASE, TIMER_A, g_rtc_sec+g_rtc_ala);
    g_alarm_H = (g_rtc_sec+g_rtc_ala)/3600;     // 알람이 일어나는 시간을 계산
    g_alarm_M = ((g_rtc_sec+g_rtc_ala) / 60) % 3600;  // 알람이 일어나는 분을 계산
    g_alarm_S = (g_rtc_sec+g_rtc_ala) % 60;     // 알람이 일어나는 초를 계산

    return 1;
}

void UARTget(){
    if(((g_charactor>='0'&&g_charactor<='9') || g_charactor==':') && g_command_len<SIZE_COMMAND-1){
        g_command[g_command_len++]=g_charactor;
        UARTprintf("%c",g_charactor);
        return;
    }
    if(g_charactor == 8){      // ASCII code 8  --> backspace
        UARTprintf("\b \b");
        if(g_command_len>0)   g_command[--g_command_len] = 0;
        return;
    }
    if(g_charactor == 13){    // ASCII code 13  --> return
        UARTprintf("\r\n");
        g_command[g_command_len]=0;
        if(GetCommand())    UARTprintf("ERROR : Syntax error occurred please enter command again\n");
        for(g_i=g_command_len-1,g_command_len=0;g_i>=0;g_i--) g_command[g_i]=0;
        return;
    }
    for(g_i=g_command_len-1,g_command_len=0;g_i>=0;g_i--) g_command[g_i]=0;
    UARTprintf("\nYou have typed an unregistered character. Please enter command again.\n");

}

void Int_UART0(){
    UARTIntClear(UART0_BASE, UARTIntStatus(UART0_BASE, true));
    g_charactor = UARTgetc();
    UARTget();
}


void RTC_clock(int rtc_spd)
{
    volatile uint32_t ui32Loop;
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    for(ui32Loop = 0; ui32Loop < rtc_spd; ui32Loop++) {};
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0x0);
    for(ui32Loop = 0; ui32Loop < rtc_spd; ui32Loop++);
}



int main(void)
{

    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
            SYSCTL_OSC_MAIN |
            SYSCTL_USE_PLL |
            SYSCTL_CFG_VCO_480), 120000000);
    // Enable the GPIO port that is used for the on-board LEDs.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));

    // Enable the GPIO pins for the LEDs (PN0 & PN1).
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Enable the peripherals used by this example.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));

    GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinConfigure(GPIO_PD0_T0CCP0);

    // Enable processor interrupts.
    IntMasterEnable();

    // Configure the two 32-bit periodic timers.
    TimerConfigure(TIMER0_BASE, TIMER_CFG_RTC);
    TimerMatchSet(TIMER0_BASE, TIMER_A, 0xFFFFFFFF);

    // Setup the interrupts for the timer timeouts.
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_RTC_MATCH);

    TimerEnable(TIMER0_BASE, TIMER_A);

    // Enable USR_SW1,2
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)){}

    GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1, GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    IntEnable(INT_GPIOJ);


    ConfigureUART();
    g_var_S=g_var_M=g_var_H=0;
    g_var_Year = 2013;
    g_var_Month = 1;
    g_var_Day = 1;

    while(1){
        if((TimerValueGet(TIMER0_BASE, TIMER_A)%60)!=g_var_S){
            g_rtc_sec = TimerValueGet(TIMER0_BASE, TIMER_A);
            g_var_S = g_rtc_sec % 60;
            g_var_M = (g_rtc_sec / 60) % 3600;
            g_var_H = g_rtc_sec / 3600;

            if(g_var_S%2 == 0){
                UARTprintf("\033[2A");
                UARTprintf("\033[J");
//              UARTprintf("\r\033[7A");    // Cursor up by 1
                UARTprintf("%d:%d  \n", g_var_H, g_var_M);
                UARTprintf("%d년 %d월 %d일 (Thus)\n", g_var_Year, g_var_Month, g_var_Day );
            }
            else {
                UARTprintf("\033[2A");
                UARTprintf("\033[J");
//              UARTprintf("\r\033[7A");    // Cursor up by 1
                UARTprintf("%d %d  \n", g_var_H, g_var_M);
                UARTprintf("%d년 %d월 %d일 (Thus)\n", g_var_Year, g_var_Month, g_var_Day );
            }
        }
        RTC_clock(200);
    }
}
