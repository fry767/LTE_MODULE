#include "rndis.h"

#define USB_REQ_CDC_SEND_ENCAPSULATED   0x00
#define USB_REQ_CDC_GET_ENCAPSULATED    0x01

USBH_StatusTypeDef USBH_RNDIS_Send_Encapsulated_Commnad(
                            USBH_HandleTypeDef *phost,
                            uint8_t messageLen,
                            uint8_t* messageBuff)
{
  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_TYPE_CLASS | \
    USB_REQ_RECIPIENT_INTERFACE;
  
  phost->Control.setup.b.bRequest = USB_REQ_CDC_SEND_ENCAPSULATED;
  
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = 0;   // interface number of the Communication Class IF
  phost->Control.setup.b.wLength.w = messageLen;
  
  return USBH_CtlReq(phost, messageBuff, messageLen );
}
 
USBH_StatusTypeDef USBH_RNDIS_Get_Encapsulated_Response(
                            USBH_HandleTypeDef *phost,
                            uint8_t messageLen,
                            uint8_t* messageBuff)
{
  phost->Control.setup.b.bmRequestType = USB_D2H | USB_REQ_TYPE_CLASS | \
    USB_REQ_RECIPIENT_INTERFACE;
  
  phost->Control.setup.b.bRequest = USB_REQ_CDC_GET_ENCAPSULATED;
  
  phost->Control.setup.b.wValue.w = 0;
  phost->Control.setup.b.wIndex.w = 0;   // interface number of the Communication Class IF
  phost->Control.setup.b.wLength.w = messageLen;
  
  return USBH_CtlReq(phost, messageBuff, messageLen );
}
