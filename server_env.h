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

#ifndef __SERVER_ENV_H__
#define __SERVER_ENV_H__

#define USE_NEW_DEVICE_METHOD 1
/* Device ID为以十六进制数来表示的字符串，各部分组成如下:
 * |--  4字符    --|     |-- 5字符 --|        |-- 6字符 --|
 * manufacturer id  +  produce  date   +  MAC   address
 *
 * 举例, Device ID "000011A0125D0F4" 各组成部分为：
 * 1. manufacturer_id: 0x0000。
 * 2. produce_date   : 2017年10月1日。其中，0x11表示2017年 (2000+0x11)，0xA表示10月，0x01表示1日。
 * 3. MAC_address    ：25:D0:F4 (设备MAC地址后24位)。
 */
#if USE_NEW_DEVICE_METHOD
#define DEVICE_ID_MAX_LEN                (32)
#define DEVICE_ID_RETAIN                 "00000"
#define DEVICE_ID_MANUFACTURER_ID        "0004"
#define DEVICE_ID_PRODUCE_ID        	 "001"
#else
#define DEVICE_ID_MAX_LEN                (26)
#define DEVICE_ID_MANUFACTURER_ID        "001"
#define DEVICE_ID_PRODUCE_ID        	 "666"
#endif

#define DEVICE_ID_PRODUCE_DATE           "20181120"
#define WEB_PREFIX_STR                   "web_"
#define WEB_PREFIX_STR_LEN               (4)
#define WEB_MQTT_TOPIC_MAX_LEN           (DEVICE_ID_MAX_LEN + WEB_PREFIX_STR_LEN)

extern char g_device_id_str[DEVICE_ID_MAX_LEN + 1];
extern char g_web_mqtt_topic_str[WEB_MQTT_TOPIC_MAX_LEN + 1];

#define PRODUCT_BRANCH      "prod"
#define PRODUCT_ID          "278576532"
#define DEVICE_ID           g_device_id_str
#define MQTT_TOPIC_SERVER   WECHAT_APP_ID
#define MQTT_TOPIC_TOY      DEVICE_ID
#define MQTT_TOPIC_BROWSER  g_web_mqtt_topic_str
#define MQTT_MSG_VER	    "0.50"

#define TEST_SERVER 0
#define DEVELOP_SERVER 1
#define OFFICIAL_SERVER 2

#define SERVER_ENV OFFICIAL_SERVER 

#if (SERVER_ENV == TEST_SERVER)
#define WECHAT_APP_ID	"gh_6107b3cec21e"
#define MQTT_USER	"admin"
#define MQTT_PASSWORD	"password"

#define MQTT_SERVER	"test.iot.aispeech.com"
#define MQTT_PORT	"61613"

#define ALARM_WECHAT_SERVER "test.iot.aispeech.com"
#define ALARM_WECHAT_PORT "80"
#endif

#if (SERVER_ENV == DEVELOP_SERVER)
#define WECHAT_APP_ID   "gh_39c5dfcbda46"
#define MQTT_USER	"admin"
#define MQTT_PASSWORD	"password"

#define MQTT_SERVER	"120.24.75.220"
#define MQTT_PORT	"61613"

#define ALARM_WECHAT_SERVER "192.168.3.67"
#define ALARM_WECHAT_PORT "9000"
#endif

#if (SERVER_ENV == OFFICIAL_SERVER)
#define WECHAT_APP_ID	"gh_5e9c85479d21"
#define MQTT_USER	"admin"
#define MQTT_PASSWORD	"password"

#define MQTT_SERVER	"mqtt.iot.aispeech.com"
#define MQTT_PORT	"1883"

#define ALARM_WECHAT_SERVER "api.iot.aispeech.com"
#define ALARM_WECHAT_PORT "80"

//#define ALARM_WECHAT_SERVER "localhost"
//#define ALARM_WECHAT_PORT "8003"

#endif

#define SERVER_CHINESE_CFG "{\
    }"
    
#define SERVER_ENGLISH_CFG "{\
        }"

#define SERVER_DUI_CFG "{\
    }"

#define CLOUD_ASR_PARAM_OGG "{\
        \"coreProvideType\": \"cloud\",\
        \"audio\": {\
            \"audioType\": \"ogg\",\
            \"sampleRate\": 16000,\
            \"channel\": 1,\
            \"compress\":\"raw\",\
            \"sampleBytes\": 2\
        },\
        \"request\": {\
            \"coreType\": \"cn.sds\",\
            \"speechRate\":1.0,\
            \"res\": \"airobot\"\
        },\
        \"sdsExpand\":{\
            \"prevdomain\":\"\",\
            \"lastServiceType\": \"cloud\"\
        }\
    }"

#define CLOUD_ASR_PARAM_WAV "{\
                    \"coreProvideType\": \"cloud\",\
                    \"audio\": {\
                        \"audioType\": \"wav\",\
                        \"sampleRate\": 16000,\
                        \"channel\": 1,\
                        \"compress\":\"raw\",\
                        \"sampleBytes\": 2\
                    },\
                    \"request\": {\
                        \"coreType\": \"cn.sds\",\
                        \"speechRate\":1.0,\
                        \"res\": \"airobot\"\
                    },\
                    \"sdsExpand\":{\
                        \"prevdomain\":\"\",\
                        \"lastServiceType\": \"cloud\"\
                    }\
                }"

#define CLOUD_ASR_PARAM_PCM "{\
                        \"coreProvideType\": \"cloud\",\
                        \"audio\": {\
                            \"audioType\": \"pcm\",\
                            \"sampleRate\": 16000,\
                            \"channel\": 1,\
                            \"compress\":\"raw\",\
                            \"sampleBytes\": 2\
                        },\
                        \"request\": {\
                            \"coreType\": \"cn.sds\",\
                            \"speechRate\":1.0,\
                            \"res\": \"airobot\"\
                        },\
                        \"sdsExpand\":{\
                            \"prevdomain\":\"\",\
                            \"lastServiceType\": \"cloud\"\
                        }\
                    }"


#define CLOUD_ASR_PARAM_VOX "{\
            \"coreProvideType\": \"cloud\",\
            \"audio\": {\
                \"audioType\": \"adpcm\",\
                \"sampleRate\": 16000,\
                \"channel\": 1,\
                \"compress\":\"raw\",\
                \"sampleBytes\": 2\
            },\
            \"request\": {\
                \"coreType\": \"cn.sds\",\
                \"speechRate\":1.0,\
                \"res\": \"airobot\"\
            },\
            \"sdsExpand\":{\
                \"prevdomain\":\"\",\
                \"lastServiceType\": \"cloud\"\
            }\
        }"


#define SERVER_TTS_SYNTH_CFG "{\
}"

#endif
