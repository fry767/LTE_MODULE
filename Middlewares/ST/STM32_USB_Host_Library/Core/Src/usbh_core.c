/**
  ******************************************************************************
  * @file    usbh_core.c 
  * @author  MCD Application Team
  * @version V3.2.2
  * @date    07-July-2015
  * @brief   This file implements the functions for the core state machine process
  *          the enumeration and the control transfer process
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 
/* Includes ------------------------------------------------------------------*/

#include "usbh_core.h"
#include "rndis.h"
#include "ndis.h"
/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_LIB_CORE
  * @{
  */

/** @defgroup USBH_CORE 
  * @brief This file handles the basic enumeration when a device is connected 
  *          to the host.
  * @{
  */ 


/** @defgroup USBH_CORE_Private_Defines
  * @{
  */ 
#define USBH_ADDRESS_DEFAULT                     0
#define USBH_ADDRESS_ASSIGNED                    1      
#define USBH_MPS_DEFAULT                         0x40
/**
  * @}
  */ 

/** @defgroup USBH_CORE_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup USBH_CORE_Private_Variables
  * @{
  */ 
/**
  * @}
  */ 
 

/** @defgroup USBH_CORE_Private_Functions
  * @{
  */ 
static USBH_StatusTypeDef  USBH_HandleEnum    (USBH_HandleTypeDef *phost);
static void                USBH_HandleSof     (USBH_HandleTypeDef *phost);
static USBH_StatusTypeDef  DeInitStateMachine(USBH_HandleTypeDef *phost);

#if (USBH_USE_OS == 1)  
static void USBH_Process_OS(void const * argument);
#endif

#define ETH_HEADER_SIZE             14
#define RNDIS_RX_BUFFER_SIZE        (ETH_MAX_PACKET_SIZE + sizeof(rndis_data_packet_t))

typedef enum host_class_request_state{
	SEND_INIT_MSG = 0,
	RCV_INIT_MSG,
	QUERY_OID_SUPPORTED,
	RCV_OID_SUPPORTED,
	QUERY_OID_MAX_FRAME_SIZE,
	RCV_OID_MAX_FRAME_SIZE,
	QUERY_MAC_ADDRESS,
	RCV_MAC_ADRESS,
	QUERY_CONNECT_STATUS,
	RCV_CONNECT_STATUS,
	SEND_PACKET_FILTER,
	RCV_PACKET_FILTER_RESPONSE,
	QUERY_PACKET_FILTER,
	RCV_PACKET_FILTER
}host_class_request_state_t;

uint8_t host_class_request_index = SEND_INIT_MSG;
uint8_t usbh_busy_index = 0;
uint32_t*	oid_supported_buf;
uint8_t RNDIS_Data_buf[27*4];
uint16_t data_to_set = 0;
/**
  * @brief  HCD_Init 
  *         Initialize the HOST Core.
  * @param  phost: Host Handle
  * @param  pUsrFunc: User Callback
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Init(USBH_HandleTypeDef *phost, void (*pUsrFunc)(USBH_HandleTypeDef *phost, uint8_t ), uint8_t id)
{
  /* Check whether the USB Host handle is valid */
  if(phost == NULL)
  {
    USBH_ErrLog("Invalid Host handle");
    return USBH_FAIL; 
  }
  
  /* Set DRiver ID */
  phost->id = id;
  
  /* Unlink class*/
  phost->pActiveClass = NULL;
  phost->ClassNumber = 0;
  
  /* Restore default states and prepare EP0 */ 
  DeInitStateMachine(phost);
  
  /* Assign User process */
  if(pUsrFunc != NULL)
  {
    phost->pUser = pUsrFunc;
  }
  
#if (USBH_USE_OS == 1) 
  
  /* Create USB Host Queue */
  osMessageQDef(USBH_Queue, 10, uint16_t);
  phost->os_event = osMessageCreate (osMessageQ(USBH_Queue), NULL); 
  
  /*Create USB Host Task */
#if defined (USBH_PROCESS_STACK_SIZE)
  osThreadDef(USBH_Thread, USBH_Process_OS, USBH_PROCESS_PRIO, 0, USBH_PROCESS_STACK_SIZE);
#else
  osThreadDef(USBH_Thread, USBH_Process_OS, USBH_PROCESS_PRIO, 0, 8 * configMINIMAL_STACK_SIZE);
#endif  
  phost->thread = osThreadCreate (osThread(USBH_Thread), phost);
#endif  
  
  /* Initialize low level driver */
  USBH_LL_Init(phost);
  return USBH_OK;
}

