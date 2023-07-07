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
#include <stdio.h>
#include <unistd.h>
#include <hi_task.h>
#include <string.h>
#include <hi_wifi_api.h>
#include <hi_mux.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_pwm.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"

/* attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
#define ONE_SECOND                          (1000)
/* oc request id */
#define CN_COMMADN_INDEX                    "commands/request_id="
#define WECHAT_SUBSCRIBE_LIGHT              "light"
#define WECHAT_SUBSCRIBE_LIGHT_ON_STATE     "1"
#define WECHAT_SUBSCRIBE_LIGHT_OFF_STATE    "0"
#define WECHAT_SUBSCRIBE_SERVO             "servo"

static int servo_angle[3] = {0, 0, 0};

int g_ligthStatus = -1;
typedef void (*FnMsgCallBack)(hi_gpio_value val);

typedef struct FunctionCallback {
    hi_bool  stop;
    hi_u32 conLost;
    hi_u32 queueID;
    hi_u32 iotTaskID;
    FnMsgCallBack    msgCallBack;
}FunctionCallback;
FunctionCallback g_functinoCallback;

/* CPU Sleep time Set */
unsigned int TaskMsleep(unsigned int ms)
{
    if (ms <= 0) {
        return HI_ERR_FAILURE;
    }
    return hi_sleep((hi_u32)ms);
}

static void DeviceConfigInit(hi_gpio_value val)
{//与PWM0的GPIO口冲突
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_9, HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(HI_GPIO_IDX_9, val);
}

static int  DeviceMsgCallback(FnMsgCallBack msgCallBack)
{
    g_functinoCallback.msgCallBack = msgCallBack;
    return 0;
}

static void wechatControlDeviceMsg(hi_gpio_value val)
{
    DeviceConfigInit(val);
}

// < this is the callback function, set to the mqtt, and if any messages come, it will be called
// < The payload here is the json string
static void DemoMsgRcvCallBack(int qos, const char *topic, const char *payload)
{
    IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
    /* 云端下发命令后，板端的操作处理 */
    //找对应的字符串
    if (strstr(payload, WECHAT_SUBSCRIBE_LIGHT) != NULL) {
        if (strstr(payload, WECHAT_SUBSCRIBE_LIGHT_OFF_STATE) != NULL) {
            //wechatControlDeviceMsg(HI_GPIO_VALUE1);
            g_ligthStatus = HI_FALSE;
            printf("light off\n");
        } 
        else {
            //wechatControlDeviceMsg(HI_GPIO_VALUE0);
            g_ligthStatus = HI_TRUE;
            printf("light on\n");
        }
    }

    //寻找servo对应的字符串
    char *p= strstr(payload, WECHAT_SUBSCRIBE_SERVO);
    if(p != NULL)
    {
        p += strlen(WECHAT_SUBSCRIBE_SERVO)+1;
        char num[4];
        num[3] = '\0';
        strncpy (num, p, 3);
        servo_angle[0] = atoi(num);
        strncpy (num, p+3, 3);
        servo_angle[1] = atoi(num);
        strncpy (num, p+6, 3);
        servo_angle[2] = atoi(num);
        printf("servo0:%d servo1:%d servo2:%d\n", servo_angle[0], servo_angle[1], servo_angle[2]);
    }


    return HI_NULL;
}

/* publish sample */
hi_void IotPublishSample(void)
{
    /* reported attribute */
    WeChatProfile weChatProfile = {
        .subscribeType = "type",
        .status.subState = "state",
        .status.subReport = "reported",
        .status.reportVersion = "version",
        .status.Token = "clientToken",
        /* report motor */
        .reportAction.subDeviceActionMotor = "motor",
        .reportAction.motorActionStatus = 0, /* 0 : motor off */
        /* report temperature */
        .reportAction.subDeviceActionTemperature = "temperature",
        .reportAction.temperatureData = 30, /* 30 :temperature data */
        /* report humidity */
        .reportAction.subDeviceActionHumidity = "humidity",
        .reportAction.humidityActionData = 70, /* humidity data */
        /* report light_intensity */
        .reportAction.subDeviceActionLightIntensity = "light_intensity",
        .reportAction.lightIntensityActionData = 60, /* 60 : light_intensity */
    };

    /* report light */
    if (g_ligthStatus == HI_TRUE) {
        weChatProfile.reportAction.subDeviceActionLight = "light";
        weChatProfile.reportAction.lightActionStatus = 1; /* 1: light on */
    } else if (g_ligthStatus == HI_FALSE) {
        weChatProfile.reportAction.subDeviceActionLight = "light";
        weChatProfile.reportAction.lightActionStatus = 0; /* 0: light off */
    } else {
        weChatProfile.reportAction.subDeviceActionLight = "light";
        weChatProfile.reportAction.lightActionStatus = 0; /* 0: light off */
    }
    /* profile report */
    IoTProfilePropertyReport(CONFIG_USER_ID, &weChatProfile);
}

