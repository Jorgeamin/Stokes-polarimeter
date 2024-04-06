#ifndef _APP_H
#define _APP_H

#include "definitions.h"

#define APP_USB_READ_BUFFER_SIZE                                512

typedef enum{
    APP_USB_STATE_INIT=0,
    APP_USB_STATE_WAIT_FOR_CONFIGURATION,
    APP_USB_STATE_SCHEDULE_READ,
    APP_USB_STATE_WAIT_FOR_READ_COMPLETE,
    APP_USB_STATE_SCHEDULE_WRITE,
    APP_USB_STATE_WAIT_FOR_WRITE_COMPLETE,
    APP_USB_STATE_ERROR

} APP_USB_STATES;

typedef struct{
    USB_DEVICE_HANDLE deviceHandle;
    APP_USB_STATES state;
    USB_CDC_LINE_CODING setLineCodingData;
    bool isConfigured;
    USB_CDC_LINE_CODING getLineCodingData;
    USB_CDC_CONTROL_LINE_STATE controlLineStateData;
    USB_DEVICE_CDC_TRANSFER_HANDLE readTransferHandle;
    USB_DEVICE_CDC_TRANSFER_HANDLE writeTransferHandle;
    bool isReadComplete;
    bool isWriteComplete;
    bool sofEventHasOccurred;
    uint16_t breakData;
    uint8_t * cdcReadBuffer;
    uint8_t * cdcWriteBuffer;
    uint32_t numBytesRead;
    uint32_t numBytesWrite; 
} APP_USB_DATA;

void APP_USB_Initialize ( void );
void APP_USB_Tasks ( void );

#endif