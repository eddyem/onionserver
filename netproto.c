/*
 * This file is part of the Onion_test project.
 * Copyright 2020 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "netproto.h"

#include <onion/types_internal.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <usefull_macros.h>

const char NormClose[2] = {0x10, 0x00}; // Normal close

typedef struct list_{
    onion_websocket *ws;        // websocket
    pthread_mutex_t mutex;      // writing mutex
    struct list_ *next;         // next in list
} WSlist;

// total amount of connected clients
static int Nconnected = 0;
// list of websockets
static WSlist *wslist = NULL;
// mutex for main proc
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// get the latest WS in list
static WSlist *getlast(){
    if(!wslist) return NULL;
    WSlist *l = wslist;
    while(l->next) l = l->next;
    return l;
}

/**
 * @brief runMainProc - main data process
 * @param data - unused
 * @return
 */
void *runMainProc(_U_ void *data){
    char message[256];
    while(1){
        int changed = 0;
        pthread_mutex_lock(&mutex);
        time_t t = time(NULL);
        struct tm *tmp = localtime(&t);
        if(tmp){
            strftime(message, 256, "%d/%m/%y, %T", tmp);
            changed = 1;
        }
        pthread_mutex_unlock(&mutex);
        if(changed) send_all_WS(message);
        sleep(1);
    }
    return NULL;
}

/**
 * @brief process_WS_signal - get signal from websocket and do something with it
 * @param ws (i)     - websocket
 * @param signal (i) - command received
 */
void process_WS_signal(onion_websocket *ws, char *signal){
    char buf[256];
    char *eq = strchr(signal, '=');
    if(eq){
        *eq++ = 0;
        snprintf(buf, 256, "parameter: '%s', its value: '%s'", signal, eq);
        send_one_WS(ws, buf);
    }else{
        snprintf(buf, 256, "Echo: %s", signal);
        send_one_WS(ws, buf);
    }
}

/**
 * @brief register_WS - add recently opened websocket to common list
 * @param ws - websocket
 * @return 0 if all OK
 */
int register_WS(onion_websocket *ws){
    if(Nconnected >= MAX_WSCLIENTS){ // no more connected
        onion_websocket_printf(ws, "Maximum of %d clientsalready reached", MAX_WSCLIENTS);
        onion_websocket_close(ws, NormClose);
        return 1;
    }
    DBG("Add NEW websocket\n");
    WSlist **curr = NULL;
    if(!wslist){ // the first element
        curr = &wslist;
    }else{
        WSlist *last = getlast();
        curr = &last->next;
    }
    WSlist *tmp = MALLOC(WSlist, 1);
    tmp->ws = ws;
    pthread_mutex_init(&tmp->mutex, NULL);
    *curr = tmp;
    ++Nconnected;
    return 0;
}

/**
 * @brief send_all_WS - send data to all opened websockets
 * @param data (i) - data to send
 */
void send_all_WS(char *data){
    if(strlen(data) == 0) return; // zero length
    WSlist *l = wslist;
    if(!l){
        DBG("list is empty");
        return;
    }
    //int cnt = 0;
    while(l){
       //DBG("try to send");
        if(0 == pthread_mutex_lock(&l->mutex)){
            if(onion_websocket_printf(l->ws, "%s", data) <= 0){ // dead websocket? remove it from list
                DBG("Error printing - check for dead");
                pthread_mutex_unlock(&l->mutex);
                unregister_WS(l->ws);
            }//else ++cnt;
            pthread_mutex_unlock(&l->mutex);
        }else DBG("CANT LOCK");
        l = l->next;
    }
    //DBG("Send message %s to %d clients", data, cnt);
}

void send_one_WS(onion_websocket *ws, char *data){
    if(strlen(data) == 0) return; // zero length
    WSlist *l = wslist;
    if(!l) return;
    while(l){
        if(l->ws == ws){
            if(0 == pthread_mutex_lock(&l->mutex)){
                if(onion_websocket_printf(l->ws, "%s", data) <= 0){ // dead websocket? remove it from list
                    DBG("Error printing - check for dead");
                    pthread_mutex_unlock(&l->mutex);
                    unregister_WS(ws);
                }
                pthread_mutex_unlock(&l->mutex);
            }else DBG("CANT LOCK");
            break;
        }
        l = l->next;
    }
}

// remove ws from list
void unregister_WS(onion_websocket *ws){
    WSlist *l = wslist, *prev = NULL;
    while(l){
        if(l->ws == ws){
            pthread_mutex_lock(&l->mutex);
            WSlist *next = l->next;
            if(l == wslist) wslist = next; // delete the first element
            else if(prev) prev->next = next;
            pthread_mutex_destroy(&l->mutex);
            FREE(l);
            --Nconnected;
            return;
        }
        prev = l;
        l = l->next;
    }
}

// find bad websockets and remove them from list
void cleanup_WS(){
    WSlist *l = wslist, *prev = NULL;
    while(l){
        if(onion_websocket_get_opcode(l->ws) == OWS_CONNECTION_CLOSE){ // closed
            DBG("Found dead WS, remove it\n");
            pthread_mutex_lock(&l->mutex);
            WSlist *next = l->next;
            if(l == wslist) wslist = next; // delete the first element
            else if(prev) prev->next = next;
            pthread_mutex_destroy(&l->mutex);
            FREE(l);
            --Nconnected;
            l = next;
            continue;
        }
        prev = l;
        l = l->next;
    }
}
