#ifndef POWER_BRINGUP_H
#define POWER_BRINGUP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    POWER_STATE_BOOT = 0,
    POWER_STATE_SAFE_INIT,
    POWER_STATE_READY,
    POWER_STATE_RUNNING,
    POWER_STATE_FAULT
} PowerBringupState;

typedef enum
{
    POWER_FAULT_NONE = 0,
    POWER_FAULT_DUTY_OUT_OF_RANGE
} PowerBringupFault;

typedef struct
{
    volatile uint16_t armRequest;
    volatile uint16_t stopRequest;
    volatile uint16_t dutyPermille;
} PowerBringupCommand;

typedef struct
{
    volatile PowerBringupState state;
    volatile PowerBringupFault fault;
    volatile uint16_t requestedDutyPermille;
    volatile uint16_t appliedDutyPermille;
    volatile uint16_t tbprd;
    volatile uint16_t cmpa;
    volatile uint16_t deadbandCount;
    volatile uint16_t tripStatus;
    volatile uint16_t bufferEnableLevel;
} PowerBringupDiag;

extern volatile PowerBringupCommand g_sPowerBringupCommand;
extern volatile PowerBringupDiag g_sPowerBringupDiag;

/*
 * Call immediately after Device_initGPIO() and before Board_init().
 * This establishes the board-level PWM buffer in its documented safe state.
 */
void PowerBringup_InitEarlySafeState(void);

/*
 * Configure EPWM1A/B while the external buffer is disabled and OST is latched.
 * Returns with the state in READY; it does not energize J5.
 */
void PowerBringup_InitPwm(void);

/*
 * Foreground-only command service. Commands are intentionally simple volatile
 * variables so the first bench can drive them from the debugger without
 * granting SCI/host code direct PWM authority.
 */
void PowerBringup_Service(void);

/* Force EPWM1A/B low through OST and disable the external PWM buffer. */
void PowerBringup_ForceSafe(PowerBringupFault fault);

#ifdef __cplusplus
}
#endif

#endif
