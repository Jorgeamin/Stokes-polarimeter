#include "app_usb.h"
#include "common.h"

#include <string.h>

uint8_t CACHE_ALIGN cdcReadBuffer[APP_USB_READ_BUFFER_SIZE];
uint8_t CACHE_ALIGN cdcWriteBuffer[APP_USB_READ_BUFFER_SIZE];

APP_USB_DATA usbappData;

/*******************************************************
 * USB CDC Device Events - Application Event Handler
 *******************************************************/

USB_DEVICE_CDC_EVENT_RESPONSE APP_USBDeviceCDCEventHandler
(
    USB_DEVICE_CDC_INDEX index,
    USB_DEVICE_CDC_EVENT event,
    void * pData,
    uintptr_t userData
)
{
    APP_USB_DATA * usbappDataObject;
    USB_CDC_CONTROL_LINE_STATE * controlLineStateData;
    USB_DEVICE_CDC_EVENT_DATA_READ_COMPLETE * eventDataRead;
    
    usbappDataObject = (APP_USB_DATA *)userData;

    switch(event)
    {
        case USB_DEVICE_CDC_EVENT_GET_LINE_CODING:

            /* This means the host wants to know the current line
             * coding. This is a control transfer request. Use the
             * USB_DEVICE_ControlSend() function to send the data to
             * host.  */

            USB_DEVICE_ControlSend(usbappDataObject->deviceHandle,
                    &usbappDataObject->getLineCodingData, sizeof(USB_CDC_LINE_CODING));

            break;

        case USB_DEVICE_CDC_EVENT_SET_LINE_CODING:

            /* This means the host wants to set the line coding.
             * This is a control transfer request. Use the
             * USB_DEVICE_ControlReceive() function to receive the
             * data from the host */

            USB_DEVICE_ControlReceive(usbappDataObject->deviceHandle,
                    &usbappDataObject->setLineCodingData, sizeof(USB_CDC_LINE_CODING));

            break;

        case USB_DEVICE_CDC_EVENT_SET_CONTROL_LINE_STATE:

            /* This means the host is setting the control line state.
             * Read the control line state. We will accept this request
             * for now. */

            controlLineStateData = (USB_CDC_CONTROL_LINE_STATE *)pData;
            usbappDataObject->controlLineStateData.dtr = controlLineStateData->dtr;
            usbappDataObject->controlLineStateData.carrier = controlLineStateData->carrier;

            USB_DEVICE_ControlStatus(usbappDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);

            break;

        case USB_DEVICE_CDC_EVENT_SEND_BREAK:

            /* This means that the host is requesting that a break of the
             * specified duration be sent. Read the break duration */

            usbappDataObject->breakData = ((USB_DEVICE_CDC_EVENT_DATA_SEND_BREAK *)pData)->breakDuration;
            
            /* Complete the control transfer by sending a ZLP  */
            USB_DEVICE_ControlStatus(usbappDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);
            
            break;

        case USB_DEVICE_CDC_EVENT_READ_COMPLETE:

            /* This means that the host has sent some data*/
            eventDataRead = (USB_DEVICE_CDC_EVENT_DATA_READ_COMPLETE *)pData;
            usbappDataObject->isReadComplete = true;
            usbappDataObject->numBytesRead = eventDataRead->length; 
            break;

        case USB_DEVICE_CDC_EVENT_CONTROL_TRANSFER_DATA_RECEIVED:

            /* The data stage of the last control transfer is
             * complete. For now we accept all the data */

            USB_DEVICE_ControlStatus(usbappDataObject->deviceHandle, USB_DEVICE_CONTROL_STATUS_OK);
            break;

        case USB_DEVICE_CDC_EVENT_CONTROL_TRANSFER_DATA_SENT:

            /* This means the GET LINE CODING function data is valid. We don't
             * do much with this data in this demo. */
            break;

        case USB_DEVICE_CDC_EVENT_WRITE_COMPLETE:

            /* This means that the data write got completed. We can schedule
             * the next read. */

            usbappDataObject->isWriteComplete = true;
            break;

        default:
            break;
    }

    return USB_DEVICE_CDC_EVENT_RESPONSE_NONE;
}

/***********************************************
 * Application USB Device Layer Event Handler.
 ***********************************************/
