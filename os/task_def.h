/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifndef __TASK_DEF_H__
#define __TASK_DEF_H__

#define configMAX_PRIORITIES 20

typedef enum {
    TASK_PRIORITY_IDLE = 0,                                 /* lowest, special for idle task */
    TASK_PRIORITY_SYSLOG,                                   /* special for syslog task */

    /* User task priority begin, please define your task priority at this interval */
    TASK_PRIORITY_LOW,                                      /* low */
    TASK_PRIORITY_BELOW_NORMAL,                             /* below normal */
    TASK_PRIORITY_NORMAL,                                   /* normal */
    TASK_PRIORITY_ABOVE_NORMAL,                             /* above normal */
    TASK_PRIORITY_HIGH,                                     /* high */
    TASK_PRIORITY_SOFT_REALTIME,                            /* soft real time */
    TASK_PRIORITY_HARD_REALTIME,                            /* hard real time */
    /* User task priority end */

    /*Be careful, the max-priority number can not be bigger than configMAX_PRIORITIES - 1, or kernel will crash!!! */
    TASK_PRIORITY_TIMER = configMAX_PRIORITIES - 1,         /* highest, special for timer task to keep time accuracy */
} task_priority_type_t;


/* for wifi net task */
#define UNIFY_NET_TASK_NAME                 "net"
#define UNIFY_NET_TASK_STACKSIZE            (1024*4) /*unit byte!*/
#define UNIFY_NET_TASK_PRIO                 TASK_PRIORITY_HIGH
#define UNIFY_NET_QUEUE_LENGTH              16

/* for wifi inband task */
#define UNIFY_INBAND_TASK_NAME              "inband"
#define UNIFY_INBAND_TASK_STACKSIZE         (1024*4) /*unit byte!*/
#define UNIFY_INBAND_TASK_PRIO              TASK_PRIORITY_HIGH
#define UNIFY_INBAND_QUEUE_LENGTH           16


/* for smart connection task */
#define UNIFY_SMTCN_TASK_NAME              "smtcn"
#define UNIFY_SMTCN_TASK_STACKSIZE         (512*4) /*unit byte!*/
#define UNIFY_SMTCN_TASK_PRIO              TASK_PRIORITY_NORMAL

/* for dhcpd task */
#define DHCPD_TASK_NAME                 "dhcpd"
#define DHCPD_TASK_STACKSIZE            (1024)
#define DHCPD_TASK_PRIO                 TASK_PRIORITY_NORMAL

/* for lwIP task */
#define TCPIP_THREAD_NAME              "lwIP"
#define TCPIP_THREAD_STACKSIZE         (512 * 4)
#define TCPIP_THREAD_PRIO              TASK_PRIORITY_HIGH

/* for iperf task */
#define IPERF_TASK_NAME                "iperf"
#define IPERF_TASK_STACKSIZE           (1200 * 4)
#define IPERF_TASK_PRIO                TASK_PRIORITY_NORMAL

/* for ping task */
#define PING_TASK_NAME                 "ping"
#define PING_TASK_STACKSIZE            (512 * 4)
#define PING_TASK_PRIO                 TASK_PRIORITY_NORMAL

/* for websocket task */
#define WEBSOCKET_THREAD_NAME           "websocket"
#define WEBSOCKET_THREAD_PRIO           TASK_PRIORITY_NORMAL
/*This definition deternines the WEBSOCKET_THREAD_STACKSIZE, 1024 * 10 if this option is enabled.*/
#ifdef MTK_WEBSOCKET_SSL_ENABLE
#define WEBSOCKET_THREAD_STACKSIZE      (1024 * 10)
#else
#define WEBSOCKET_THREAD_STACKSIZE      (1024 * 2)
#endif

/* syslog task definition */
#define SYSLOG_TASK_NAME "SYSLOG"
#if defined(MTK_PORT_SERVICE_ENABLE)
#define SYSLOG_TASK_STACKSIZE 1024
#else
#define SYSLOG_TASK_STACKSIZE 192
#endif
#define SYSLOG_TASK_PRIO TASK_PRIORITY_SYSLOG
#if (PRODUCT_VERSION == 7687) || (PRODUCT_VERSION == 7697) || (PRODUCT_VERSION == 7686) || (PRODUCT_VERSION == 7682) || (PRODUCT_VERSION == 5932) || defined(MTK_NO_PSRAM_ENABLE)
#define SYSLOG_QUEUE_LENGTH 8
#elif (PRODUCT_VERSION == 2523)
#define SYSLOG_QUEUE_LENGTH 512
#endif

