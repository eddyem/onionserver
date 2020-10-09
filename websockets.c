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

#include "websockets.h"

#include <errno.h>
#include <onion/log.h>
#include <stdio.h>
#include <string.h>
#include <usefull_macros.h>

#define BUFLEN 255

static onion_connection_status websocket_cont(_U_ void *data, onion_websocket *ws, ssize_t dlen){
    FNAME();
    char tmp[BUFLEN+1];
    if(dlen > BUFLEN) dlen = BUFLEN;

    int len = onion_websocket_read(ws, tmp, dlen);
    if(len <= 0){
        ONION_ERROR("Error reading data: %d: %s (%d)", errno, strerror(errno), dlen);
        return OCS_NEED_MORE_DATA;
    }
    tmp[len] = 0;
    DBG("WS: got %s", tmp);
    onion_websocket_printf(ws, "Echo: %s", tmp);
    ONION_INFO("Read from websocket: %d: %s", len, tmp);
    return OCS_NEED_MORE_DATA;
}

onion_connection_status websocket_run(_U_ void *data, onion_request *req, onion_response *res){
    FNAME();
    onion_websocket *ws = onion_websocket_new(req, res);
    if (!ws){
        green("PROC\n");
        DBG("Processed");
        return OCS_PROCESSED;
    }
    DBG("WS ready");
    green("RDY\n");
    onion_websocket_printf(ws, "Hello from server. Write something to echo it");
    onion_websocket_set_callback(ws, websocket_cont);
    return OCS_WEBSOCKET;
}
