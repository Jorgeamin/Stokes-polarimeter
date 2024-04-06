#ifndef _APP_I2C_H
#define _APP_I2C_H

#include "driver/i2c/drv_i2c.h"

typedef enum{
    APP_I2C_STATE_INIT,                
    APP_I2C_STATE_WRITE,                        
    APP_I2C_STATE_READ,       
    APP_I2C_STATE_IDLE,                                                
    APP_I2C_STATE_ERROR
} APP_I2C_STATES;

typedef enum{
    APP_I2C_REQ_STATUS_DONE,            
    APP_I2C_REQ_STATUS_ERROR,
    APP_I2C_REQ_STATUS_IN_PROGRESS,    
}APP_I2C_REQ_STATUS;

typedef struct{
    APP_I2C_STATES state;  
    DRV_HANDLE i2cHandle;     
    DRV_I2C_TRANSFER_HANDLE transferHandle;     
    uint8_t i2cTxBuffer[32];     
    uint8_t i2cRxBuffer[32];                
    bool isI2CWriteReq;    
    uint32_t wrLength; 
    volatile APP_I2C_REQ_STATUS reqStatus;       
} OLED_APP_DATA;

void APP_I2C_Initialize ( void );
void APP_I2C_Tasks( void );
void APP_I2C_SetWriteRequest(uint8_t*, uint8_t);

#endif
