/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IOT_CONFIG_H
#define IOT_CONFIG_H

// <CONFIG THE LOG
/* if you need the iot log for the development , please enable it, else please comment it */
#define CONFIG_LINKLOG_ENABLE   1

// < CONFIG THE WIFI
/* Please modify the ssid and pwd for the own */
#define CONFIG_AP_SSID  "realme GT" // WIFI SSID
#define CONFIG_AP_PWD   "u12138138" // WIFI PWD
/* Tencent iot Cloud user ID , password */
#define CONFIG_USER_ID    "GNJOUT1LQ6Board1;12010126;a7be7;1691596800"
#define CONFIG_USER_PWD   "8325ba087bb7fa0219d80fcf7743811186bb4e0a36b44ec8c8cec638750c0cfc;hmacsha256"
#define CN_CLIENTID     "GNJOUT1LQ6Board1" // Tencent cloud ClientID format: Product ID + device name
#endif