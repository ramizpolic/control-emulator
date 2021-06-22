/*
 * File:   baking.c
 * Author: Ramiz Polic
 * 
 * Notes: 
 *  - OOP driven approach
 *  - Defines and Initializes main baking logic
 *  - Max stack depth should not reach 4
 *  - Optimized
 */

#include "baking.h"
#include "mcc_generated_files/mcc.h"


/*********************************
 * Timer helper
 *********************************/
Timer TIMER_GetTime(uint8_t time) {
    Timer res = {
        .mm = time / 60,
        .ss = time % 60
    };
    
    return res;
}

/****************************************************
 * Baking getters/helpers
 * 
 * Notes: 
 * - Inline functions that show internal states
 *   or configure non-object related state 
 *   (such as LEDS)
 * 
 ****************************************************/
inline bool baking_IsDoorOpen(BakingInterface *self) {
    return !self->Door;
}

inline bool baking_IsDoorClosed(BakingInterface *self) {
    return self->Door;
}

inline void baking_SetMagnetron(bool value) {
    LED1_PORT = value;
}

inline void baking_SetInternalLight(bool value) {
    LED2_PORT = value;
}

inline void baking_SetBakingLight(bool value) {
    LED3_PORT = value;
}

inline void baking_ToggleBakingLight(void) {
    LED3_Toggle();
}

inline bool baking_IsFinished(BakingInterface *self) {
    return self->Time == 0;
}

inline void baking_Reset(BakingInterface *self) {
    self->Time = 0;
    
    self->SetMagnetron(false);
    self->SetInternalLight(false);
    self->SetBakingLight(false);
}

/****************************************************
 * Baking print functions
 * 
 * Notes: 
 * - Print internal states to console
 * - This function should not be called from Interrupts
 *   as its CPU-heavy (due to `printf` blocking invocation).
 *   Blocks main thread for ~= 300ms
 * - ANSI color-formatted output
 * 
 ****************************************************/
void BAKER_Print(BakingInterface *self) {
    /* get time data */
    Timer selected = TIMER_GetTime(self->SelectedTime);
    Timer remaining = TIMER_GetTime(self->Time);
    
    /* Get status percentage (+ 0.001 to avoid Division by Zero) */
    uint8_t percent = 100 - (self->Time * (100.0 / (self->RequestedTime + 0.001)));

    /* format & print */
    printf( "\r"
            "\e[4;37mStatus\e[0m  \e[0;35m%02u:%02u\e[0m  [%-03u%%]"
            " | \e[4;37mDoor\e[0m [%06s\e[0m]"
            " | \e[4;37mSelected\e[0m [\e[1;37m%02u:%02u\e[0m]"
            " | \e[4;37mOperation\e[0m %20s\e[0m"
            "\0",
            remaining.mm, remaining.ss,
            percent,
            DoorStates[self->Door],
            selected.mm, selected.ss,
            self->Note
    );
}

/****************************************************
 * Baking state manager
 * 
 * Notes: 
 * - Handles internal state change and invokes pre- and
 *   post- logic required for proper hardware states.
 * 
 ****************************************************/
void baking_SetState(BakingInterface *self, BakingState newState) {
    /* clear note */
    sprintf(self->Note, BakingStates[newState]);

    /* handle change */
    switch (newState) {
        case Off:
            self->Reset(self);
            break;
        
        case InProgress:
            if(self->Time == 0) {
                self->Time = self->SelectedTime;
                self->RequestedTime = self->SelectedTime;
            }

            self->SetMagnetron(true);
            self->SetInternalLight(true);
            break;

        case Paused:
            self->SetMagnetron(false);
            break;
            
        case Succeeded:
            self->Reset(self);
            newState = Off; /* fix state */
            break;

        case Canceled:
            self->Reset(self);
            newState = Off; /* fix state */
            break;
            
        default:
            break;
    }
 
    /* bump */
    self->State = newState;

    self->Updated = true;
}

/****************************************************
 * Events
 * 
 * Notes: 
 * - Handles user events (timer selection/button press)
 * 
 ****************************************************/
