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

#pragma once
#ifndef NETPROTO_H__

#include "websockets.h"

// not more than 8 (or need to patch libonion)
#define MAX_WSCLIENTS       (5)

void *runMainProc(void *data);

void process_WS_signal(onion_websocket *ws, char *signal);
int register_WS(onion_websocket *ws);
void unregister_WS(onion_websocket *ws);
void cleanup_WS();
void send_all_WS(char *data);
void send_one_WS(onion_websocket *ws, char *data);

#define NETPROTO_H__
#endif // NETPROTO_H__
