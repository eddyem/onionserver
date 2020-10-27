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

#include "auth.h"
#include "websockets.h"

#include <errno.h>
#include <onion/dict.h>
#include <onion/log.h>
#include <stdio.h>
#include <string.h>
#include <usefull_macros.h>

#define BUFLEN 255

// bit-fields of `data` field (websocket_cont)
#define WS_FLAG_NOTAUTHORIZED   1
typedef struct{
    uint64_t UAhash;
    uint64_t IPhash;
    uint64_t flags;
} WSdata;

// https://stackoverflow.com/a/57960443/1965803
static uint64_t MurmurOAAT64(const char *key){
    uint64_t h = 525201411107845655ull;
    for (;*key;++key){
        h ^= (uint64_t)*key;
        h *= 0x5bd1e9955bd1e995;
        h ^= h >> 47;
    }
    return h;
}

static onion_connection_status websocket_cont(void *data, onion_websocket *ws, ssize_t dlen){
    FNAME();
    WSdata *wsdata = (WSdata*)data;
    char tmp[BUFLEN+1];
    if(dlen > BUFLEN) dlen = BUFLEN;

    int len = onion_websocket_read(ws, tmp, dlen);
    if(len <= 0){
        ONION_ERROR("Error reading data: %d: %s (%d)", errno, strerror(errno), dlen);
        return OCS_NEED_MORE_DATA;
    }
    tmp[len] = 0;
    //ONION_INFO("Read from websocket: %s (len=%d)", tmp, len);
    DBG("WS: got %s", tmp);
    if(wsdata->flags & WS_FLAG_NOTAUTHORIZED){ // not authorized over websocket
        sessinfo *session = NULL;
        if(strncmp(tmp, "Akey=", 5) == 0){ // got authorized key - check it
            char *key = tmp + 5;
            session = getSession(key);
        }
        if(!session){
            onion_websocket_printf(ws, AUTH_ANS_NEEDAUTH);
            WARNX("Wrong websocket session ID");
            return OCS_FORBIDDEN;
        }
        //
        onion_dict *json = onion_dict_from_json(session->data);
        freeSessInfo(&session);
        if(json){
            uint64_t UAhash = MurmurOAAT64(onion_dict_get(json, "User-Agent"));
            uint64_t IPhash = MurmurOAAT64(onion_dict_get(json, "User-IP"));
            if(wsdata->IPhash != IPhash || wsdata->UAhash != UAhash){
                onion_websocket_printf(ws, AUTH_ANS_WRONGIP);
                WARNX("Websocket IP/UA are wrong");
                return OCS_FORBIDDEN;
            }
            red("WSdata checked!\n");
            onion_dict_free(json);
        }else{
            onion_websocket_printf(ws, AUTH_ANS_NOUSERDATA);
            WARNX("No user IP and/or UA in database");
            return OCS_FORBIDDEN;
        }
        wsdata->flags &= ~WS_FLAG_NOTAUTHORIZED; // clear non-authorized flag
        return OCS_NEED_MORE_DATA;
    }
    char *eq = strchr(tmp, '=');
    if(eq){
        *eq++ = 0;
        onion_websocket_printf(ws, "parameter: '%s', its value: '%s'", tmp, eq);
    }else{
        onion_websocket_printf(ws, "Echo: %s", tmp);
    }
    return OCS_NEED_MORE_DATA;
}

onion_connection_status websocket_run(_U_ void *data, onion_request *req, onion_response *res){
    FNAME();
    onion_websocket *ws = onion_websocket_new(req, res);
    if (!ws){
        DBG("Processed");
        return OCS_PROCESSED;
    }
    DBG("WS ready");
    const char *host = onion_request_get_client_description(req);
    const char *UA = onion_request_get_header(req, "User-Agent");
    green("Got WS connection from %s (UA: %s)\n", host, UA);
    WSdata *wsdata = calloc(1, sizeof(WSdata));
    wsdata->flags = WS_FLAG_NOTAUTHORIZED;
    wsdata->IPhash = MurmurOAAT64(host);
    wsdata->UAhash = MurmurOAAT64(UA);
    onion_websocket_set_userdata(ws, (void*)wsdata, free);
    onion_websocket_set_callback(ws, websocket_cont);
    return OCS_WEBSOCKET;
}