/**
  * @brief  HCD_Init 
  *         De-Initialize the Host portion of the driver.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_DeInit(USBH_HandleTypeDef *phost)
{
  DeInitStateMachine(phost);
  
  if(phost->pData != NULL)
  {
    phost->pActiveClass->pData = NULL;
    USBH_LL_Stop(phost);
  }

  return USBH_OK;
}

/**
  * @brief  DeInitStateMachine 
  *         De-Initialize the Host state machine.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef  DeInitStateMachine(USBH_HandleTypeDef *phost)
{
  uint32_t i = 0;

  /* Clear Pipes flags*/
  for ( ; i < USBH_MAX_PIPES_NBR; i++)
  {
    phost->Pipes[i] = 0;
  }
  
  for(i = 0; i< USBH_MAX_DATA_BUFFER; i++)
  {
    phost->device.Data[i] = 0;
  }
  
  phost->gState = HOST_IDLE;
  phost->EnumState = ENUM_IDLE;
  phost->RequestState = CMD_SEND;
  phost->Timer = 0;  
  
  phost->Control.state = CTRL_SETUP;
  phost->Control.pipe_size = USBH_MPS_DEFAULT;  
  phost->Control.errorcount = 0;
  
  phost->device.address = USBH_ADDRESS_DEFAULT;
  phost->device.speed   = USBH_SPEED_FULL;
  
  return USBH_OK;
}

/**
  * @brief  USBH_RegisterClass 
  *         Link class driver to Host Core.
  * @param  phost : Host Handle
  * @param  pclass: Class handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_RegisterClass(USBH_HandleTypeDef *phost, USBH_ClassTypeDef *pclass)
{
  USBH_StatusTypeDef   status = USBH_OK;
  
  if(pclass != 0)
  {
    if(phost->ClassNumber < USBH_MAX_NUM_SUPPORTED_CLASS)
    {
      /* link the class to the USB Host handle */
      phost->pClass[phost->ClassNumber++] = pclass;
      status = USBH_OK;
    }
    else
    {
      USBH_ErrLog("Max Class Number reached");
      status = USBH_FAIL; 
    }
  }
  else
  {
    USBH_ErrLog("Invalid Class handle");
    status = USBH_FAIL; 
  }
  
  return status;
}

/**
  * @brief  USBH_SelectInterface 
  *         Select current interface.
  * @param  phost: Host Handle
  * @param  interface: Interface number
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SelectInterface(USBH_HandleTypeDef *phost, uint8_t interface)
{
  USBH_StatusTypeDef   status = USBH_OK;
  
  if(interface < phost->device.CfgDesc.bNumInterfaces)
  {
    phost->device.current_interface = interface;
    USBH_UsrLog ("Switching to Interface (#%d)", interface);
    USBH_UsrLog ("Class    : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceClass );
    USBH_UsrLog ("SubClass : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceSubClass );
    USBH_UsrLog ("Protocol : %xh", phost->device.CfgDesc.Itf_Desc[interface].bInterfaceProtocol );                 
  }
  else
  {
    USBH_ErrLog ("Cannot Select This Interface.");
    status = USBH_FAIL; 
  }
  return status;  
}

/**
  * @brief  USBH_GetActiveClass 
  *         Return Device Class.
  * @param  phost: Host Handle
  * @param  interface: Interface index
  * @retval Class Code
  */
