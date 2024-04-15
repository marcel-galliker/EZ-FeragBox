/*
 * feragbox.h
 *
 *  Created on: Mar 15, 2024
 *      Author: WPATR
 */

#ifndef INC_FERAGBOX_H_
#define INC_FERAGBOX_H_

typedef struct {
	float voltage3V3;
	float voltage5V;
	float voltage12V;
	float voltage12VNuc;
	float voltage12VDisplay;
	float voltage24V;
	float voltagePcbRevision;
} S_VoltageMonitoring;

typedef struct {
	uint8_t nucPower;
	uint8_t displayPower;
	uint8_t printGoStatus;
	uint8_t printDoneStatus;
	uint8_t dipSwitchStatus;
	int32_t encoderSpeed;
	uint32_t encoderPosition;
	float boardTemperature;
	S_VoltageMonitoring voltages;
	uint8_t pcbRevision;
} S_FeragBoxStatus;



#endif /* INC_FERAGBOX_H_ */
