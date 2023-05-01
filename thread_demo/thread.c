/*
 * Copyright (C) 2022 HiHope Open Source Organization .
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http:// www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#define STACK_SIZE 1024
#define OS_DELAY 100
#define OS_DELAYONE 20

osThreadId_t newThread(char *name, osThreadFunc_t func, int*arg)
{
    osThreadAttr_t attr = {
        name, 0, NULL, 0, NULL, 1024*2, osPriorityNormal, 0, 0
    };
    osThreadId_t tid = osThreadNew(func, arg, &attr);
    if (tid == NULL) {
        printf("[Thread Test] osThreadNew(%s) failed.\r\n", name);
    } else {
        printf("[Thread Test] osThreadNew(%s) success, thread id: %d.\r\n", name, tid);
    }
    return tid;
}

void threadTest(int *arg)
{
    static int count = 0;
    printf("%s\r\n", (char *)arg);
    osThreadId_t tid = osThreadGetId();
    printf("[Thread Test] threadTest osThreadGetId, thread id:%p\r\n", tid);
    while (1) {
        count++;
        printf("[Thread Test] threadTest, count: %d.\r\n", count);
        osDelay(OS_DELAYONE);
    }
}

void rtosv2_thread_main(int *arg)
{
    (void)arg;//因为arg没有使用，防止编译器报错

    osThreadId_t tid = newThread("test_thread", threadTest, "This is a test thread.");
    
    //获取名字
    const char *t_name = osThreadGetName(tid);
    printf("[Thread Main] osThreadGetName, thread name: %s.\r\n", t_name);

    //获取状态
    osThreadState_t state = osThreadGetState(tid);
    printf("[Thread Main] osThreadGetState, state :%d.\r\n", state);

    //设置指定线程的优先级
    osStatus_t status = osThreadSetPriority(tid, osPriorityNormal4);
    printf("[Thread Main] osThreadSetPriority, status: %d.\r\n", status);

    //获取指定线程的优先级
    osPriority_t pri = osThreadGetPriority(tid);
    printf("[Thread Main] osThreadGetPriority, priority: %d.\r\n", pri);

    //挂起线程
    status = osThreadSuspend(tid);
    printf("[Thread Main] osThreadSuspend, status: %d.\r\n", status);

    //恢复线程
    status = osThreadResume(tid);
    printf("[Thread Main] osThreadResume, status: %d.\r\n", status);

    //获取线程的堆栈大小
    uint32_t stacksize = osThreadGetStackSize(tid);
    printf("[Thread Main] osThreadGetStackSize, stacksize: %u.\r\n", stacksize);

    //获取线程的未使用的堆栈空间大小
    uint32_t stackspace = osThreadGetStackSpace(tid);
    printf("[Thread Main] osThreadGetStackSpace, stackspace: %u.\r\n", stackspace);

    //	获取活跃线程数
    uint32_t t_count = osThreadGetCount();
    printf("[Thread Main] osThreadGetCount, count: %u.\r\n", t_count);

    osDelay(OS_DELAY);
    
    //终止指定线程的运行
    status = osThreadTerminate(tid);
    printf("[Thread Main] osThreadTerminate, status: %d.\r\n", status);
}

static void ThreadTestTask(void)
{
    osThreadAttr_t attr;
    attr.name = "rtosv2_thread_main";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = STACK_SIZE;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)rtosv2_thread_main, NULL, &attr) == NULL) {
        printf("[ThreadTestTask] Failed to create rtosv2_thread_main!\n");
    }
}

//应用层入口函数
APP_FEATURE_INIT(ThreadTestTask);