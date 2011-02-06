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

#ifndef CFG_H
#define CFG_H

#include "globals.h"

struct _config {
	char pidfile[_MAX_PATH];
	char syslogfile[_MAX_PATH];
	char action[_MAX_PATH];
	char exclude[_MAX_PATH];
	char regex[BUFFSIZE];
	int action_threshold;
	int time_interval;
	int purge_after;
};

typedef struct _config config;

extern config* config_read(const char* file);
extern config* config_get();

#endif
