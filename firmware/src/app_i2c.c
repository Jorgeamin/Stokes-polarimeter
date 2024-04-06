#include "app_i2c.h"

#define APP_I2C_SLAVE_ADDR                   0x003C

OLED_APP_DATA app_i2cData;

void APP_I2C_EventHandler(
    DRV_I2C_TRANSFER_EVENT event,
    DRV_I2C_TRANSFER_HANDLE transferHandle,
    uintptr_t context
)
{
    switch(event){
        case DRV_I2C_TRANSFER_EVENT_COMPLETE:
            /* I2C read complete. */
            app_i2cData.reqStatus = APP_I2C_REQ_STATUS_DONE;
            break;
        case DRV_I2C_TRANSFER_EVENT_ERROR:
            app_i2cData.reqStatus = APP_I2C_REQ_STATUS_ERROR;
            break;
        default:
            break;
    }
}

static void _APP_I2C_Write(uint32_t wrLength){
    uint8_t dummyData = 0, error_count = 0;

    app_i2cData.reqStatus = APP_I2C_REQ_STATUS_IN_PROGRESS;

    DRV_I2C_WriteTransferAdd(app_i2cData.i2cHandle, APP_I2C_SLAVE_ADDR,
        (void *)app_i2cData.i2cTxBuffer, wrLength, &app_i2cData.transferHandle);

    if (app_i2cData.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID){
        /* Handle error condition */
    }

    while (app_i2cData.reqStatus == APP_I2C_REQ_STATUS_IN_PROGRESS);

    do{
        app_i2cData.reqStatus = APP_I2C_REQ_STATUS_IN_PROGRESS;

        DRV_I2C_WriteTransferAdd(app_i2cData.i2cHandle, APP_I2C_SLAVE_ADDR,
                (void *)&dummyData, 1, &app_i2cData.transferHandle);

        if (app_i2cData.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID){
            /* Handle error condition */
        }

        while (app_i2cData.reqStatus == APP_I2C_REQ_STATUS_IN_PROGRESS);
        
        if ( error_count > 3 ){
            app_i2cData.state = APP_I2C_STATE_ERROR;
            break;
        }
        error_count++;
        
    }while (app_i2cData.reqStatus == APP_I2C_REQ_STATUS_ERROR);

 }

void APP_I2C_SetWriteRequest(uint8_t* wrData, uint8_t wrLength){ 
    uint8_t n;
    for (n=0; n<wrLength; n++)  {app_i2cData.i2cTxBuffer[n] = wrData[n];}
    app_i2cData.wrLength = wrLength;

    if (app_i2cData.state != APP_I2C_STATE_ERROR){
        //app_i2cData.isOLEDWriteReq = true;
        _APP_I2C_Write(wrLength);
    }
}
 
void APP_I2C_Initialize ( void ){
    app_i2cData.state = APP_I2C_STATE_INIT;
}

void APP_I2C_Tasks ( void ){
    switch ( app_i2cData.state ){
        case APP_I2C_STATE_INIT:
            app_i2cData.i2cHandle = DRV_I2C_Open( DRV_I2C_INDEX_0, DRV_IO_INTENT_READWRITE );
            if (app_i2cData.i2cHandle != DRV_HANDLE_INVALID){
                DRV_I2C_TransferEventHandlerSet(app_i2cData.i2cHandle,  APP_I2C_EventHandler, 0);
                app_i2cData.state =  APP_I2C_STATE_IDLE;   
            }
            else{
                /* Handle error condition */
            }
            break;

        case APP_I2C_STATE_WRITE:
            _APP_I2C_Write(app_i2cData.wrLength);
            app_i2cData.state = APP_I2C_STATE_IDLE;
            break;

        case APP_I2C_STATE_READ:
            //app_i2cData.i2cTxBuffer[0] = OLED_APP_LOG_MEMORY_ADDR;
            app_i2cData.reqStatus = APP_I2C_REQ_STATUS_IN_PROGRESS;

            DRV_I2C_WriteReadTransferAdd(app_i2cData.i2cHandle, APP_I2C_SLAVE_ADDR,
                    app_i2cData.i2cTxBuffer, 1, app_i2cData.i2cRxBuffer, 5 , &app_i2cData.transferHandle);

			if (app_i2cData.transferHandle == DRV_I2C_TRANSFER_HANDLE_INVALID){
                /* Handle error condition */
            }

            while (app_i2cData.reqStatus == APP_I2C_REQ_STATUS_IN_PROGRESS);
            app_i2cData.state = APP_I2C_STATE_IDLE;
            break;

        case APP_I2C_STATE_IDLE:
            if (app_i2cData.isI2CWriteReq == true){
                app_i2cData.isI2CWriteReq = false;
                app_i2cData.state = APP_I2C_STATE_WRITE;
            }
            break;

        case APP_I2C_STATE_ERROR:
            /* Handle error conditions here */
            break;

        default:
            break;

    }
}