#include "rndis.h"
#include "ndis.h"
const uint32_t OIDSupportedList[] = 
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
//    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
//    OID_GEN_TRANSMIT_BUFFER_SPACE,
//    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
//    OID_GEN_CURRENT_LOOKAHEAD,
//    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_MAC_OPTIONS
};
uint8_t encap_buf[50];

USBH_StatusTypeDef USBH_RNDIS_Send_Encapsulated_Command(
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
USBH_StatusTypeDef USBH_RNDIS_Send_Initialisation_Message(USBH_HandleTypeDef *phost)
{
	rndis_initialize_msg_t *initialisation_message;
	
	
	initialisation_message = (rndis_initialize_msg_t *)encap_buf;
	initialisation_message->MessageType = REMOTE_NDIS_INITIALIZE_MSG;
	initialisation_message->MessageLength = sizeof(rndis_generic_msg_t)+4;
	initialisation_message->RequestId = phost->gState;
	initialisation_message->MajorVersion = RNDIS_MAJOR_VERSION;
	initialisation_message->MinorVersion = RNDIS_MINOR_VERSION;
	initialisation_message->MaxTransferSize = phost->device.DevDesc.bMaxPacketSize;
	
	
	return USBH_RNDIS_Send_Encapsulated_Command(phost,((rndis_initialize_msg_t *)encap_buf)->MessageLength,encap_buf);
}
USBH_StatusTypeDef USBH_RNDIS_Get_Initialisation_Complete(USBH_HandleTypeDef *phost)
{
	rndis_initialize_cmplt_t *rcv_msg;
	rcv_msg = (rndis_initialize_cmplt_t *)encap_buf;
	__IO USBH_StatusTypeDef status = USBH_FAIL;
	
	status = USBH_RNDIS_Get_Encapsulated_Response(phost,sizeof(rndis_initialize_cmplt_t),encap_buf);
	if(status != RNDIS_USBH_OK)
		return status;
	
	if(rcv_msg->Status != RNDIS_STATUS_SUCCESS)
		return RNDIS_ERROR_LOGICAL_CMD_FAILED ;
	
	uint32_t num = rcv_msg->MaxTransferSize;
	phost->device.CfgDesc.Itf_Desc[1].Ep_Desc[0].wMaxPacketSize = ((num>>8)&0x0000000f)|((num>>4)&0x0000000f)|((num)&0x0000000f);
	phost->device.CfgDesc.Itf_Desc[1].Ep_Desc[1].wMaxPacketSize = ((num>>8)&0x0000000f)|((num>>4)&0x0000000f)|((num)&0x0000000f);
	return status;
	
}