/* for os utilization task */
#if defined(MTK_OS_CPU_UTILIZATION_ENABLE)
#define MTK_OS_CPU_UTILIZATION_TASK_NAME "CPU"
#define MTK_OS_CPU_UTILIZATION_STACKSIZE 1024
#define MTK_OS_CPU_UTILIZATION_PRIO      TASK_PRIORITY_SOFT_REALTIME
#endif

/* part_2: Application and customer tasks configure information */
/* currently, only UI task and tasks to show example project function which define in apps/project/src/main.c */

/* for create cli */
#if defined(MTK_MINICLI_ENABLE)
#define MINICLI_TASK_NAME               "cli"
#define MINICLI_TASK_STACKSIZE          (4096)
#define MINICLI_TASK_PRIO               TASK_PRIORITY_SOFT_REALTIME
#endif

/* for set n9log cli task */
#define N9LOG_TASK_NAME                 "n9log"
#define N9LOG_TASK_STACKSIZE            (512)
#define MAX_N9LOG_SIZE                  (1024)
#define N9LOG_TASK_PRIO                 TASK_PRIORITY_LOW

/* ATCI task definition */
#define ATCI_TASK_NAME              "ATCI"
#ifdef MTK_AUDIO_TUNING_ENABLED
#define ATCI_TASK_STACKSIZE         (5000*4) /*unit byte!*/
#else
#define ATCI_TASK_STACKSIZE         (1024*4) /*unit byte!*/
#endif
#define ATCI_TASK_PRIO              TASK_PRIORITY_NORMAL

/* for I2S  task */
#define I2S_TASK_NAME           "i2s_task"
#define I2S_TASK_STACKSIZE      1024
#define I2S_TASK_PRIO           TASK_PRIORITY_SOFT_REALTIME
#define I2S_TASK_QUEUE_LENGTH   20


/* for Application task */
#define MP3_PLAY_EXAMPLE_TASK_NAME       "mp3_codec_demo"
#define MP3_PLAY_EXAMPLE_TASK_STACKSIZE  1024
#define MP3_PLAY_EXAMPLE_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for MP3 codec task */
#define MP3_CODEC_TASK_NAME       "mp3_codec_task"
#define MP3_CODEC_TASK_STACKSIZE  1024
#define MP3_CODEC_TASK_PRIO       TASK_PRIORITY_SOFT_REALTIME
#define MP3_CODEC_QUEUE_LENGTH 20

/* for audio codec task */
#define AUDIO_CODEC_TASK_NAME       "audio_codec_task"
#define AUDIO_CODEC_TASK_STACKSIZE  1024
#define AUDIO_CODEC_TASK_PRIO       TASK_PRIORITY_NORMAL
#define AUDIO_CODEC_QUEUE_LENGTH    20

/* for get and write audio file by http task */
#define HTTP_GET_WRITE_FILE_TASK_NAME       "httpclient_get_write_file_task"
#define HTTP_GET_WRITE_FILE_TASK_STACKSIZE  10240
#define HTTP_GET_WRITE_FILE_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for Wifi network config task */
#define NETWORK_CONFIG_TASK_NAME        "network config"
#define NETWORK_CONFIG_TASK_STACKSIZE   (4*1024)
#define NETWORK_CONFIG_TASK_PRIO        TASK_PRIORITY_NORMAL

/* for Wifi network config prompt task */
#define NETWORK_CONFIG_PROMPT_TASK_NAME        "network_config_prompt_task_main"
#define NETWORK_CONFIG_PROMPT_TASK_STACKSIZE   (4096)
#define NETWORK_CONFIG_PROMPT_TASK_PRIO        TASK_PRIORITY_NORMAL

/* for multi smart connection */
#define MSC_TASK_NAME                     "msc"
#define MSC_TASK_STACKSIZE                (512*4) /*unit byte!*/
#define MSC_TASK_PRIO                     TASK_PRIORITY_NORMAL