uint8_t USBH_GetActiveClass(USBH_HandleTypeDef *phost)
{
   return (phost->device.CfgDesc.Itf_Desc[0].bInterfaceClass);            
}
/**
  * @brief  USBH_FindInterface 
  *         Find the interface index for a specific class.
  * @param  phost: Host Handle
  * @param  Class: Class code
  * @param  SubClass: SubClass code
  * @param  Protocol: Protocol code
  * @retval interface index in the configuration structure
  * @note : (1)interface index 0xFF means interface index not found
  */
uint8_t  USBH_FindInterface(USBH_HandleTypeDef *phost, uint8_t Class, uint8_t SubClass, uint8_t Protocol)
{
  USBH_InterfaceDescTypeDef    *pif ;
  USBH_CfgDescTypeDef          *pcfg ;
  int8_t                        if_ix = 0;
  
  pif = (USBH_InterfaceDescTypeDef *)0;
  pcfg = &phost->device.CfgDesc;  
  
  while (if_ix < USBH_MAX_NUM_INTERFACES)
  {
    pif = &pcfg->Itf_Desc[if_ix];
    if(((pif->bInterfaceClass == Class) || (Class == 0xFF))&&
       ((pif->bInterfaceSubClass == SubClass) || (SubClass == 0xFF))&&
         ((pif->bInterfaceProtocol == Protocol) || (Protocol == 0xFF)))
    {
      return  if_ix;
    }
    if_ix++;
  }
  return 0xFF;
}

/**
  * @brief  USBH_FindInterfaceIndex 
  *         Find the interface index for a specific class interface and alternate setting number.
  * @param  phost: Host Handle
  * @param  interface_number: interface number
  * @param  alt_settings    : alternate setting number
  * @retval interface index in the configuration structure
  * @note : (1)interface index 0xFF means interface index not found
  */
uint8_t  USBH_FindInterfaceIndex(USBH_HandleTypeDef *phost, uint8_t interface_number, uint8_t alt_settings)
{
  USBH_InterfaceDescTypeDef    *pif ;
  USBH_CfgDescTypeDef          *pcfg ;
  int8_t                        if_ix = 0;
  
  pif = (USBH_InterfaceDescTypeDef *)0;
  pcfg = &phost->device.CfgDesc;  
  
  while (if_ix < USBH_MAX_NUM_INTERFACES)
  {
    pif = &pcfg->Itf_Desc[if_ix];
    if((pif->bInterfaceNumber == interface_number) && (pif->bAlternateSetting == alt_settings))
    {
      return  if_ix;
    }
    if_ix++;
  }
  return 0xFF;
}

/**
  * @brief  USBH_Start 
  *         Start the USB Host Core.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Start  (USBH_HandleTypeDef *phost)
{
  /* Start the low level driver  */
  USBH_LL_Start(phost);
  
  /* Activate VBUS on the port */ 
  USBH_LL_DriverVBUS (phost, TRUE);
  
  return USBH_OK;  
}

/**
  * @brief  USBH_Stop 
  *         Stop the USB Host Core.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Stop   (USBH_HandleTypeDef *phost)
{
  /* Stop and cleanup the low level driver  */
  USBH_LL_Stop(phost);  
  
  /* DeActivate VBUS on the port */ 
  USBH_LL_DriverVBUS (phost, FALSE);
  
  /* FRee Control Pipes */
  USBH_FreePipe  (phost, phost->Control.pipe_in);
  USBH_FreePipe  (phost, phost->Control.pipe_out);  
  
  return USBH_OK;  
}

/**
  * @brief  HCD_ReEnumerate 
  *         Perform a new Enumeration phase.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_ReEnumerate   (USBH_HandleTypeDef *phost)
{
  /*Stop Host */ 
  USBH_Stop(phost);

  /*Device has disconnected, so wait for 200 ms */  
  USBH_Delay(200);
  
  /* Set State machines in default state */
  DeInitStateMachine(phost);
   
  /* Start again the host */
  USBH_Start(phost);
      
