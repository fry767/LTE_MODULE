#ifndef RNDIS_H
#define RNDIS_H

#include "usbh_core.h"
#include "usbh_cdc.h"
/**
 * \file rndis_protocol.h
 *         RNDIS Defines
 *
 * \author
 *         Colin O'Flynn <coflynn@newae.com>
 *
 * \addtogroup usbstick
 */

/* Copyright (c) 2008  Colin O'Flynn
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
/** 
  \addtogroup RNDIS
  @{
  */

#include <stdint.h>
#define BUF_SIZE												(27*4+32)
#define USB_REQ_CDC_SEND_ENCAPSULATED   0x00
#define USB_REQ_CDC_GET_ENCAPSULATED    0x01

#define RNDIS_MAJOR_VERSION	1
#define RNDIS_MINOR_VERSION 0

#define RNDIS_STATUS_SUCCESS            0X00000000
#define RNDIS_STATUS_FAILURE            0XC0000001
#define RNDIS_STATUS_INVALID_DATA       0XC0010015
#define RNDIS_STATUS_NOT_SUPPORTED      0XC00000BB
#define RNDIS_STATUS_MEDIA_CONNECT      0X4001000B
#define RNDIS_STATUS_MEDIA_DISCONNECT   0X4001000C

//Error for logical command failure
#define RNDIS_ERROR_LOGICAL_CMD_FAILED        0x80

/* Message set for Connectionless (802.3) Devices */
#define REMOTE_NDIS_PACKET_MSG          0x00000001
#define REMOTE_NDIS_INITIALIZE_MSG      0X00000002
#define REMOTE_NDIS_HALT_MSG            0X00000003
#define REMOTE_NDIS_QUERY_MSG           0X00000004
#define REMOTE_NDIS_SET_MSG             0X00000005
#define REMOTE_NDIS_RESET_MSG           0X00000006
#define REMOTE_NDIS_INDICATE_STATUS_MSG 0X00000007
#define REMOTE_NDIS_KEEPALIVE_MSG       0X00000008
#define REMOTE_NDIS_INITIALIZE_CMPLT    0X80000002
#define REMOTE_NDIS_QUERY_CMPLT         0X80000004
#define REMOTE_NDIS_SET_CMPLT           0X80000005
#define REMOTE_NDIS_RESET_CMPLT         0X80000006
#define REMOTE_NDIS_KEEPALIVE_CMPLT     0X80000008

typedef uint32_t rndis_MessageType_t;
typedef uint32_t rndis_MessageLength_t;
typedef uint32_t rndis_RequestId_t;
typedef uint32_t rndis_MajorVersion_t;
typedef uint32_t rndis_MinorVersion_t;
typedef uint32_t rndis_MaxTransferSize_t;
typedef uint32_t rndis_Status_t;


/* Device Flags */
#define RNDIS_DF_CONNECTIONLESS      0x00000001
#define RNDIS_DF_CONNECTION_ORIENTED 0x00000002
typedef uint32_t rndis_DeviceFlags_t;

/* Mediums */
#define RNDIS_MEDIUM_802_3           0x00000000
typedef uint32_t rndis_Medium_t;


typedef uint32_t rndis_MaxPacketsPerTransfer_t;
typedef uint32_t rndis_PacketAlignmentFactor_t;
typedef uint32_t rndis_AfListOffset_t;
typedef uint32_t rndis_AfListSize_t;

/*** Remote NDIS Generic Message type ***/
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	} rndis_generic_msg_t;


/*** Remote NDIS Initialize Message ***/
typedef struct{
	rndis_MessageType_t 	MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_MajorVersion_t	MajorVersion;
	rndis_MinorVersion_t	MinorVersion;
	rndis_MaxTransferSize_t	MaxTransferSize;
	} rndis_initialize_msg_t;
	
