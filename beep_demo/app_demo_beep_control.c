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

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_watchdog.h"
#include "iot_pwm.h"
#include "hi_time.h"


#define LED_INTERVAL_TIME_US 300000
#define LED_TASK_STACK_SIZE 512
#define LED_TASK_PRIO 25

static int g_beepState = 1;
static int g_iState = 0;
static int duty = 10;
#define IOT_PWM_PORT_PWM0   0
#define IOT_PWM_BEEP        9
#define IOT_GPIO_KEY        8

//控制舵机，Pwm占空比2.5`12.5，50Hz
static void *PWMBeepTask(const char *arg)
{
    return;
    while (1) {
        static int i=2, dir=1;
        // if (g_beepState) {
        //     IoTPwmStart(IOT_PWM_PORT_PWM0, 80, 4000); /* 占空比50 / 4000,频率4000 */
        // } else {
        //     IoTPwmStart(IOT_PWM_PORT_PWM0, 80, 4000);//IoTPwmStop(IOT_PWM_PORT_PWM0);
        // }
        // if (g_iState == 0xffff) {
        //     g_iState = 0;
        //     break;
        // }
        IoTPwmStart(IOT_PWM_PORT_PWM0, i, 4000);//GPIO9
        IoTPwmStart(2, 80, 4000); //GPIO2
        IoTPwmStart(4, i, 50);//GPIO1

        i += dir;
        if(i>=13) {dir *=-1;}
        if(i<=2) {dir *=-1;}

        //休眠1秒
        TaskMsleep(1000);
        printf("occupation %d\n",i);
    }


}

static void OnButtonPressed(const char *arg)
{
    (void) arg;
    g_beepState = !g_beepState;
    printf("PRESS_KEY!\n");
    g_iState++;
}

//GPIO模拟PWM波
#define FRE 50
#define IOT_GPIO_IDX_10 10

static void *SERVOCntrolDemo(const char *arg)
{
    static int i = 0;
    printf("LedTask start\r\n");
    while(1){
        i++;
        int interval = 1000000 / FRE;
        //高电平
        int time =(int)interval * duty / 100;
        IoTGpioSetDir(IOT_GPIO_IDX_10, IOT_GPIO_DIR_OUT);
        IoTGpioSetOutputVal(IOT_GPIO_IDX_10, IOT_GPIO_VALUE1);
        //TaskMsleep(2);
        hi_udelay(time);
        //低电平
        time = interval - time;
        IoTGpioSetOutputVal(IOT_GPIO_IDX_10, IOT_GPIO_VALUE0);
        //TaskMsleep(18);
        hi_udelay(time);
        if (i >= 25) {
            i = 0;
            printf("hi_delay LedTask running,interval:%d, high_time:%d\r\n", interval, interval-time);
        }
    }
}

static void *angle_control(const char *arg)
{
    static int i = 3, dir = 1;
    printf("angleTask start\r\n");
    while(1){
        i+=dir;
        if(i>=13) {dir *=-1;}
        if(i<=2) {dir *=-1;}
        duty = i;
        printf("angle %d, duty %d\n",i,duty);
        TaskMsleep(500);
    }
}

static void StartPWMBeepTask(void)
{
    osThreadAttr_t attr;

    IoTGpioInit(IOT_GPIO_KEY);
    IoSetFunc(IOT_GPIO_KEY, 0);
    IoTGpioSetDir(IOT_GPIO_KEY, IOT_GPIO_DIR_IN);
    IoSetPull(IOT_GPIO_KEY, IOT_IO_PULL_UP);
    IoTGpioRegisterIsrFunc(IOT_GPIO_KEY, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, OnButtonPressed, NULL);
    
    IoTGpioInit(IOT_PWM_BEEP);
    IoSetFunc(IOT_PWM_BEEP, 5); /* 设置IO5的功能 */
    IoTGpioSetDir(IOT_PWM_BEEP, IOT_GPIO_DIR_OUT);
    IoTPwmInit(IOT_PWM_PORT_PWM0);

    IoTGpioInit(IOT_IO_NAME_GPIO_2);
    IoSetFunc(IOT_IO_NAME_GPIO_2, IOT_IO_FUNC_GPIO_2_PWM2_OUT);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_2, IOT_GPIO_DIR_OUT);
    IoTPwmInit( 2 );

    IoTGpioInit(IOT_IO_NAME_GPIO_1);
    IoSetFunc(IOT_IO_NAME_GPIO_1, IOT_IO_FUNC_GPIO_1_PWM4_OUT);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_1, IOT_GPIO_DIR_OUT);
    IoTPwmInit( 4 );

    IoTWatchDogDisable();

    attr.name = "PWMBeepTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024; /* 堆栈大小为1024 */
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)PWMBeepTask, NULL, &attr) == NULL) {
        printf("[StartPWMBeepTask] Failed to create PWMBeepTask!\n");
    }

    IoTGpioInit(IOT_GPIO_IDX_10);
    // 设置GPIO为输出方向
    IoTGpioSetDir(IOT_GPIO_IDX_10, IOT_GPIO_DIR_OUT);

    attr.name = "SERVOCntrolDemo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024; /* 堆栈大小为1024 */
    attr.priority = osPriorityNormal;
    // 报错
    if (osThreadNew((osThreadFunc_t)SERVOCntrolDemo, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create LedTask!\n");
    }

    attr.name = "ANGLE_Cntrol";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024; /* 堆栈大小为1024 */
    attr.priority = osPriorityNormal;
    // 报错 
    if (osThreadNew((osThreadFunc_t)angle_control, NULL, &attr) == NULL) {
        printf("[LedExample] Failed to create angleTask!\n");
    }
}

APP_FEATURE_INIT(StartPWMBeepTask);