#if (USBH_USE_OS == 1)
      osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif  
  return USBH_OK;  
}

/**
  * @brief  USBH_Process 
  *         Background process of the USB Core.
  * @param  phost: Host Handle
  * @retval USBH Status
  */
USBH_StatusTypeDef  USBH_Process(USBH_HandleTypeDef *phost)
{
  __IO USBH_StatusTypeDef status = USBH_FAIL;
  uint8_t idx = 0;
  
  switch (phost->gState)
  {
  case HOST_IDLE :
    
    if (phost->device.is_connected)  
    {
      /* Wait for 200 ms after connection */
      phost->gState = HOST_DEV_WAIT_FOR_ATTACHMENT; 
      USBH_Delay(200); 
      USBH_LL_ResetPort(phost);
#if (USBH_USE_OS == 1)
      osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif
    }
    break;
    
  case HOST_DEV_WAIT_FOR_ATTACHMENT:
    break;    
    
  case HOST_DEV_ATTACHED :
    
    USBH_UsrLog("USB Device Attached");  
      
    /* Wait for 100 ms after Reset */
    USBH_Delay(100); 
          
    phost->device.speed = USBH_LL_GetSpeed(phost);
    
    phost->gState = HOST_ENUMERATION;
    
    phost->Control.pipe_out = USBH_AllocPipe (phost, 0x00);
    phost->Control.pipe_in  = USBH_AllocPipe (phost, 0x80);    
    
    
    /* Open Control pipes */
    USBH_OpenPipe (phost,
                   phost->Control.pipe_in,
                   0x80,
                   phost->device.address,
                   phost->device.speed,
                   USBH_EP_CONTROL,
                   phost->Control.pipe_size); 
    
    /* Open Control pipes */
    USBH_OpenPipe (phost,
                   phost->Control.pipe_out,
                   0x00,
                   phost->device.address,
                   phost->device.speed,
                   USBH_EP_CONTROL,
                   phost->Control.pipe_size);
    
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif    
    
    break;
    
  case HOST_ENUMERATION:     
    /* Check for enumeration status */  
    if ( USBH_HandleEnum(phost) == USBH_OK)
    { 
      /* The function shall return USBH_OK when full enumeration is complete */
      USBH_UsrLog ("Enumeration done.");
      phost->device.current_interface = 0;
      if(phost->device.DevDesc.bNumConfigurations == 1)
      {
        USBH_UsrLog ("This device has only 1 configuration.");
        phost->gState  = HOST_SET_CONFIGURATION;        
        
      }
      else
      {
        phost->gState  = HOST_INPUT; 
      }
          
    }
    break;
    
  case HOST_INPUT:
    {
      /* user callback for end of device basic enumeration */
      if(phost->pUser != NULL)
      {
        phost->pUser(phost, HOST_USER_SELECT_CONFIGURATION);
        phost->gState = HOST_SET_CONFIGURATION;
        
#if (USBH_USE_OS == 1)
        osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif         
      }
    }
    break;
    
  case HOST_SET_CONFIGURATION:
    /* set configuration */
    if (USBH_SetCfg(phost, phost->device.CfgDesc.bConfigurationValue) == USBH_OK)
    {
      phost->gState  = HOST_CHECK_CLASS;
      USBH_UsrLog ("Default configuration set.");
      
    }      
    
    break;
    
  case HOST_CHECK_CLASS:
    
    if(phost->ClassNumber == 0)
    {
      USBH_UsrLog ("No Class has been registered.");
    }
    else
    {
      phost->pActiveClass = NULL;
      
      for (idx = 0; idx < USBH_MAX_NUM_SUPPORTED_CLASS ; idx ++)
      {
        if(phost->pClass[idx]->ClassCode == phost->device.CfgDesc.Itf_Desc[0].bInterfaceClass)
        {
          phost->pActiveClass = phost->pClass[idx];
        }
      }
      
      if(phost->pActiveClass != NULL)
      {
        if(phost->pActiveClass->Init(phost)== USBH_OK)
        {
          phost->gState  = HOST_CLASS_REQUEST; 
          USBH_UsrLog ("%s class started.", phost->pActiveClass->Name);
          
          /* Inform user that a class has been activated */
          phost->pUser(phost, HOST_USER_CLASS_SELECTED);   
        }
        else
        {
          phost->gState  = HOST_ABORT_STATE;
          USBH_UsrLog ("Device not supporting %s class.", phost->pActiveClass->Name);
        }
      }
      else
      {
        phost->gState  = HOST_ABORT_STATE;
        USBH_UsrLog ("No registered class for this device.");
      }
    }
    
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif 
    break;    
    
  case HOST_CLASS_REQUEST:  
    /* process class standard control requests state machine */
    if(phost->pActiveClass != NULL)
    {
			switch(host_class_request_index)
			{
				uint8_t device_max_frame_size[4];
				uint8_t device_connect_status[4];
				uint8_t device_packet_filter[4];
				uint8_t device_mac_adress[6];
				case SEND_INIT_MSG : 
						//status = phost->pActiveClass->Requests(phost);
					status = USBH_RNDIS_Send_Initialisation_Message(phost);
					if(status != USBH_OK)
					{
						return status;
					} 
					host_class_request_index++;
				break;
					
				case RCV_INIT_MSG : 
					status = USBH_RNDIS_Get_Initialisation_Complete(phost);
					if(status != USBH_OK)
					{
						
						return status;	
					}		
					
					host_class_request_index ++;
				break;
				
				case QUERY_OID_SUPPORTED : 
					status = USBH_RNDIS_Query_Specific_Oid(phost,OID_GEN_SUPPORTED_LIST);
					if(status!=USBH_OK)
						return status;
					host_class_request_index ++;
				break;
				case RCV_OID_SUPPORTED :
					status = USBH_RNDIS_Get_Specific_Oid(phost,OID_GEN_SUPPORTED_LIST,RNDIS_Data_buf);
					if(status != USBH_OK)
					{		
						return status;	
						
					}		
					host_class_request_index ++;
					
				break;
				case QUERY_OID_MAX_FRAME_SIZE:
					status = USBH_RNDIS_Query_Specific_Oid(phost,OID_GEN_MAXIMUM_FRAME_SIZE);
					if(status!=USBH_OK)
						return status;
					host_class_request_index ++;
				break;
				case RCV_OID_MAX_FRAME_SIZE :
					status = USBH_RNDIS_Get_Specific_Oid(phost,OID_GEN_MAXIMUM_FRAME_SIZE,device_max_frame_size);
					if(status != USBH_OK)
					{
						return status;	
					}	
					host_class_request_index ++;
				break;
				case QUERY_MAC_ADDRESS:
					status = USBH_RNDIS_Query_Specific_Oid(phost,OID_802_3_CURRENT_ADDRESS);
					if(status!=USBH_OK)
						return status;
					host_class_request_index ++;
				break;
				case RCV_MAC_ADRESS:
					status = USBH_RNDIS_Get_Specific_Oid(phost,OID_802_3_CURRENT_ADDRESS,device_mac_adress);
					if(status != USBH_OK)
					{
						return status;	
					}		

					host_class_request_index ++;
				break;
				case QUERY_CONNECT_STATUS:
					status = USBH_RNDIS_Query_Specific_Oid(phost,OID_GEN_MEDIA_CONNECT_STATUS);
					if(status!=USBH_OK)
						return status;
					host_class_request_index ++;
				break;
				case RCV_CONNECT_STATUS:
					status = USBH_RNDIS_Get_Specific_Oid(phost,OID_GEN_MEDIA_CONNECT_STATUS,device_connect_status);
					if(status != USBH_OK )
					{
						return status;	
					}		
					
					host_class_request_index ++;
				break;
				case SEND_PACKET_FILTER : 
					data_to_set = PACKET_FILTER;
					status = USBH_RNDIS_Set_Oid_Property(phost,OID_GEN_CURRENT_PACKET_FILTER,&data_to_set);
					if(status != USBH_OK)
					{
						return status;	
					}	
					host_class_request_index ++;
				break;
				case RCV_PACKET_FILTER_RESPONSE :
					status = USBH_RNDIS_Get_SetResponse(phost);
					if(status!=USBH_OK)
					{
						return status;
					}
					host_class_request_index ++;	
				break;
				case QUERY_PACKET_FILTER :
					status = USBH_RNDIS_Query_Specific_Oid(phost,OID_GEN_CURRENT_PACKET_FILTER);
					if(status!=USBH_OK)
						return status;
					host_class_request_index ++;
				break;
				case RCV_PACKET_FILTER:
					
					status = USBH_RNDIS_Get_Specific_Oid(phost,OID_GEN_CURRENT_PACKET_FILTER,device_packet_filter);
					if(status != USBH_OK)
					{
						return status;	
					}
					if((device_packet_filter[3]|device_packet_filter[2]|device_packet_filter[1]|device_packet_filter[0])!=PACKET_FILTER)
					{
						host_class_request_index = SEND_PACKET_FILTER;
						return status;
					}
					host_class_request_index = SEND_INIT_MSG;
					phost->pUser(phost, HOST_USER_CLASS_ACTIVE); 
					phost->gState = HOST_CLASS;
				break;
			}
    }
    else
    {
      phost->gState  = HOST_ABORT_STATE;
      USBH_ErrLog ("Invalid Class Driver.");
    
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif       
    }
    
    break;    
  case HOST_CLASS:   
    /* process class state machine */
    if(phost->pActiveClass != NULL)
    { 
      phost->pActiveClass->BgndProcess(phost);
    }
    break;       

  case HOST_DEV_DISCONNECTED :
    
    DeInitStateMachine(phost);  
    
    /* Re-Initilaize Host for new Enumeration */
    if(phost->pActiveClass != NULL)
    {
      phost->pActiveClass->DeInit(phost); 
      phost->pActiveClass = NULL;
    }     
    break;
    
  case HOST_ABORT_STATE:
  default :
    break;
  }
 return USBH_OK;  
}