/* Response: */
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_Status_t			Status;
	rndis_MajorVersion_t	MajorVersion;
	rndis_MinorVersion_t	MinorVersion;
	rndis_DeviceFlags_t		DeviceFlags;
	rndis_Medium_t			Medium;
	rndis_MaxPacketsPerTransfer_t 	MaxPacketsPerTransfer;
	rndis_MaxTransferSize_t			MaxTransferSize;
	rndis_PacketAlignmentFactor_t 	PacketAlignmentFactor;
	rndis_AfListOffset_t	AfListOffset;
	rndis_AfListSize_t		AfListSize;
	} rndis_initialize_cmplt_t;
	

/*** Remote NDIS Halt Message ***/
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	} rndis_halt_msg_t;
	
typedef uint32_t rndis_Oid_t;
typedef uint32_t rndis_InformationBufferLength_t;
typedef uint32_t rndis_InformationBufferOffset_t;
typedef uint32_t rndis_DeviceVcHandle_t;

/*** Remote NDIS Query Message ***/
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_Oid_t				Oid;
	rndis_InformationBufferLength_t	InformationBufferLength;
	rndis_InformationBufferOffset_t	InformationBufferOffset;
	rndis_DeviceVcHandle_t			DeviceVcHandle;
	}  rndis_query_msg_t;
	
/* Response: */

typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_Status_t			Status;
	rndis_InformationBufferLength_t	InformationBufferLength;
	rndis_InformationBufferOffset_t	InformationBufferOffset;
	} rndis_query_cmplt_t;
	
/*** Remote NDIS Set Message ***/
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_Oid_t				Oid;
	rndis_InformationBufferLength_t	InformationBufferLength;
	rndis_InformationBufferOffset_t	InformationBufferOffset;
	rndis_DeviceVcHandle_t			DeviceVcHandle;
	} rndis_set_msg_t;
	
/* Response */
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_Status_t			Status;
	}rndis_set_cmplt_t;

/* Information buffer layout for OID_GEN_RNDIS_CONFIG_PARAMETER */
typedef uint32_t rndis_ParameterNameOffset_t;
typedef uint32_t rndis_ParameterNameLength_t;
typedef uint32_t rndis_ParameterType_t;
typedef uint32_t rndis_ParameterValueOffset_t;
typedef uint32_t rndis_ParameterValueLength_t;

#define PARAMETER_TYPE_STRING		2
#define PARAMETER_TYPE_NUMERICAL	0

typedef struct{
	rndis_ParameterNameOffset_t		ParameterNameOffset;
	rndis_ParameterNameLength_t		ParameterNameLength;
	rndis_ParameterType_t			ParameterType;
	rndis_ParameterValueOffset_t	ParameterValueOffset;
	rndis_ParameterValueLength_t	ParameterValueLength;	
	}rndis_config_parameter_t;
	
typedef uint32_t rndis_Reserved_t;

/*** Remote NDIS Soft Reset Message ***/
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_Reserved_t		Reserved;
	} rndis_reset_msg_t;
	
typedef uint32_t rndis_AddressingReset_t;

/* Response: */
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_Status_t			Status;
	rndis_AddressingReset_t	AddressingReset;
	}  rndis_reset_cmplt_t;
	
/*** Remote NDIS Indicate Status Message ***/
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_Status_t			Status;
	rndis_Status_t			StatusBufferLength;
	rndis_Status_t			StatusBufferOffset;
	}  rndis_indicate_status_t;
	
typedef uint32_t rndis_DiagStatus_t;
typedef uint32_t rndis_ErrorOffset_t;

typedef struct {
	rndis_DiagStatus_t		DiagStatus;
	rndis_ErrorOffset_t		ErrorOffset;
	}rndis_diagnostic_info_t;
	
/*** Remote NDIS Keepalive Message */
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	}rndis_keepalive_msg_t;
	
/* Response: */
typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
	rndis_RequestId_t		RequestId;
	rndis_Status_t			Status;
	}rndis_keepalive_cmplt_t;

/*** Remote NDIS Data Packet ***/