/* Handles baking time change event */
void baking_OnBakingTimeSelected(BakingInterface* self, uint8_t time) {
    self->SelectedTime = time;

    self->Updated = true;
}

/* Handles door state change event.
 * Notes:
 *  - Sets 'Paused' when doors are opened for 'InProgress' state 
 *  - Sets Internal Lightning when doors are open or 'InProgress' state
 */
void baking_OnDoorStateChanged(BakingInterface *self, bool doorState) {
    /* update door state */
    self->Door = doorState;    
    
    /* handle state change */
    if (self->State == InProgress && self->IsDoorOpen(self)) 
        self->SetState(self, Paused);
    
    /* handle internal light */
    self->SetInternalLight(self->IsDoorOpen(self) || self->State == InProgress);

    self->Updated = true;
}

/* Handles START/STOP button press event.
 * Notes:
 *  1. When doors are closed:
 *     - 'Off' or 'Paused' state changes to 'InProgress'
 *     - 'InProgress' state changes to 'Canceled'
 *  2. When doors are opened:
 *     - Non-'Off' state changes to 'Canceled'
 */
void baking_OnOperationButtonPressed(BakingInterface *self) {
    /* handle state change */
    if(self->IsDoorClosed(self)) {
        if (self->State == Off || self->State == Paused)
            self->SetState(self, InProgress);
        else if(self->State == InProgress)
            self->SetState(self, Canceled);  
    }
    else if (self->State != Off) {
        self->SetState(self, Canceled);
    }
    
    self->Updated = true;
}

/****************************************************
 * Baking counters
 * 
 * Notes: 
 * - Triggered externally, handles 100ms passed event.
 *   Instead of 1s event, 100ms is used to ensure
 *   that pause operation is 10x more precise.
 * - Reusable logic for n * 100ms operations.
 * 
 ****************************************************/
void baking_On100msElapsed(BakingInterface *self) {
    /* handle states */
    switch (self->State) {
        case Paused:
            self->ToggleBakingLight();  /* toggle light */
            break;

        case InProgress:
            self->ToggleBakingLight();  /* toggle light */

            /* bump counter */
            self->Counter100ms++;

            /* 1s passed? */
            if(self->Counter100ms >= 10) {
                /* bump time */
                if(!self->IsFinished(self))
                    self->Time--;
                else
                    self->SetState(self, Succeeded);

                /* reset counter */
                self->Counter100ms = 0;

                self->Updated = true;
            }
            break;
            
        default:
            break;
    }
}

/*********************************
 * Controllers
 *********************************/
volatile BakingInterface Baker;

/*
 * Initializes Baker object.
 * 
 * Notes:
 *  - Must be called during main program initialization.
 *  - Must be called after required hardware initializers.
 */
void BAKER_Initialize(void) {
    /* init */
    BakingInterface manager = {
        .State = Off,
        .Door = false,
        .Updated = true,
        .Counter100ms = 0,
        .Time = 0,
        .RequestedTime = 0,
        .SelectedTime = 0,

        .IsFinished = &baking_IsFinished,
        .IsDoorOpen = &baking_IsDoorOpen,
        .IsDoorClosed = &baking_IsDoorClosed,

        .Reset = &baking_Reset,
        .ToggleBakingLight = &baking_ToggleBakingLight,
        .SetBakingLight = &baking_SetBakingLight,
        .SetMagnetron = &baking_SetMagnetron,
        .SetInternalLight = &baking_SetInternalLight,
        .SetState = &baking_SetState,

        .On100msElapsed = &baking_On100msElapsed,
        .OnBakingTimeSelected = &baking_OnBakingTimeSelected,
        .OnDoorStateChanged = &baking_OnDoorStateChanged,
        .OnOperationButtonPressed = &baking_OnOperationButtonPressed,
        
        .Print = &BAKER_Print,
    };
    
    /* refresh note */
    sprintf(manager.Note, BakingStates[manager.State]);
    
    /* assign */
    Baker = manager;
}