// < this is the demo main task entry,here we will set the wifi/cjson/mqtt ready and
// < wait if any work to do in the while
static hi_void *DemoEntry(const char *arg)
{
    WifiStaReadyWait();
    cJsonInit();
    IoTMain();
    /* 云端下发回调 */
    IoTSetMsgCallback(DemoMsgRcvCallBack);
    /* 主动上报 */
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
    while (1) {
        /* 用户可以在这调用发布函数进行发布，需要用户自己写调用函数 */
        IotPublishSample(); // 发布例程
#endif
        TaskMsleep(ONE_SECOND);
    }
    return NULL;
}

// < This is the demo entry, we create a task here,
// and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE  0x1000
#define CN_IOT_TASK_PRIOR 25
#define CN_IOT_TASK_NAME "IOTDEMO"

// < here we define the servo control pins
#define SERVO_TASK_STACKSIZE  1024

//servo0-pwm0
#define servo0_PWMport 0
#define servo0_GPIO 9
//servo1-pwm1
#define servo1_PWMport 4
#define servo1_GPIO 1
//servo2-pwm2
#define servo2_PWMport 2
#define servo2_GPIO 2
#define FREQUENCY 50

//servo control function
static void *Servo_Task(const char *arg)
{
    unsigned short servo_occupation[3] = {0, 0, 0};
    for (int i = 0; i < 3; i++) {
        servo_occupation[i] = servo_angle[i] * 1000 / 180 + 500;
    }
    while (1) {
        // IoTPwmStart(servo0_PWMport, servo_occupation[0], FREQUENCY);
        // IoTPwmStart(servo1_PWMport, servo_occupation[1], FREQUENCY);
        // IoTPwmStart(servo2_PWMport, servo_occupation[2], FREQUENCY);
        IoTPwmStart(servo0_PWMport, 80, FREQUENCY);
        IoTPwmStart(servo1_PWMport, 80, FREQUENCY);
        IoTPwmStart(servo2_PWMport, 80, FREQUENCY);
        TaskMsleep(ONE_SECOND);
    }
    return NULL;
}

static void AppDemoIot(void)
{
    osThreadAttr_t attr;
    IoTWatchDogDisable();

    attr.name = "IOTDEMO";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CN_IOT_TASK_STACKSIZE;
    attr.priority = CN_IOT_TASK_PRIOR;

    if (osThreadNew((osThreadFunc_t)DemoEntry, NULL, &attr) == NULL) {
        printf("[mqtt] Falied to create IOTDEMO!\n");
    }

    // //GPIO INIT
    // IoTGpioInit(servo0_GPIO);
    // IoTGpioInit(servo1_GPIO);
    // IoTGpioInit(servo2_GPIO);
    // IoSetFunc(servo0_GPIO, 5); /* 设置IO5的功能 */
    // IoSetFunc(servo1_GPIO, IOT_IO_FUNC_GPIO_10_PWM1_OUT);
    // IoSetFunc(servo2_GPIO, IOT_IO_FUNC_GPIO_2_PWM2_OUT);
    // IoTGpioSetDir(servo0_GPIO, IOT_GPIO_DIR_OUT);
    // IoTGpioSetDir(servo1_GPIO, IOT_GPIO_DIR_OUT);
    // IoTGpioSetDir(servo2_GPIO, IOT_GPIO_DIR_OUT);
    // IoTPwmInit(servo0_PWMport);
    // IoTPwmInit(servo1_PWMport);
    // IoTPwmInit(servo2_PWMport);

    // < here we create a task to control the servo
    attr.name = "servo_control";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = SERVO_TASK_STACKSIZE;
    attr.priority = osPriorityNormal;
    if (osThreadNew((osThreadFunc_t)Servo_Task, NULL, &attr) == NULL) {
        printf("[servo] Falied to create servo_control!\n");
    }


}

SYS_RUN(AppDemoIot);