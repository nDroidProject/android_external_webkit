/*
 * Copyright 2007, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JavaSharedClient.h"
#define LOG_TAG "JavaSharedClient"
#include "utils/Log.h"
#include "SkDeque.h"
#include "SkThread.h"

namespace android {
    void AndroidSignalServiceFuncPtrQueue();

    TimerClient* JavaSharedClient::GetTimerClient()
    {
        //LOG_ASSERT(gTimerClient != NULL, "gTimerClient not initialized!!!");
        return gTimerClient;
    }

    CookieClient* JavaSharedClient::GetCookieClient()
    {
        //LOG_ASSERT(gCookieClient != NULL, "gCookieClient not initialized!!!");
        return gCookieClient;
    }

    KeyGeneratorClient* JavaSharedClient::GetKeyGeneratorClient()
    {
        //LOG_ASSERT(gKeyGeneratorClient != NULL, "gKeyGeneratorClient not initialized!!!");
        return gKeyGeneratorClient;
    }

    void JavaSharedClient::SetTimerClient(TimerClient* client)
    {
        //LOG_ASSERT(gTimerClient == NULL || client == NULL, "gTimerClient already set, aborting...");
        gTimerClient = client;
    }

    void JavaSharedClient::SetCookieClient(CookieClient* client)
    {
        //LOG_ASSERT(gCookieClient == NULL || client == NULL, "gCookieClient already set, aborting...");
        gCookieClient = client;
    }

    void JavaSharedClient::SetKeyGeneratorClient(KeyGeneratorClient* client)
    {
        //LOG_ASSERT(gKeyGeneratorClient == NULL || client == NULL, "gKeyGeneratorClient already set, aborting...");
        gKeyGeneratorClient = client;
    }

    TimerClient*    JavaSharedClient::gTimerClient = NULL;
    CookieClient*   JavaSharedClient::gCookieClient = NULL;
    KeyGeneratorClient* JavaSharedClient::gKeyGeneratorClient = NULL;

    ///////////////////////////////////////////////////////////////////////////
    
    struct FuncPtrRec {
        void (*fProc)(void* payload);
        void* fPayload;
    };
    
    static SkMutex gFuncPtrQMutex;
    static SkDeque gFuncPtrQ(sizeof(FuncPtrRec));

    void JavaSharedClient::EnqueueFunctionPtr(void (*proc)(void* payload),
                                              void* payload)
    {
        gFuncPtrQMutex.acquire();

        FuncPtrRec* rec = (FuncPtrRec*)gFuncPtrQ.push_back();
        rec->fProc = proc;
        rec->fPayload = payload;
        
        gFuncPtrQMutex.release();
        
        android::AndroidSignalServiceFuncPtrQueue();
    }

    void JavaSharedClient::ServiceFunctionPtrQueue()
    {
        for (;;) {
            void (*proc)(void*);
            void* payload;
            const FuncPtrRec* rec;
            
            // we have to copy the proc/payload (if present). we do this so we
            // don't call the proc inside the mutex (possible deadlock!)
            gFuncPtrQMutex.acquire();
            rec = (const FuncPtrRec*)gFuncPtrQ.front();
            if (NULL != rec) {
                proc = rec->fProc;
                payload = rec->fPayload;
                gFuncPtrQ.pop_front();
            }
            gFuncPtrQMutex.release();
            
            if (NULL == rec) {
                break;
            }
            proc(payload);
        }
    }
}