typedef uint32_t rndis_DataOffset_t;
typedef uint32_t rndis_DataLength_t;
typedef uint32_t rndis_OOBDataOffset_t;
typedef uint32_t rndis_OOBDataLength_t;
typedef uint32_t rndis_NumOOBDataElements_t;
typedef uint32_t rndis_PerPacketInfoOffset_t;
typedef uint32_t rndis_PerPacketInfoLength_t;

typedef struct{
	rndis_MessageType_t			MessageType;
	rndis_MessageLength_t		MessageLength;
	rndis_DataOffset_t			DataOffset;
	rndis_DataLength_t			DataLength;
	rndis_OOBDataOffset_t		OOBDataOffset;
	rndis_OOBDataLength_t		OOBDataLength;
	rndis_NumOOBDataElements_t	NumOOBDataElements;
	rndis_PerPacketInfoOffset_t	PerPacketInfoOffset;
	rndis_PerPacketInfoLength_t PerPacketInfoLength;
	rndis_DeviceVcHandle_t		DeviceVcHandle;
	rndis_Reserved_t			Reserved;
	}rndis_data_packet_t;

typedef struct{
	rndis_MessageType_t		MessageType;
	rndis_MessageLength_t	MessageLength;
}rndis_header_t;


typedef uint32_t rndis_ClassInformationOffset_t;
typedef uint32_t rndis_Size_t;
typedef uint32_t rndis_Type_t;

typedef struct{
	rndis_Size_t					Size;
	rndis_Type_t					Type;
	rndis_ClassInformationOffset_t	ClassInformationType;
	}rndis_OOB_packet_t;



typedef enum rnids_state_e {
	rndis_uninitialized,
	rndis_initialized,
	rndis_data_initialized
	} rndis_state_t;
typedef struct query_message_data{
	rndis_query_cmplt_t query_msg;
	uint8_t							buffer[BUF_SIZE - 24];
}query_msg_data_t;

	typedef struct set_msg_data{
		rndis_set_msg_t set_msg;
		uint8_t txBuffer[4*8];
	}set_msg_data_t;

typedef enum rndis_usb_state {
	RNDIS_USBH_OK = 0,
	RNDIS_USBH_BUSY,
}rndis_usbh_state_t;

typedef struct {
	uint32_t		txok;
	uint32_t		rxok;
	uint32_t		txbad;
	uint32_t		rxbad;
} usb_eth_stat_t;
//Maximum size in bytes of an Ethernet frame according to the Ethernet standard. 
		#define ETHERNET_FRAME_SIZE_MAX               1500
		
USBH_StatusTypeDef USBH_RNDIS_Send_Encapsulated_Command(
                            USBH_HandleTypeDef *phost,
                            uint8_t messageLen,
                            uint8_t* messageBuff);
														
USBH_StatusTypeDef USBH_RNDIS_Get_Encapsulated_Response(
                            USBH_HandleTypeDef *phost,
                            uint8_t messageLen,
                            uint8_t* messageBuff);
USBH_StatusTypeDef USBH_RNDIS_Send_Initialisation_Message(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_RNDIS_Get_Initialisation_Complete(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_RNDIS_Query_Oid_Supported_List(USBH_HandleTypeDef *phost);
USBH_StatusTypeDef USBH_RNDIS_Get_Oid_Supported_List(USBH_HandleTypeDef *phost,void* buffer);
USBH_StatusTypeDef USBH_RNDIS_Query_Specific_Oid(USBH_HandleTypeDef *phost,uint32_t oid);
USBH_StatusTypeDef USBH_RNDIS_Get_Specific_Oid(USBH_HandleTypeDef *phost,uint32_t oid,void* buffer);
USBH_StatusTypeDef USBH_RNDIS_Set_Oid_Property(USBH_HandleTypeDef *phost,uint32_t oid,void* buffer);
USBH_StatusTypeDef USBH_RNDIS_Get_SetResponse(USBH_HandleTypeDef *phost);

#endif //RNDIS_H
