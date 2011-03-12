
/* Copyright (C) 2010 Rafael Ostertag 
 *
 * This file is part of agentsmith.
 *
 * agentsmith is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * agentsmith is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * agentsmith.  If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id$ */

#ifndef THREADS_H
#define THREADS_H

#include "records.h"

extern void threads_start();
extern void threads_stop();

/*
 * This function is exported because it is also called from main.c upon exit.
 */
extern void threads_records_callback_action_removal(hostrecord_t *ptr);

#endif