/* for player local play manage task */
#define PLAYER_LOCAL_PLAY_MANAGE_TASK_NAME       "player_local_play_manage_task"
#define PLAYER_LOCAL_PLAY_MANAGE_TASK_STACKSIZE  4096
#define PLAYER_LOCAL_PLAY_MANAGE_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for network status check task */
#define STARTUP_INTERACTIVE_PROCESS_TASK_NAME       "startup_interactive_process_task"
#define STARTUP_INTERACTIVE_PROCESS_TASK_STACKSIZE  4096
#define STARTUP_INTERACTIVE_PROCESS_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for airkiss device discover task */
#define AIRKISS_LAN_DEVICE_DISCOVER_TASK_NAME       "airkiss_lan_device_discover_main"
#define AIRKISS_LAN_DEVICE_DISCOVER_TASK_STACKSIZE  2048
#define AIRKISS_LAN_DEVICE_DISCOVER_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for battery capacity monitor task */
#define BATTERY_CAPACITY_MONITOR_TASK_NAME       "battery_capacity_monitor_task"
#define BATTERY_CAPACITY_MONITOR_TASK_STACKSIZE  2048
#define BATTERY_CAPACITY_MONITOR_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for low power process task */
#define LOW_POWER_PROCESS_TASK_NAME       "long_unuse_monitor_task"
#define LOW_POWER_PROCESS_TASK_STACKSIZE  2048
#define LOW_POWER_PROCESS_TASK_PRIO       TASK_PRIORITY_NORMAL

/* for low power enter task */
#define LOW_POWER_ENTER_TASK_NAME       "low_power_enter_task"
#define LOW_POWER_ENTER_TASK_STACKSIZE  2048
#define LOW_POWER_ENTER_TASK_PRIO       TASK_PRIORITY_NORMAL

#define RTC_IRQ_STAT_POLLING_TASK_NAME       "rtc_irq_stat_polling_task_main"
#define RTC_IRQ_STAT_POLLING_TASK_STACKSIZE  1024
#define RTC_IRQ_STAT_POLLING_TASK_PRIO       TASK_PRIORITY_NORMAL

#define NETWORK_STATUS_MONITOR_TASK_NAME       "network_status_monitor_task_main"
#define NETWORK_STATUS_MONITOR_TASK_STACKSIZE  1024
#define NETWORK_STATUS_MONITOR_TASK_PRIO       TASK_PRIORITY_NORMAL

#define TF_CARD_DETECT_TASK_NAME       "tf_card_detect_task"
#define TF_CARD_DETECT_TASK_STACKSIZE  2048
#define TF_CARD_DETECT_TASK_PRIO       TASK_PRIORITY_NORMAL

#define NETWORK_DECT_TASK_NAME       "network_dect_task"
#define NETWORK_DECT_TASK_STACKSIZE  2048
#define NETWORK_DECT_TASK_PRIO       TASK_PRIORITY_NORMAL
#define MQTT_EVENT_PROCESS_TASK_NAME       "task_mqtt_event_process_main"
#define MQTT_EVENT_PROCESS_TASK_STACKSIZE  2048
#define MQTT_EVENT_PROCESS_TASK_PRIO       TASK_PRIORITY_NORMAL

#define MQTT_MSG_PROCESS_TASK_NAME       "task_mqtt_msg_process_main"
#define MQTT_MSG_PROCESS_TASK_STACKSIZE  2048
#define MQTT_MSG_PROCESS_TASK_PRIO       TASK_PRIORITY_NORMAL

#define MQTT_RECV_MSG_TASK_NAME       "task_mqtt_recv_msg_main"
#define MQTT_RECV_MSG_TASK_STACKSIZE  2048
#define MQTT_RECV_MSG_TASK_PRIO       TASK_PRIORITY_HIGH

#define MQTT_PERIODIC_MSG_SEND_TASK_NAME       "task_mqtt_periodic_msg_send_main"
#define MQTT_PERIODIC_MSG_SEND_TASK_STACKSIZE  2048
#define MQTT_PERIODIC_MSG_SEND_TASK_PRIO       TASK_PRIORITY_NORMAL

#endif