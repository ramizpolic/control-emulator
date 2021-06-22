# Microwave controller

Implements microwave emulator which relies on polling, interrupts, and serial communication. 
Written by following OOP approach in C, developed in **MPLAB X v5.45**, and works for **PIC16F18857** microcontrollers.


### References
  * [PIC16F18857 Datasheet](https://www.microchip.com/wwwproducts/en/PIC16F18857)

## Requirements
#### Board
X2C Development board was used for the realization of the task. The board already contains all the elements, but the needed parts include:
* Timer
* Potmeter
* 2 Buttons
* Interrupts 
* USART Interface

#### Softwares and libraries
* [MPLAB X v5.45](https://www.microchip.com/en-us/development-tools-tools-and-software/mplab-x-ide/)
* [MCC - MPLAB Code Configurator](https://www.microchip.com/en-us/development-tools-tools-and-software/embedded-software-center/mplab-code-configurator)


## Configuration
Make sure to update `mcc_generated_files` based on your board specifications and availability using MPLAB MCC.


## Build
#### On Windows
* Clone repository
* Open MPLAB X
* Load project
* Open and configure **realterm**
* Start communication

#### On Linux
```bash
# install required packages
$ sudo apt-get install gcc gcc-multilib g++-multilib

# build and flash
$ git clone https://github.com/fhivemind/microwave-controller.git
$ make
$ make flash
```
---

## Description
> **Timer:**
The backing time can be set up with potentiometer in 20-second intervals. The maximum value of timer is 2 minute. Use the UART terminal to display the preset time in mm:ss format.

> **Buttons:**
Cooking can be start and stop with S1 button (START/STOP button). The S2 button simulates the state of the door. If the button is pressed the microwave oven door is opened. If the button is released the microwave oven door is closed.

> **Status LEDs:**
The LED1 symbolizes state of magnetron. If the LED is turned on the baking is in progress. If the LED is turned off then the magnetron is not active. The LED2 represents the internal lighting. If the LED is turned on the oven interior light is active.

> **Baking process:**
The cooking process can be start with pressing the START button. The cooking process starts if the door is closed and the START button is pressed. During the cooking process the time indicator LED3 is blinking with 100 ms period and the magnetron is turned on. Indicate the remaining time on the serial port.
The cooking process can be paused with opening of the door. If the door is opened the cooking process have to be stopped immediately, but the LED3 is blinking continuously indicatTimeing that the cooking program is in progress. To continue the cooking process the user have to close the door and have to press the START button.
Pressing the STOP button stops the cooking process. In the case when the cooking time has elapsed the cooking process is halted also.

> **Lighting:**
The oven interior light is turned on during cooking process or the door is open.

## Operation
*  Base frequency at 4 MHz
*  Baud rate at 9600
*  **Interrupt  (10ms TM2)**  - handles states management & system configuration
*  **Main logic (50ms)**      - UART transmission of formatted and color-coded internal states
 
*The response of the controller on UART port will be the representation of internal states of the controller. The line will be held if the output is wide enough.*

| State | Representation | Steps |
| --- | --- | --- |
| **Waiting** | `Status  00:00  [100%] \| Door [Opened] \| Selected [00:40] \| Operation           Off` | After booting or completing the cycle |
| **In progress** | `Status  00:24  [40 %] \| Door [Closed] \| Selected [00:40] \| Operation   In Progress` | Holding Door button (S2) and starting the process by clicking Start/Stop button (S1) |
| **Paused** | `Status  00:17  [57 %] \| Door [Opened] \| Selected [00:40] \| Operation        Paused` | Releasing Door button (S2) while In Progress state |
| **Successful** | `Status  00:00  [100%] \| Door [Closed] \| Selected [00:40] \| Operation     Succeeded` | Waiting for In Progress to finish by holding Door button (S2) |
| **Canceled** | `Status  00:00  [100%] \| Door [Closed] \| Selected [00:40] \| Operation      Canceled` | Pressing Start/Stop button (S1) while in In Progress or Paused state |

To enter `In Progress` from `Paused`, you need to simulate closed doors by holding Door button (S2) and resuming the operation by pressing the Start/Stop button (S1).


***
Author: [Ramiz Polic](https://github.com/fhivemind)
