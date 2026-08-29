#include "driverlib.h"
#include "device.h"
#include "power_bringup.h"

#define POWER_PWM_BASE                    EPWM1_BASE
#define POWER_PWM_BUFFER_GPIO             99U
#define POWER_PWM_BUFFER_DISABLE_LEVEL    1U
#define POWER_PWM_BUFFER_ENABLE_LEVEL     0U

#define POWER_PWM_FREQUENCY_HZ            20000UL
#define POWER_PWM_INITIAL_DUTY_PERMILLE   100U
#define POWER_PWM_MIN_DUTY_PERMILLE       50U
#define POWER_PWM_MAX_DUTY_PERMILLE       900U

/* EPWMCLK = SYSCLK and TBCLK prescalers are both /1 for this M1 bring-up. */
#define POWER_PWM_TBPRD                   ((uint16_t)(DEVICE_SYSCLK_FREQ / (2UL * POWER_PWM_FREQUENCY_HZ)))
#define POWER_PWM_DEADBAND_COUNT          ((uint16_t)(DEVICE_SYSCLK_FREQ / 2000000UL))

volatile PowerBringupCommand g_sPowerBringupCommand =
{
    0U,
    0U,
    POWER_PWM_INITIAL_DUTY_PERMILLE
};

volatile PowerBringupDiag g_sPowerBringupDiag =
{
    POWER_STATE_BOOT,
    POWER_FAULT_NONE,
    POWER_PWM_INITIAL_DUTY_PERMILLE,
    0U,
    POWER_PWM_TBPRD,
    0U,
    POWER_PWM_DEADBAND_COUNT,
    0U,
    POWER_PWM_BUFFER_DISABLE_LEVEL
};

static uint16_t PowerBringup_DutyToCmpA(uint16_t dutyPermille)
{
    uint32_t offTimeCounts;

    /*
     * AQ makes EPWMA high from up-CMPA until down-CMPA, so duty is
     * (TBPRD - CMPA) / TBPRD in up/down mode.
     */
    offTimeCounts = ((uint32_t)POWER_PWM_TBPRD *
                     (uint32_t)(1000U - dutyPermille)) / 1000UL;

    return (uint16_t)offTimeCounts;
}

static bool PowerBringup_ApplyDuty(uint16_t dutyPermille)
{
    uint16_t cmpa;

    g_sPowerBringupDiag.requestedDutyPermille = dutyPermille;

    if((dutyPermille < POWER_PWM_MIN_DUTY_PERMILLE) ||
       (dutyPermille > POWER_PWM_MAX_DUTY_PERMILLE))
    {
        PowerBringup_ForceSafe(POWER_FAULT_DUTY_OUT_OF_RANGE);
        return false;
    }

    cmpa = PowerBringup_DutyToCmpA(dutyPermille);
    EPWM_setCounterCompareValue(POWER_PWM_BASE,
                                EPWM_COUNTER_COMPARE_A,
                                cmpa);

    g_sPowerBringupDiag.cmpa = cmpa;
    g_sPowerBringupDiag.appliedDutyPermille = dutyPermille;
    return true;
}

static void PowerBringup_BufferDisable(void)
{
    GPIO_writePin(POWER_PWM_BUFFER_GPIO, POWER_PWM_BUFFER_DISABLE_LEVEL);
    g_sPowerBringupDiag.bufferEnableLevel = GPIO_readPin(POWER_PWM_BUFFER_GPIO);
}

static void PowerBringup_BufferEnable(void)
{
    GPIO_writePin(POWER_PWM_BUFFER_GPIO, POWER_PWM_BUFFER_ENABLE_LEVEL);
    g_sPowerBringupDiag.bufferEnableLevel = GPIO_readPin(POWER_PWM_BUFFER_GPIO);
}

