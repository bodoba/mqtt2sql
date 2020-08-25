/* *********************************************************************************** */
/*                                                                                     */
/*  Copyright (c) 2020 by Bodo Bauer <bb@bb-zone.com>                                  */
/*                                                                                     */
/*  This program is free software: you can redistribute it and/or modify               */
/*  it under the terms of the GNU General Public License as published by               */
/*  the Free Software Foundation, either version 3 of the License, or                  */
/*  (at your option) any later version.                                                */
/*                                                                                     */
/*  This program is distributed in the hope that it will be useful,                    */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      */
/*  GNU General Public License for more details.                                       */
/*                                                                                     */
/*  You should have received a copy of the GNU General Public License                  */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.              */
/* *********************************************************************************** */

#ifndef mqtt2sql_h
#define mqtt2sql_h

#include <stdio.h>
#include <time.h>

#define CONFIG_FILE  "/etc/mqtt2sql.cnf"

/* ----------------------------------------------------------------------------------- *
 * Database server connection
 * ----------------------------------------------------------------------------------- */
#define SQL_ADDRESS  "127.0.0.1"
#define SQL_USER     "mqtt2sql"
#define SQL_PASSWORD "joshua"
#define SQL_DATABASE "mqtt2sql"

/* ----------------------------------------------------------------------------------- *
 * MQTT broker connection
 * ----------------------------------------------------------------------------------- */
#define MQTT_ADDRESS  "192.168.100.26"
#define MQTT_PORT      1883
#define MQTT_KEEPALIVE 60

/* ----------------------------------------------------------------------------------- *
 * Default values for configurable parameter
 * ----------------------------------------------------------------------------------- */
#define DEBUG        0                           /* no debug info by default           */
#define PID_FILE     "/var/run/yardcontrol.pid"  /* damon stores its pid here          */
#define MAX_PAYLOAD  512                         /* max length of the payload string   */
#define MAX_BRIDGE   128                         /* max mapping table entries          */

/* ----------------------------------------------------------------------------------- *
 * Runtime configuraition
 * ----------------------------------------------------------------------------------- */
typedef struct config {
    char *sql_address;
    char *sql_database;
    char *sql_user;
    char *sql_password;
    
    char *mqtt_address;
    int   mqtt_port;
    int   mqtt_keepalive;
} config_t;

/* ----------------------------------------------------------------------------------- *
 * Definition of topics to be persisted
 * ----------------------------------------------------------------------------------- */
#define CHANGE_T  1
#define UPDATE_T  2

typedef struct    bridge {
    const char    *topic;
    const char    *tableName;
    char          *lastValue;
    time_t        lastRecord;
    unsigned int  type;            // Save on 1: Change or 2:Update
} bridge_t;

#endif /* mqtt2sql_h */
