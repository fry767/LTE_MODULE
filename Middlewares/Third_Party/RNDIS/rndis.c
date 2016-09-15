#include "rndis.h"
#include "ndis.h"


uint8_t encap_buf_rx[BUF_SIZE];

uint8_t encap_buf_tx[BUF_SIZE];

uint8_t rcv_index = 0;

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
	//memset(encap_buf,0,sizeof(encap_buf));
	rndis_initialize_msg_t *initialisation_message;
	
	initialisation_message = (rndis_initialize_msg_t *)encap_buf_tx;
	initialisation_message->MessageType = REMOTE_NDIS_INITIALIZE_MSG;
	initialisation_message->MessageLength = sizeof(rndis_initialize_msg_t)+4;
	initialisation_message->RequestId = phost->gState;
	initialisation_message->MajorVersion = RNDIS_MAJOR_VERSION;
	initialisation_message->MinorVersion = RNDIS_MINOR_VERSION;
	initialisation_message->MaxTransferSize = phost->device.DevDesc.bMaxPacketSize;
	
	
	return USBH_RNDIS_Send_Encapsulated_Command(phost,((rndis_initialize_msg_t *)encap_buf_tx)->MessageLength,encap_buf_tx);
}
USBH_StatusTypeDef USBH_RNDIS_Get_Initialisation_Complete(USBH_HandleTypeDef *phost)
{
	//memset(encap_buf,0,sizeof(encap_buf));
	rndis_initialize_cmplt_t *rcv_msg;
	rcv_msg = (rndis_initialize_cmplt_t *)encap_buf_rx;
	
	
	__IO USBH_StatusTypeDef status = USBH_FAIL;
	while (status!=RNDIS_USBH_OK) 
		status = USBH_RNDIS_Get_Encapsulated_Response(phost,sizeof(rndis_initialize_cmplt_t),encap_buf_rx);
	
	if(rcv_msg->Status != RNDIS_STATUS_SUCCESS)
		return RNDIS_ERROR_LOGICAL_CMD_FAILED ;
	
	uint32_t num = rcv_msg->MaxTransferSize;
	phost->device.CfgDesc.Itf_Desc[1].Ep_Desc[0].wMaxPacketSize = ((num>>8)&0x0000000f)|((num>>4)&0x0000000f)|((num)&0x0000000f);
	phost->device.CfgDesc.Itf_Desc[1].Ep_Desc[1].wMaxPacketSize = ((num>>8)&0x0000000f)|((num>>4)&0x0000000f)|((num)&0x0000000f);
	return status;
	
}
USBH_StatusTypeDef USBH_RNDIS_Query_Oid_Supported_List(USBH_HandleTypeDef *phost)
{
	//memset(encap_buf,0,sizeof(encap_buf));
	rndis_query_msg_t *query_msg;
	query_msg = (rndis_query_msg_t *)encap_buf_tx;
	query_msg->MessageType = REMOTE_NDIS_QUERY_MSG;
	query_msg->MessageLength = sizeof(rndis_query_msg_t);
	query_msg->RequestId = phost->gState;
	query_msg->Oid = OID_GEN_SUPPORTED_LIST;
	query_msg->InformationBufferLength = 0;
	query_msg->InformationBufferOffset = 0;
	query_msg->DeviceVcHandle = 0;
	
	return USBH_RNDIS_Send_Encapsulated_Command(phost,sizeof(rndis_query_msg_t),encap_buf_tx);
	
}
USBH_StatusTypeDef USBH_RNDIS_Get_Oid_Supported_List(USBH_HandleTypeDef *phost,void* buffer)
{
	//memset(encap_buf,0,sizeof(encap_buf));
		struct
		{
			rndis_query_cmplt_t query_msg;
			uint8_t							buffer[BUF_SIZE - 24];
		}query_msg_data;
		struct query_msg_data* query_msg_data_ptr;

	__IO USBH_StatusTypeDef status = USBH_FAIL;
	
	while (status!=RNDIS_USBH_OK) 
		status = USBH_RNDIS_Get_Encapsulated_Response(phost,sizeof(query_msg_data),(uint8_t*)query_msg_data_ptr);
	
	
	if(query_msg_data.query_msg.Status != RNDIS_STATUS_SUCCESS)
		return RNDIS_ERROR_LOGICAL_CMD_FAILED ;
	memcpy(buffer, &query_msg_data.buffer, (query_msg_data.query_msg.InformationBufferLength));
	return status;
	
}
USBH_StatusTypeDef USBH_RNDIS_Query_Specific_Oid(USBH_HandleTypeDef *phost,uint32_t oid)
{
	memset(encap_buf_tx,0,sizeof(encap_buf_tx));
	__IO USBH_StatusTypeDef status = USBH_FAIL;
	rndis_query_msg_t *query_msg;
	query_msg = (rndis_query_msg_t *)encap_buf_tx;
	query_msg->MessageType = REMOTE_NDIS_QUERY_MSG;
	query_msg->MessageLength = sizeof(rndis_query_msg_t);
	query_msg->RequestId = phost->gState;
	query_msg->Oid = oid;
	query_msg->InformationBufferLength = 0;
	query_msg->InformationBufferOffset = 0;
	query_msg->DeviceVcHandle = 0;
	
	while(status != RNDIS_USBH_OK)
		status = USBH_RNDIS_Send_Encapsulated_Command(phost,sizeof(rndis_query_msg_t),encap_buf_tx);
	
	return status;
}
USBH_StatusTypeDef USBH_RNDIS_Get_Specific_Oid(USBH_HandleTypeDef *phost,uint32_t oid,void* buffer)
{
	memset(encap_buf_rx,0,sizeof(encap_buf_rx));
	 query_msg_data_t* response_msg;
		
	 response_msg = (query_msg_data_t*)encap_buf_rx;
		
	__IO USBH_StatusTypeDef status = USBH_FAIL;
	while (status!=RNDIS_USBH_OK)
		status = USBH_RNDIS_Get_Encapsulated_Response(phost,(sizeof(rndis_query_cmplt_t)+sizeof(buffer)),encap_buf_rx);
		
	
	
	if(response_msg->query_msg.Status != RNDIS_STATUS_SUCCESS)
		return RNDIS_ERROR_LOGICAL_CMD_FAILED ;
	
	memcpy(buffer,(response_msg->buffer),(response_msg->query_msg.InformationBufferLength));
	return status;
}
USBH_StatusTypeDef USBH_RNDIS_Set_Oid_Property(USBH_HandleTypeDef *phost,uint32_t oid,void* buffer)
{
	//memset(encap_buf,0,sizeof(encap_buf));
	set_msg_data_t* set_msg;
	set_msg = (set_msg_data_t*)encap_buf_tx;
	__IO USBH_StatusTypeDef status = USBH_FAIL;
	
	set_msg->set_msg.MessageType = REMOTE_NDIS_SET_MSG;
	set_msg->set_msg.MessageLength = (sizeof(rndis_set_msg_t) + 2);
	set_msg->set_msg.RequestId = phost->gState;
	set_msg->set_msg.Oid = oid;
	set_msg->set_msg.InformationBufferLength = sizeof(buffer);
	set_msg->set_msg.InformationBufferOffset = sizeof(rndis_set_msg_t) - sizeof(rndis_header_t);
	set_msg->set_msg.DeviceVcHandle = 0;
	
	memcpy((set_msg->txBuffer),buffer,sizeof(buffer));

	while(status!= RNDIS_USBH_OK)
		status = USBH_RNDIS_Send_Encapsulated_Command(phost,(sizeof(rndis_set_msg_t) + sizeof(buffer)),encap_buf_tx);
	
	return status;
}
USBH_StatusTypeDef USBH_RNDIS_Get_SetResponse(USBH_HandleTypeDef *phost)
{
	memset(encap_buf_rx,0,sizeof(encap_buf_rx));
	rndis_set_cmplt_t* set_response;
	set_response = (rndis_set_cmplt_t *)encap_buf_rx;
	
	__IO USBH_StatusTypeDef status = USBH_FAIL;
	
	
	
	while(status != RNDIS_USBH_OK)
		status = USBH_RNDIS_Get_Encapsulated_Response(phost,sizeof(rndis_set_cmplt_t),encap_buf_rx); 
	
	if(set_response->Status != RNDIS_STATUS_SUCCESS)
		return RNDIS_ERROR_LOGICAL_CMD_FAILED ;
	

	return status;
	
	
}
USBH_StatusTypeDef USBH_RNDIS_Send_Bulk(USBH_HandleTypeDef *phost,void* buffer)
{
	rndis_data_packet_t* data_msg_to_send;
	memset(data_msg_to_send,0,sizeof(rndis_data_packet_t));
	
	data_msg_to_send->MessageType = REMOTE_NDIS_PACKET_MSG;
	data_msg_to_send->MessageLength = sizeof(rndis_data_packet_t) + sizeof(buffer);
	data_msg_to_send->DataOffset = sizeof(rndis_data_packet_t) - sizeof(rndis_header_t);
	data_msg_to_send->DataLength = sizeof(buffer);
	
	return USBH_CDC_Transmit(phost,buffer,sizeof(buffer));
}
	