/**
  * @brief  USBH_HandleEnum 
  *         This function includes the complete enumeration process
  * @param  phost: Host Handle
  * @retval USBH_Status
  */
static USBH_StatusTypeDef USBH_HandleEnum (USBH_HandleTypeDef *phost)
{
  USBH_StatusTypeDef Status = USBH_BUSY;  
  
  switch (phost->EnumState)
  {
  case ENUM_IDLE:  
    /* Get Device Desc for only 1st 8 bytes : To get EP0 MaxPacketSize */
    if ( USBH_Get_DevDesc(phost, 8) == USBH_OK)
    {
      phost->Control.pipe_size = phost->device.DevDesc.bMaxPacketSize;

      phost->EnumState = ENUM_GET_FULL_DEV_DESC;
      
      /* modify control channels configuration for MaxPacket size */
      USBH_OpenPipe (phost,
                           phost->Control.pipe_in,
                           0x80,
                           phost->device.address,
                           phost->device.speed,
                           USBH_EP_CONTROL,
                           phost->Control.pipe_size); 
      
      /* Open Control pipes */
      USBH_OpenPipe (phost,
                           phost->Control.pipe_out,
                           0x00,
                           phost->device.address,
                           phost->device.speed,
                           USBH_EP_CONTROL,
                           phost->Control.pipe_size);           
      
    }
    break;
    
  case ENUM_GET_FULL_DEV_DESC:  
    /* Get FULL Device Desc  */
    if ( USBH_Get_DevDesc(phost, USB_DEVICE_DESC_SIZE)== USBH_OK)
    {
      USBH_UsrLog("PID: %xh", phost->device.DevDesc.idProduct );  
      USBH_UsrLog("VID: %xh", phost->device.DevDesc.idVendor );  
      
      phost->EnumState = ENUM_SET_ADDR;
       
    }
    break;
   
  case ENUM_SET_ADDR: 
    /* set address */
    if ( USBH_SetAddress(phost, USBH_DEVICE_ADDRESS) == USBH_OK)
    {
      USBH_Delay(2);
      phost->device.address = USBH_DEVICE_ADDRESS;
      
      /* user callback for device address assigned */
      USBH_UsrLog("Address (#%d) assigned.", phost->device.address);
      phost->EnumState = ENUM_GET_CFG_DESC;
      
      /* modify control channels to update device address */
      USBH_OpenPipe (phost,
                           phost->Control.pipe_in,
                           0x80,
                           phost->device.address,
                           phost->device.speed,
                           USBH_EP_CONTROL,
                           phost->Control.pipe_size); 
      
      /* Open Control pipes */
      USBH_OpenPipe (phost,
                           phost->Control.pipe_out,
                           0x00,
                           phost->device.address,
                           phost->device.speed,
                           USBH_EP_CONTROL,
                           phost->Control.pipe_size);        
    }
    break;
    
  case ENUM_GET_CFG_DESC:  
    /* get standard configuration descriptor */
    if ( USBH_Get_CfgDesc(phost, 
                          USB_CONFIGURATION_DESC_SIZE) == USBH_OK)
    {
      phost->EnumState = ENUM_GET_FULL_CFG_DESC;        
    }
    break;
    
  case ENUM_GET_FULL_CFG_DESC:  
    /* get FULL config descriptor (config, interface, endpoints) */
    if (USBH_Get_CfgDesc(phost, 
                         phost->device.CfgDesc.wTotalLength) == USBH_OK)
    {
      phost->EnumState = ENUM_GET_MFC_STRING_DESC;       
    }
    break;
    
  case ENUM_GET_MFC_STRING_DESC:  
    if (phost->device.DevDesc.iManufacturer != 0)
    { /* Check that Manufacturer String is available */
      
      if ( USBH_Get_StringDesc(phost,
                               phost->device.DevDesc.iManufacturer, 
                                phost->device.Data , 
                               0xff) == USBH_OK)
      {
        /* User callback for Manufacturing string */
        USBH_UsrLog("Manufacturer : %s",  (char *)phost->device.Data);
        phost->EnumState = ENUM_GET_PRODUCT_STRING_DESC;
        
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif          
      }
    }
    else
    {
     USBH_UsrLog("Manufacturer : N/A");      
     phost->EnumState = ENUM_GET_PRODUCT_STRING_DESC; 
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif       
    }
    break;
    
  case ENUM_GET_PRODUCT_STRING_DESC:   
    if (phost->device.DevDesc.iProduct != 0)
    { /* Check that Product string is available */
      if ( USBH_Get_StringDesc(phost,
                               phost->device.DevDesc.iProduct, 
                               phost->device.Data, 
                               0xff) == USBH_OK)
      {
        /* User callback for Product string */
        USBH_UsrLog("Product : %s",  (char *)phost->device.Data);
        phost->EnumState = ENUM_GET_SERIALNUM_STRING_DESC;        
      }
    }
    else
    {
      USBH_UsrLog("Product : N/A");
      phost->EnumState = ENUM_GET_SERIALNUM_STRING_DESC; 
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif        
    } 
    break;
    
  case ENUM_GET_SERIALNUM_STRING_DESC:   
    if (phost->device.DevDesc.iSerialNumber != 0)
    { /* Check that Serial number string is available */    
      if ( USBH_Get_StringDesc(phost,
                               phost->device.DevDesc.iSerialNumber, 
                               phost->device.Data, 
                               0xff) == USBH_OK)
      {
        /* User callback for Serial number string */
         USBH_UsrLog("Serial Number : %s",  (char *)phost->device.Data);
        Status = USBH_OK;
      }
    }
    else
    {
      USBH_UsrLog("Serial Number : N/A"); 
      Status = USBH_OK;
#if (USBH_USE_OS == 1)
    osMessagePut ( phost->os_event, USBH_STATE_CHANGED_EVENT, 0);
#endif        
    }  
    break;
    
  default:
    break;
  }  
  return Status;
}