void PowerBringup_InitEarlySafeState(void)
{
    /*
     * Board documentation identifies GPIO99 as active-low buffer enable.
     * Explicitly reclaim CPU1 ownership because this is a multicore workspace.
     * Set the output latch HIGH before making the pin an output so software
     * does not intentionally create an enable glitch during direction setup.
     */
    GPIO_setControllerCore(POWER_PWM_BUFFER_GPIO, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_99_GPIO99);
    GPIO_writePin(POWER_PWM_BUFFER_GPIO, POWER_PWM_BUFFER_DISABLE_LEVEL);
    GPIO_setPadConfig(POWER_PWM_BUFFER_GPIO, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(POWER_PWM_BUFFER_GPIO, GPIO_DIR_MODE_OUT);

    PowerBringup_BufferDisable();

    g_sPowerBringupDiag.state = POWER_STATE_SAFE_INIT;
    g_sPowerBringupDiag.fault = POWER_FAULT_NONE;
}

void PowerBringup_InitPwm(void)
{
    /* Stop all ePWM time-base clocks while EPWM1 is being configured. */
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    EPWM_setClockPrescaler(POWER_PWM_BASE,
                           EPWM_CLOCK_DIVIDER_1,
                           EPWM_HSCLOCK_DIVIDER_1);
    EPWM_setTimeBasePeriod(POWER_PWM_BASE, POWER_PWM_TBPRD);
    EPWM_setTimeBaseCounter(POWER_PWM_BASE, 0U);
    EPWM_setTimeBaseCounterMode(POWER_PWM_BASE, EPWM_COUNTER_MODE_UP_DOWN);

    EPWM_setCounterCompareShadowLoadMode(POWER_PWM_BASE,
                                         EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);

    /* Base waveform: centered active-high pulse around TBCTR = TBPRD. */
    EPWM_setActionQualifierAction(POWER_PWM_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(POWER_PWM_BASE,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);

    /* Active-high complementary pair derived from EPWMA with dead band. */
    EPWM_setRisingEdgeDeadBandDelayInput(POWER_PWM_BASE, EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(POWER_PWM_BASE, EPWM_DB_INPUT_EPWMA);
    EPWM_setDeadBandDelayMode(POWER_PWM_BASE, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(POWER_PWM_BASE, EPWM_DB_FED, true);
    EPWM_setDeadBandDelayPolarity(POWER_PWM_BASE,
                                  EPWM_DB_RED,
                                  EPWM_DB_POLARITY_ACTIVE_HIGH);
    EPWM_setDeadBandDelayPolarity(POWER_PWM_BASE,
                                  EPWM_DB_FED,
                                  EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setDeadBandCounterClock(POWER_PWM_BASE,
                                 EPWM_DB_COUNTER_CLOCK_FULL_CYCLE);
    EPWM_setRisingEdgeDelayCount(POWER_PWM_BASE, POWER_PWM_DEADBAND_COUNT);
    EPWM_setFallingEdgeDelayCount(POWER_PWM_BASE, POWER_PWM_DEADBAND_COUNT);

    /* A software OST must force both connector-intended PWM outputs low. */
    EPWM_setTripZoneAction(POWER_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZA,
                           EPWM_TZ_ACTION_LOW);
    EPWM_setTripZoneAction(POWER_PWM_BASE,
                           EPWM_TZ_ACTION_EVENT_TZB,
                           EPWM_TZ_ACTION_LOW);

    if(!PowerBringup_ApplyDuty(POWER_PWM_INITIAL_DUTY_PERMILLE))
    {
        /* Preserve the FAULT state if configuration constants are invalid. */
        SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
        return;
    }

    EPWM_forceTripZoneEvent(POWER_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    g_sPowerBringupDiag.tripStatus = EPWM_getTripZoneFlagStatus(POWER_PWM_BASE);

    /*
     * The previous workspace assigned GPIO1 to CPU2. Reclaim both EPWM pins
     * explicitly so CPU1-only debug reloads do not depend on reset defaults.
     * Expose the EPWM mux only after OST behavior is already established.
     */
    GPIO_setControllerCore(0U, GPIO_CORE_CPU1);
    GPIO_setControllerCore(1U, GPIO_CORE_CPU1);
    GPIO_setPadConfig(0U, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(1U, GPIO_PIN_TYPE_STD);
    GPIO_setPinConfig(GPIO_0_EPWM1A);
    GPIO_setPinConfig(GPIO_1_EPWM1B);

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    PowerBringup_BufferDisable();
    g_sPowerBringupDiag.state = POWER_STATE_READY;
}

void PowerBringup_ForceSafe(PowerBringupFault fault)
{
    EPWM_forceTripZoneEvent(POWER_PWM_BASE, EPWM_TZ_FORCE_EVENT_OST);
    PowerBringup_BufferDisable();

    g_sPowerBringupDiag.tripStatus = EPWM_getTripZoneFlagStatus(POWER_PWM_BASE);

    if(fault != POWER_FAULT_NONE)
    {
        g_sPowerBringupDiag.fault = fault;
        g_sPowerBringupDiag.state = POWER_STATE_FAULT;
    }
    else if(g_sPowerBringupDiag.state != POWER_STATE_FAULT)
    {
        /* STOP is safe but is not fault-clear authority. */
        g_sPowerBringupDiag.state = POWER_STATE_READY;
    }
}

void PowerBringup_Service(void)
{
    uint16_t tripStatus;

    /*
     * FAULT is latched in this first slice. Recovery is intentionally omitted;
     * a reset/reload is required so STOP or a new duty cannot clear the latch.
     */
    if(g_sPowerBringupDiag.state == POWER_STATE_FAULT)
    {
        g_sPowerBringupCommand.armRequest = 0U;
        g_sPowerBringupCommand.stopRequest = 0U;
        PowerBringup_BufferDisable();
        g_sPowerBringupDiag.tripStatus = EPWM_getTripZoneFlagStatus(POWER_PWM_BASE);
        return;
    }

    if(g_sPowerBringupCommand.stopRequest != 0U)
    {
        g_sPowerBringupCommand.stopRequest = 0U;
        g_sPowerBringupCommand.armRequest = 0U;
        PowerBringup_ForceSafe(POWER_FAULT_NONE);
        return;
    }

    if(g_sPowerBringupCommand.dutyPermille !=
       g_sPowerBringupDiag.requestedDutyPermille)
    {
        if(!PowerBringup_ApplyDuty(g_sPowerBringupCommand.dutyPermille))
        {
            g_sPowerBringupCommand.armRequest = 0U;
            return;
        }
    }

    if((g_sPowerBringupCommand.armRequest != 0U) &&
       (g_sPowerBringupDiag.state == POWER_STATE_READY))
    {
        g_sPowerBringupCommand.armRequest = 0U;

        /*
         * Clear the internal trip while the board buffer is still disabled.
         * The buffer is released only after the OST status reads back clear.
         */
        EPWM_clearTripZoneFlag(POWER_PWM_BASE, EPWM_TZ_FLAG_OST);
        tripStatus = EPWM_getTripZoneFlagStatus(POWER_PWM_BASE);
        g_sPowerBringupDiag.tripStatus = tripStatus;

        if((tripStatus & EPWM_TZ_FLAG_OST) == 0U)
        {
            PowerBringup_BufferEnable();
            g_sPowerBringupDiag.state = POWER_STATE_RUNNING;
        }
        else
        {
            PowerBringup_BufferDisable();
        }
    }

    g_sPowerBringupDiag.tripStatus = EPWM_getTripZoneFlagStatus(POWER_PWM_BASE);
}
