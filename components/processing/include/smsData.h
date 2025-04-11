#ifndef SMSDATA_H
#define SMSDATA_H

#include <stdbool.h>  
#include <stdint.h> 

typedef struct {
    char cmdat_sms[20]; 
    char sender_phone[20];
    char sms_date[20];
    char sms_time[20];
    char imei_received[20];
    char cmd_received[20];
    char sms_flag[20];
} smsData_t;

#endif