/**
  * @brief  USBH_LL_SetTimer 
  *         Set the initial Host Timer tick
  * @param  phost: Host Handle
  * @retval None
  */
void  USBH_LL_SetTimer  (USBH_HandleTypeDef *phost, uint32_t time)
{
  phost->Timer = time;
}
/**
  * @brief  USBH_LL_IncTimer 
  *         Increment Host Timer tick
  * @param  phost: Host Handle
  * @retval None
  */
void  USBH_LL_IncTimer  (USBH_HandleTypeDef *phost)
{
  phost->Timer ++;
  USBH_HandleSof(phost);
}

/**
  * @brief  USBH_HandleSof 
  *         Call SOF process
  * @param  phost: Host Handle
  * @retval None
  */
void  USBH_HandleSof  (USBH_HandleTypeDef *phost)
{
  if((phost->gState == HOST_CLASS)&&(phost->pActiveClass != NULL))
  {
    phost->pActiveClass->SOFProcess(phost);
  }
}
/**
  * @brief  USBH_LL_Connect 
  *         Handle USB Host connexion event
  * @param  phost: Host Handle
  * @retval USBH_Status
  */
USBH_StatusTypeDef  USBH_LL_Connect  (USBH_HandleTypeDef *phost)
{
  if(phost->gState == HOST_IDLE )
  {
    phost->device.is_connected = 1;
    
    if(phost->pUser != NULL)
    {    
      phost->pUser(phost, HOST_USER_CONNECTION);
    }
  } 
  else if(phost->gState == HOST_DEV_WAIT_FOR_ATTACHMENT )
  {
    phost->gState = HOST_DEV_ATTACHED ;
  }
#if (USBH_USE_OS == 1)
  osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif 
  
  return USBH_OK;
}

