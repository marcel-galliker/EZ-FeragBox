/*
 * encoder_engine.c
 *
 *  Created on: Mar 25, 2024
 *      Author: WPATR
 */

#include <enc.h>

extern TIM_HandleTypeDef htim2; // Assuming htim2 is declared elsewhere

static int32_t _Speed = 0;

//--- enc_init ----------------------------------
void enc_init(void)
{
}

//--- enc_set_speed ---------------------------
void enc_set_speed(int32_t speed)
{
	if (abs(speed-_Speed)<100) return;

	// If the encoder is running, stop it before reconfiguring
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);

    _Speed = speed;

    // Adjust PWM parameters based on the new configuration
    uint32_t timer_clock_frequency = HAL_RCC_GetPCLK1Freq(); // Adjust based on your clock tree settings
    uint32_t prescaler = 1; // Fixed prescaler value
    // Calculate the period for the desired speed
    if (prescaler * speed == 0)return;

    int reverse=0;
    if (speed<0)
    {
    	reverse=1;
    	speed=-speed;
    }

    uint32_t period = (timer_clock_frequency / (prescaler * speed)) - 1;

    // Update Timer configuration
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = prescaler - 1; // Prescaler is 0-based, for prescaler = 1, this will be 0
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = period;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
    sConfigOC.OCIdleState = TIM_OUTPUTSTATE_ENABLE;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;

    // Apply direction
    if (reverse)
    { // Reverse
        sConfigOC.Pulse = (period * 3) / 4; // CH1 at 25%
        HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
        sConfigOC.Pulse = (period * 1) / 4; // CH2 at 75%, ie half cycle later
        HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
    }
    else
    { // Forward
        sConfigOC.Pulse = (period * 1) / 4; // CH1 at 25%
        HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
        sConfigOC.Pulse = (period * 3) / 4; // CH2 at 75%, ie half cycle later
        HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
    }

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

//--- enc_start ---------------------------
void enc_start(void)
{
    // Apply the last configured settings and start PWM
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

//--- enc_stop ---------------------------------
void enc_stop(void) {
    // Stop the PWM and ensure both outputs are low
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}

//--- enc_get_speed ------------------------
int32_t enc_get_speed(void)
{
	return _Speed;
}

