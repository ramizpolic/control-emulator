/*
 * Class: Microcontroller based software engineering I.
 * Date: 22/04/2021 
 * Project: Microwave
 * 
 * Name: Ramiz Polic
 * 
 * Notes: 
 *  - USART Transmit is enabled
 *  - Interrupts for S1, S2, POT are replaced with polling logic,
 *    since interrupts introduce problems such as
 *    state management, refetched polling, delays, and threading issues.
 *  - Imports "baking.h"
 * 
 * Operation:
 *  - Operating at 4 MHz
 *  - Baud rate at 9600
 *  - Interrupt  (10ms TM2)  => handles states management & configuration
 *  - Main logic (50ms)      => UART transmission of formatted internal states
 */

#include "baking.h"
#include "mcc_generated_files/mcc.h"

/*********************************
 * Application-specific definitions
 *********************************/
#define POT_MAX_VALUE                       1024        /* max pot value */
#define TIMER_PERIOD_MILISECONDS            10          /* TM2 period */
#define STATES_REFRESH_MILISECONDS          100         /* refetches user states such as S1,S2,POT 
                                                         * and invokes On100msElapsed method on Baker
                                                         */
#define BAKING_MAX_TIMER_SECONDS            2*60.0
#define BAKING_TIMER_INTERVAL_SECONDS       20

/*********************************
 * Constants
 *********************************/
/* Controls the size of interval based on pot value to determine microwave timer segment.
 * Example: 1024 / (120.0 / 20) = 1024 / 6
 */
const float CONST_POT_MW_TIMER_INTERVAL_GROUP_SIZE = (float)(POT_MAX_VALUE / 
                                                    (BAKING_MAX_TIMER_SECONDS / BAKING_TIMER_INTERVAL_SECONDS));

/*********************************
 * State Controller Interface    
 *********************************/
typedef struct {
    /* User-controlled states */
    bool OperationState;            /* selected S1 state */
    bool DoorState;                 /* selected S2 state */
    
    uint8_t SelectedTimerSegment;   /* selected microwave timer segment
                                     * o    ranges = [1, 6]
                                     * o    selected time -> range * 20s  */
    
    /* Timer controlled states */
    uint16_t Counter;
    
    /* BAKER CONTROLLER */
    BakingInterface* Baker;
} StateInterface;

/* State Controller manager, dynamic, initialized */
volatile StateInterface StateController = {
    .OperationState = true,
    .DoorState = true,
    .SelectedTimerSegment = 0,
    .Counter = 0,
};

/*********************************
 * Events & Helpers
 *********************************/
void RefreshDoorStateButton(void);
void RefreshOperationStateButton(void);
void RefreshTimerInterval(void);
uint8_t GetMicrowaveTimerSegmentFromPot(void);

/*********************************
 * Interrupts
 *********************************/
void OnTimeElapsed(void);


/*********************************
 * Main Application
 *********************************/
void main() {
    /* Initialize the device */
    SYSTEM_Initialize();
    
    /* Set event reference */
    TMR2_SetInterruptHandler(OnTimeElapsed);  /* TIMER */
    
    /* enable Interrupts */
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    
    /* Initialize Baker */
    BAKER_Initialize();
    
    while (1)
    {
        /* Refresh console every 50ms */
        if(Baker.Updated) {
            // switch to false as the object might get updated during printing
            Baker.Updated = false;
            
            BAKER_Print(&Baker);
        }
        
        __delay_ms(50);
    }
}

/*********************************
 * Helpers
 *********************************/
/* 
 * Calculates Microwave timer segment from POT value.
 * 
 * Notes: 
 *  - Pot value is read in opposite direction
 */
uint8_t GetMicrowaveTimerSegmentFromPot(void) {
    return 1 + ((POT_MAX_VALUE - ADCC_GetSingleConversion(POT) - 1) / CONST_POT_MW_TIMER_INTERVAL_GROUP_SIZE);
}


/*********************************
 * Events
 *********************************/
/* 
 * Handles S1 button press and updates related state variable 
 */
void RefreshOperationStateButton(void) {
    bool newState = !S1_GetValue(); // read inversely

    /* detect S1 state change (low->high or high->low) */
    if (newState != StateController.OperationState) {
        StateController.OperationState = newState;

        /* handle S1 press event */
        if (newState)
            Baker.OnOperationButtonPressed(&Baker);
    }
}

/* 
 * Handles S2 button press/release and updates related state variable 
 */
void RefreshDoorStateButton(void) {
    bool newState = !S2_GetValue(); // read inversely
    
    /* detect S1 state change (low->high or high->low) */
    if (newState != StateController.DoorState) {
        StateController.DoorState = newState;

        /* handle S2 press/release event */
        Baker.OnDoorStateChanged(&Baker, newState);
    }
}

/* 
 * Handles change of POT value and updates related state variable.
 */
void RefreshTimerInterval(void) {
    uint8_t segment = GetMicrowaveTimerSegmentFromPot();

    /* detect POT value change */
    if(segment != StateController.SelectedTimerSegment) {
        StateController.SelectedTimerSegment = segment;
        
        /* handle POT value change */
        Baker.OnBakingTimeSelected(&Baker, segment * 20);
    }
}

/*********************************
 * Interrupts
 *********************************/
/* 
 * Handles timer interrupt.
 * 
 * Notes:
 *  - Bumps state counter
 *  - Polls user-controlled states (S1, S2, POT)
 *    Interrupts are avoided as they introduce new problems for
 *    state management, additional polling logic, and threading issues.
 *  - Performs main baking logic
 */
void OnTimeElapsed(void) {
    StateController.Counter += TIMER_PERIOD_MILISECONDS;

    if(StateController.Counter >= STATES_REFRESH_MILISECONDS) {
        /* fetch user controlled states */
        RefreshTimerInterval();
        RefreshDoorStateButton();
        RefreshOperationStateButton();
        
        /* refresh baking process */
        Baker.On100msElapsed(&Baker);
        
        /* reset counter */
        StateController.Counter = 0;
    }
}