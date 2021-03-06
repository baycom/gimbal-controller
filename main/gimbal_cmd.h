#ifndef GIMBL_CMD_H
#define GIMBL_CMD_H

#define GIMBAL_GET_BATTERYVOLTAGE	0x0106
#define GIMBAL_GET_POWERSTATE	0x0107
#define GIMBAL_GET_PITCH	0x0122
#define GIMBAL_GET_ROLL	0x0123
#define GIMBAL_GET_PAN	0x0124
#define GIMBAL_GET_FINEPITCH	0x0125
#define GIMBAL_GET_FINEROLL	0x0126
#define GIMBAL_GET_MODE	0x0127
#define GIMBAL_GET_DEADZONEPITCH	0x015b
#define GIMBAL_GET_DEADZONEROLL	0x015c
#define GIMBAL_GET_DEADZONEPAN	0x015d
#define GIMBAL_GET_FOLLOWSPEEDPITCH	0x015e
#define GIMBAL_GET_FOLLOWSPEEDROLL	0x015f
#define GIMBAL_GET_FOLLOWSPEEDPAN	0x0160
#define GIMBAL_GET_SMOOTHPITCH	0x0161
#define GIMBAL_GET_SMOOTHROLL	0x0162
#define GIMBAL_GET_SMOOTHPAN	0x0163
#define GIMBAL_GET_CONTROLSPEEDPITCH	0x0164
#define GIMBAL_GET_CONTROLSPEEDROLL	0x0165
#define GIMBAL_GET_CONTROLSPEEDPAN	0x0166
#define GIMBAL_GET_REVERSE	0x0167
#define GIMBAL_GET_CAMERAMANUFACTURER	0x0168
#define GIMBAL_GET_MOTORINTENSITY	0x0169
#define GIMBAL_SET_PITCH	0x1001
#define GIMBAL_SET_ROLL	0x1003
#define GIMBAL_SET_PAN	0x1002
#define GIMBAL_SET_CALIBRATIONMODE	0x8109
#define GIMBAL_SET_FINEPITCH	0x8125
#define GIMBAL_SET_FINEROLL	0x8126
#define GIMBAL_SET_MODE	0x8127
#define GIMBAL_SET_DEADZONEPITCH	0x815B
#define GIMBAL_SET_DEADZONEROLL	0x815C
#define GIMBAL_SET_DEADZONEPAN	0x815D
#define GIMBAL_SET_FOLLOWSPEEDPITCH	0x815E
#define GIMBAL_SET_FOLLOWSPEEDROLL	0x815F
#define GIMBAL_SET_FOLLOWSPEEDPAN	0x8160
#define GIMBAL_SET_SMOOTHPITCH	0x8161
#define GIMBAL_SET_SMOOTHROLL	0x8162
#define GIMBAL_SET_SMOOTHPAN	0x8163
#define GIMBAL_SET_CONTROLSPEEDPITCH	0x8164
#define GIMBAL_SET_CONTROLSPEEDROLL	0x8165
#define GIMBAL_SET_CONTROLSPEEDPAN	0x8166
#define GIMBAL_SET_JOYSTICKREVERSE	0x8167
#define GIMBAL_SET_CAMERAMANUFACTURER	0x8168
#define GIMBAL_SET_MOTORINTENSITY	0x8169
#define GIMBAL_SET_POWERUP	0xC103
#define GIMBAL_SET_POWERDOWN	0xC104
#define GIMBAL_SET_FACTORYRESET	0xC105
#define GIMBAL_SET_POSITIONRESET	0xC121
#define GIMBAL_GET_KEYPRESSED	0xC720

typedef struct {
    bool        connected;
    bool        powerState;
    double      batteryLevel; 
    double      pitch;
    double      roll;
    double      pan;
    uint16_t    pitchmove;
    uint16_t    rollmove;
    uint16_t    panmove;
    uint16_t    mode;
    uint16_t    keyPressed;
    uint8_t     wheelMode;
    uint8_t     wheelStatus;
    int8_t      wheelValue;    
} gimbalstate_t;

extern gimbalstate_t gs;

int gimbalCmd3(int cmd, int val, int repeat);
int gimbalCmd2(int cmd, int val);
int gimbalCmd(int cmd);

void gimbalReceive(uint8_t *data, uint16_t len);
esp_err_t gimbalCmd_init();

#endif