void APP_USBDeviceEventHandler 
(
    USB_DEVICE_EVENT event, 
    void * eventData, 
    uintptr_t context 
)
{
    USB_DEVICE_EVENT_DATA_CONFIGURED *configuredEventData;

    switch(event)
    {
        case USB_DEVICE_EVENT_SOF:

            usbappData.sofEventHasOccurred = true;
            
            break;

        case USB_DEVICE_EVENT_RESET:

            /* Update LED to show reset state */
            //LED_Clear();//LED_Off();
            USBconnected(false);

            usbappData.isConfigured = false;

            break;

        case USB_DEVICE_EVENT_CONFIGURED:

            /* Check the configuration. We only support configuration 1 */
            configuredEventData = (USB_DEVICE_EVENT_DATA_CONFIGURED*)eventData;
            
            if ( configuredEventData->configurationValue == 1)
            {
                /* Update LED to show configured state */
                //LED_Set();
                USBconnected(true);
                
                /* Register the CDC Device application event handler here.
                 * Note how the appData object pointer is passed as the
                 * user data */

                USB_DEVICE_CDC_EventHandlerSet(USB_DEVICE_CDC_INDEX_0, APP_USBDeviceCDCEventHandler, (uintptr_t)&usbappData);

                /* Mark that the device is now configured */
                usbappData.isConfigured = true;
            }
            
            break;

        case USB_DEVICE_EVENT_POWER_DETECTED:

            /* VBUS was detected. We can attach the device */
            USB_DEVICE_Attach(usbappData.deviceHandle);
            
            break;

        case USB_DEVICE_EVENT_POWER_REMOVED:

            /* VBUS is not available any more. Detach the device. */
            USB_DEVICE_Detach(usbappData.deviceHandle);
            
            //LED_Clear();
            USBconnected(false);
            
            break;

        case USB_DEVICE_EVENT_SUSPENDED:

            //LED_Clear();
            USBconnected(false);
            
            break;

        case USB_DEVICE_EVENT_RESUMED:
        case USB_DEVICE_EVENT_ERROR:
        default:
            
            break;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

/*****************************************************
 * This function is called in every step of the
 * application state machine.
 *****************************************************/

bool APP_USB_StateReset(void)
{
    /* This function returns true if the device
     * was reset  */

    bool retVal;

    if(usbappData.isConfigured == false)
    {
        usbappData.state = APP_USB_STATE_WAIT_FOR_CONFIGURATION;
        usbappData.readTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
        usbappData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
        usbappData.isReadComplete = true;
        usbappData.isWriteComplete = true;
        retVal = true;
    }
    else
    {
        retVal = false;
    }

    return(retVal);
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize(void)

  Remarks:
    See prototype in app.h.
 */

void APP_USB_Initialize(void)
{
    /* Place the App state machine in its initial state. */
    usbappData.state = APP_USB_STATE_INIT;
    
    /* Device Layer Handle  */
    usbappData.deviceHandle = USB_DEVICE_HANDLE_INVALID ;

    /* Device configured status */
    usbappData.isConfigured = false;

    /* Initial get line coding state */
    usbappData.getLineCodingData.dwDTERate = 115200;
    usbappData.getLineCodingData.bParityType = 0;
    usbappData.getLineCodingData.bParityType = 0;
    usbappData.getLineCodingData.bDataBits = 8;

    /* Read Transfer Handle */
    usbappData.readTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

    /* Write Transfer Handle */
    usbappData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

    /* Initialize the read complete flag */
    usbappData.isReadComplete = true;

    /*Initialize the write complete flag*/
    usbappData.isWriteComplete = true;

    /* Reset other flags */
    usbappData.sofEventHasOccurred = false;

    /* Set up the read buffer */
    usbappData.cdcReadBuffer = &cdcReadBuffer[0];

    /* Set up the read buffer */
    usbappData.cdcWriteBuffer = &cdcWriteBuffer[0];       
}


/******************************************************************************
  Function:
    void APP_Tasks(void)

  Remarks:
    See prototype in app.h.
 */

void APP_USB_Tasks(void)
{
    //char text[20];
    char* token;
    uint8_t i=0, error=0;
    uint16_t adc_result[6] = {0,0,0,0,0,0};
    uint8_t adc_channel = 0;

    switch(usbappData.state)
    {
        case APP_USB_STATE_INIT:

            /* Open the device layer */
            usbappData.deviceHandle = USB_DEVICE_Open( USB_DEVICE_INDEX_0, DRV_IO_INTENT_READWRITE );

            if(usbappData.deviceHandle != USB_DEVICE_HANDLE_INVALID)
            {
                /* Register a callback with device layer to get event notification (for end point 0) */
                USB_DEVICE_EventHandlerSet(usbappData.deviceHandle, APP_USBDeviceEventHandler, 0);

                usbappData.state = APP_USB_STATE_WAIT_FOR_CONFIGURATION;
            }
            else
            {
                /* The Device Layer is not ready to be opened. We should try
                 * again later. */
            }

            break;

        case APP_USB_STATE_WAIT_FOR_CONFIGURATION:

            /* Check if the device was configured */
            if(usbappData.isConfigured)
            {
                /* If the device is configured then lets start reading */
                usbappData.state = APP_USB_STATE_SCHEDULE_READ;
            }
            
            break;

        case APP_USB_STATE_SCHEDULE_READ:

            if(APP_USB_StateReset())
            {
                break;
            }

            /* If a read is complete, then schedule a read
             * else wait for the current read to complete */

            usbappData.state = APP_USB_STATE_WAIT_FOR_READ_COMPLETE;
            if(usbappData.isReadComplete == true)
            {
                usbappData.isReadComplete = false;
                usbappData.readTransferHandle =  USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;

                USB_DEVICE_CDC_Read (USB_DEVICE_CDC_INDEX_0,
                        &usbappData.readTransferHandle, usbappData.cdcReadBuffer,
                        APP_USB_READ_BUFFER_SIZE);
                
                if(usbappData.readTransferHandle == USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID)
                {
                    usbappData.state = APP_USB_STATE_ERROR;
                    break;
                }
            }

            break;

        case APP_USB_STATE_WAIT_FOR_READ_COMPLETE:

            if(APP_USB_StateReset()){
                break;
            }
            
            if(usbappData.isReadComplete){   
                error = 0;
                memset((char*)usbappData.cdcWriteBuffer, 0 , 512);
                i = (int)strchr((char*)usbappData.cdcReadBuffer,'>');
                
                if((usbappData.cdcReadBuffer[0] == '<') & (usbappData.cdcReadBuffer[i] == '>')){
                    memmove(&usbappData.cdcReadBuffer[0], &usbappData.cdcReadBuffer[1], i-1);
                    usbappData.cdcReadBuffer[i-1] = '\0';
                    
                    token = strtok((char*)usbappData.cdcReadBuffer, ",");
                    
                    if (strcmp(token, "r") == 0){
                        token = strtok(0, ",");
                        if(token == NULL){
                            for(adc_channel=0; adc_channel<6; adc_channel++){
                                switch(adc_channel){
                                    case 0: ADC_ChannelSelect(ADC_POSINPUT_AIN5 , ADC_NEGINPUT_GND); break;
                                    case 1: ADC_ChannelSelect(ADC_POSINPUT_AIN4 , ADC_NEGINPUT_GND); break;
                                    case 2: ADC_ChannelSelect(ADC_POSINPUT_AIN0 , ADC_NEGINPUT_GND); break;
                                    case 3: ADC_ChannelSelect(ADC_POSINPUT_AIN1 , ADC_NEGINPUT_GND); break;
                                    case 4: ADC_ChannelSelect(ADC_POSINPUT_AIN6 , ADC_NEGINPUT_GND); break;
                                    case 5: ADC_ChannelSelect(ADC_POSINPUT_AIN7 , ADC_NEGINPUT_GND); break;
                                }
                                ADC_ConversionStart( );
                                while(!ADC_ConversionStatusGet());
                                adc_result[adc_channel] = ADC_ConversionResultGet();
                            }
                            
                            usbappData.numBytesWrite = sprintf((char*)usbappData.cdcWriteBuffer, "[%04d,%04d,%04d,%04d,%04d,%04d]\n", adc_result[0],adc_result[1],adc_result[2],adc_result[3],adc_result[4],adc_result[5]);    
                            error = 0xff;
                        }
                        else error = 1;
                    }
                    else if (strcmp(token, "i") == 0){
                        token = strtok(0, ",");
                        if(token == NULL){
                            usbappData.numBytesWrite = sprintf((char*)usbappData.cdcWriteBuffer, "Polarization_analyzer,v2.0\n");    
                            error = 0xff;
                        }
                        else error = 1;
                    }
                    else error = 1;//default
                }     
                else error = 1;
                
                memset((char*)usbappData.cdcReadBuffer, 0 , 512);
                
                if (error == 0)
                    usbappData.numBytesWrite = sprintf((char*)usbappData.cdcWriteBuffer, "Done\n");
                else if (error < 0xff)
                    usbappData.numBytesWrite = sprintf((char*)usbappData.cdcWriteBuffer, "Error code: %d\n",error);
                   
                usbappData.state = APP_USB_STATE_SCHEDULE_WRITE;
            }
            break;


        case APP_USB_STATE_SCHEDULE_WRITE:

            if(APP_USB_StateReset())
            {
                break;
            }

            /* Setup the write */

            usbappData.writeTransferHandle = USB_DEVICE_CDC_TRANSFER_HANDLE_INVALID;
            usbappData.isWriteComplete = false;
            usbappData.state = APP_USB_STATE_WAIT_FOR_WRITE_COMPLETE;
                   
            
            USB_DEVICE_CDC_Write(USB_DEVICE_CDC_INDEX_0,
                    &usbappData.writeTransferHandle,
                    usbappData.cdcWriteBuffer, usbappData.numBytesWrite,
                    USB_DEVICE_CDC_TRANSFER_FLAGS_DATA_COMPLETE);
            
            break;

        case APP_USB_STATE_WAIT_FOR_WRITE_COMPLETE:

            if(APP_USB_StateReset())
            {
                break;
            }

            /* Check if a character was sent. The isWriteComplete
             * flag gets updated in the CDC event handler */

            if(usbappData.isWriteComplete == true)
            {
                usbappData.state = APP_USB_STATE_SCHEDULE_READ;
            }

            break;

        case APP_USB_STATE_ERROR:
        default:
            
            break;
    }
}

/*******************************************************************************
 End of File
 */

