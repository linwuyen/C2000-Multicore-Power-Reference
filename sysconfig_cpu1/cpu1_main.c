#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "sci_drv.h"
#include "power_bringup.h"

#define SCI_LINE_BUFFER_SIZE 64U

uint16_t CPU1HB;

static void SCI_WriteString(const char *text)
{
    while(*text != '\0')
    {
        SCI_WriteByte((uint16_t)*text);
        text++;
    }
}

void main(void)
{
    uint16_t lineBuffer[SCI_LINE_BUFFER_SIZE];
    uint16_t lineLength = 0U;
    uint16_t data;
    uint16_t i;
    bool ignoreLineFeed = false;

    Device_init();
    Device_initGPIO();

    /*
     * Establish board-level actuator safety before SysConfig or application
     * initialization can touch GPIO0/GPIO1 or any future power peripheral.
     */
    PowerBringup_InitEarlySafeState();

    Board_init();

    Interrupt_initModule();
    Interrupt_initVectorTable();

    SCI_Init();

    /*
     * EPWM1 is configured with OST latched and GPIO99 buffer disabled.
     * The function returns in READY and does not energize J5.
     */
    PowerBringup_InitPwm();

    for(;;)
    {
        CPU1HB++;

        /*
         * Debugger-driven ARM/STOP/duty requests are mediated here so SCI or
         * other communication code does not own direct PWM authority.
         */
        PowerBringup_Service();

        if(SCI_isDataAvailableNonFIFO(SCIA_BASE))
        {
            data = SCI_ReadByte();

            if(ignoreLineFeed && (data == '\n'))
            {
                ignoreLineFeed = false;
                continue;
            }

            ignoreLineFeed = false;

            if((data == '\r') || (data == '\n'))
            {
                SCI_WriteString("\r\nRX: ");

                for(i = 0U; i < lineLength; i++)
                {
                    SCI_WriteByte(lineBuffer[i]);
                }

                SCI_WriteString("\r\n");
                lineLength = 0U;
                ignoreLineFeed = (data == '\r');
            }
            else if((data == '\b') || (data == 0x7FU))
            {
                if(lineLength > 0U)
                {
                    lineLength--;
                    SCI_WriteString("\b \b");
                }
            }
            else if(lineLength < SCI_LINE_BUFFER_SIZE)
            {
                lineBuffer[lineLength] = data;
                lineLength++;
                SCI_WriteByte(data);
            }
            else
            {
                lineLength = 0U;
                SCI_WriteString("\r\nERR: LINE TOO LONG\r\n");
            }
        }
    }
}