/**
  * @brief  USBH_LL_Disconnect 
  *         Handle USB Host disconnection event
  * @param  phost: Host Handle
  * @retval USBH_Status
  */
USBH_StatusTypeDef  USBH_LL_Disconnect  (USBH_HandleTypeDef *phost)
{
  /*Stop Host */ 
  USBH_LL_Stop(phost);  
  
  /* FRee Control Pipes */
  USBH_FreePipe  (phost, phost->Control.pipe_in);
  USBH_FreePipe  (phost, phost->Control.pipe_out);  
   
  phost->device.is_connected = 0; 
   
  if(phost->pUser != NULL)
  {    
    phost->pUser(phost, HOST_USER_DISCONNECTION);
  }
  USBH_UsrLog("USB Device disconnected"); 
  
  /* Start the low level driver  */
  USBH_LL_Start(phost);
  
  phost->gState = HOST_DEV_DISCONNECTED;
  
#if (USBH_USE_OS == 1)
  osMessagePut ( phost->os_event, USBH_PORT_EVENT, 0);
#endif 
  
  return USBH_OK;
}


#if (USBH_USE_OS == 1)  
/**
  * @brief  USB Host Thread task
  * @param  pvParameters not used
  * @retval None
  */
static void USBH_Process_OS(void const * argument)
{
  osEvent event;
  
  for(;;)
  {
    event = osMessageGet(((USBH_HandleTypeDef *)argument)->os_event, osWaitForever );
    
    if( event.status == osEventMessage )
    {
      USBH_Process((USBH_HandleTypeDef *)argument);
    }
   }
}

/**
* @brief  USBH_LL_NotifyURBChange 
*         Notify URB state Change
* @param  phost: Host handle
* @retval USBH Status
*/
USBH_StatusTypeDef  USBH_LL_NotifyURBChange (USBH_HandleTypeDef *phost)
{
  osMessagePut ( phost->os_event, USBH_URB_EVENT, 0);
  return USBH_OK;
}
#endif  
/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
