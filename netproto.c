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

typedef struct list_{
    onion_websocket *ws;
    struct list_ *next;
    struct list_ *prev;
    struct list_ *last;
} WSlist;

static WSlist *wslist = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static char message[256];

/**
 * @brief runMainProc - main data process
 * @param data - unused
 * @return
 */
void *runMainProc(_U_ void *data){
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
    char *eq = strchr(signal, '=');
    if(eq){
        *eq++ = 0;
        onion_websocket_printf(ws, "parameter: '%s', its value: '%s'", signal, eq);
    }else{
        onion_websocket_printf(ws, "Echo: %s", signal);
    }
}

/**
 * @brief register_WS - add recently opened websocket to common list
 * @param ws - websocket
 */
void register_WS(onion_websocket *ws){
    green("Add NEW websocket\n");
    if(!wslist){
        wslist = MALLOC(WSlist, 1);
        wslist->ws = ws;
        wslist->last = wslist;
    }else{
        wslist->last->next = MALLOC(WSlist, 1);
        wslist->last->next->ws = ws;
        wslist->last->next->prev = wslist->last;
        wslist->last = wslist->last->next;
    }
}

/**
 * @brief send_all_WS - send data to all opened websockets
 * @param data (i) - data to send
 */
void send_all_WS(char *data){
    if(strlen(data) == 0) return; // zero length
    WSlist *l = wslist;
    int cnt = 0;
    unregister_WS(); // check for dead ws
    while(l){
        DBG("try to send");
        if(onion_websocket_printf(l->ws, "%s", data) <= 0){ // dead websocket? remove it from list
            DBG("Error printing - check for dead");
            unregister_WS();
        }else ++cnt;
        l = l->next;
    }
    DBG("Send message %s to %d clients", data, cnt);
}

void unregister_WS(){
    WSlist *l = wslist;
    while(l){
        if(l->ws->req->connection.fd < 0){
            red("Found dead WS, remove it\n");
            if(l->prev){
                //DBG("l->prev->next = l->next");
                l->prev->next = l->next;
            }
            if(l->next){
                //DBG("l->next->prev = l->prev");
                l->next->prev = l->prev;
            }else{
                //DBG("wslist->last = l->prev");
                wslist->last = l->prev; // we should delete last element
            }
            WSlist *nxt = l->next;
            FREE(l);
            l = nxt;
            continue;
        }
        l = l->next;
    }
}
