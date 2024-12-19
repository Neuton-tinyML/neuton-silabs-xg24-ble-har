/* 2023-06-09T11:22:25Z */

/* ----------------------------------------------------------------------
Copyright (c) 2022-2025 Neuton.AI, Inc.
*
The source code and its binary form are being made available on the following terms:
Redistribution, use, and modification are permitted provided that the following
conditions are met:
*
1. Redistributions of source code and/or its binary form must retain the above copyright notice,
* this list of conditions (and the disclaimer) either in the body of the source code or in
* the documentation and/or other materials provided with the distribution of the binary form, as
applicable.
*
2. The name of the copyright holder may not be used to endorse or promote products derived from this
* source code or its binary form without specific prior written permission of Neuton.AI, Inc.
*
3. Disclaimer. THIS SOURCE CODE AND ITS BINARY FORM ARE PROVIDED BY THE COPYRIGHT HOLDER "AS IS".
* THE COPYRIGHT HOLDER HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE HELD LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS
* OF THIRD PARTIES; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOURCE CODE OR ITS BINARY FORM, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------- */

/**
 *
 * @defgroup neuton_har_app Neuton BLE Human Activity Recognition Demo Application
 * @{
 *
 * @details     This demo application allows the user to connect Silabs xG24 devkit to a PC and 
 *              using Neuton TinyML library recognize and send via BLE the following human activities:
 * 
 *                  - Washing hands
 *                  - Hand waving
 *                  - Brushing Hair
 *                  - Clapping
 *                  - Wiping a table
 *                  - Using screwdriver
 *                  - Idle (no activity)
 *                  - Unknown
 *              More information about gestures you can find in the README.md file
 * 
 *
 */
#ifndef NEUTON_HAR_APPLICATION_H_
#define NEUTON_HAR_APPLICATION_H_

#include <stdint.h>

/**
 * @brief Initialize Human Activity Recognition Demo Application
 * 
 */
void neuton_har_app_init(void);

/**
 * @brief Main process of Human Activity Recognition Demo Application
 * 
 */
void neuton_har_app_dowork(void);


#endif /* NEUTON_HAR_APPLICATION_H_ */

/**
 * @}
 */
