#ifndef RNDIS_H
#define RNDIS_H

#include "usbh_core.h"



USBH_StatusTypeDef USBH_RNDIS_Send_Encapsulated_Commnad(
                            USBH_HandleTypeDef *phost,
                            uint8_t messageLen,
                            uint8_t* messageBuff);
														
USBH_StatusTypeDef USBH_RNDIS_Get_Encapsulated_Response(
                            USBH_HandleTypeDef *phost,
                            uint8_t messageLen,
                            uint8_t* messageBuff);




#endif //RNDIS_H
