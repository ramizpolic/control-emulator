/* 
 * File:   baking.h
 * Author: Ramiz Polic
 *
 * Notes: 
 *  - OOP driven approach
 *  - Defines main baking interface
 *  - C++ supported
 *  - Memory optimised
 */

#ifndef BAKING_H
#define	BAKING_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
    
/*********************************/
/* State Types */
typedef enum BakingState BakingState;
typedef struct IBaker BakingInterface;
typedef struct ITimer Timer;

/* State Values */
enum BakingState {Off, InProgress, Paused, Succeeded, Canceled};

/* Printable State Values */
static const char *BakingStates[5] = { 
    "\e[0;37mOff", 
    "\e[0;36mIn Progress", 
    "\e[0;33mPaused", 
    "\e[0;32mSucceeded", 
    "\e[0;31mCanceled" 
};
static const char *DoorStates[2] = { 
    "\e[0;31mOpened", 
    "\e[0;32mClosed" 
};

/* Controller */
extern volatile BakingInterface Baker;

/* Public functions & Initializers */
extern void BAKER_Initialize(void);
extern void BAKER_Print(BakingInterface*);
extern Timer TIMER_GetTime(uint8_t);

/* Baking Interface */
struct IBaker {
    BakingState State;
    bool Door;
    bool Updated;
    uint8_t Counter100ms;
    uint8_t Time;
    uint8_t RequestedTime;
    uint8_t SelectedTime;
    char Note[31];

    bool (*IsFinished)(BakingInterface*);
    bool (*IsDoorOpen)(BakingInterface*);
    bool (*IsDoorClosed)(BakingInterface*);
    
    void (*ToggleBakingLight)(void);
    void (*SetBakingLight)(bool);
    void (*SetMagnetron)(bool);
    void (*SetInternalLight)(bool);
    void (*SetState)(BakingInterface*, BakingState);
    void (*Reset)(BakingInterface*);

    void (*On100msElapsed)(BakingInterface*); 
    void (*OnBakingTimeSelected)(BakingInterface*, uint8_t);
    void (*OnDoorStateChanged)(BakingInterface*, bool);
    void (*OnOperationButtonPressed)(BakingInterface*);
    
    void (*Print)(BakingInterface*);
};

/* Timer Interface */
struct ITimer { uint8_t mm; uint8_t ss; };

#ifdef	__cplusplus
}
#endif

#endif	/* BAKING_H */

