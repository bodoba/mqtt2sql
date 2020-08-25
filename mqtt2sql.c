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

/* ----------------------------------------------------------------------------------- *
 * System includes
 * ----------------------------------------------------------------------------------- */
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>

/* ----------------------------------------------------------------------------------- *
 * Add on libraries
 * ----------------------------------------------------------------------------------- */
#include <mosquitto.h>
#include <mysql.h>
#include <my_global.h>

/* ----------------------------------------------------------------------------------- *
 * Project includes
 * ----------------------------------------------------------------------------------- */
#include "mqtt2sql.h"
#include "logging.h"
#include "daemon.h"
#include "mqttGateway.h"

/* ----------------------------------------------------------------------------------- *
 * Mapping from MQTT Topics to table names
 * ----------------------------------------------------------------------------------- */
bridge_t mappingTable[MAX_BRIDGE];
    
/* ----------------------------------------------------------------------------------- *
 * Some globals we can't do without... ;)
 * ----------------------------------------------------------------------------------- */
MYSQL    *sql               = NULL;              // database connection
int      debug              = DEBUG;             // debug level
bool     foreground         = false;             // run in foreground, not as daemon
config_t cf;                                     // runtome configuration

/* ----------------------------------------------------------------------------------- *
 * Prototypes
 * ----------------------------------------------------------------------------------- */
int  main(int argc, char *argv[]);

// Database operations
bool sqlConnect( int argc, char *argv[] );
void sqlClose(void);
bool sqlAddRecord(const char* tableName, const char *value );

void addBridgeRule( char *topic, char *table, unsigned int type);
void mqttMessageCB(char* payload, int payloadlen, char *topic);
void mainLoop(void);
bool loadConfig(const char *configFile);

/* ----------------------------------------------------------------------------------- *
 * Connect to SQL server
 * ----------------------------------------------------------------------------------- */
bool sqlConnect( int argc, char *argv[] ) {
    bool retval = false;
    
    if (mysql_library_init(argc, argv, NULL)) {
        writeLog(LOG_ERR, "Could not initialize MySQL client library");
    } else {
        sql = mysql_init(sql);
        if (!sql) {
            writeLog(LOG_ERR, "Init faild, out of memory?");
        } else if (!mysql_real_connect(sql, cf.sql_address, cf.sql_user, cf.sql_password, cf.sql_database, 0, NULL, 0 )) {
            writeLog(LOG_ERR, "Connect failed to SQL server %s:%s as %s", cf.sql_address, cf.sql_database, cf.sql_user);
        } else {
            writeLog(LOG_INFO, "Connected to SQL server %s:%s as %s", cf.sql_address, cf.sql_database, cf.sql_user);
            retval = true;
        }
    }
    return(retval);
}

/* ----------------------------------------------------------------------------------- *
 * Close connection to SQL server
 * ----------------------------------------------------------------------------------- */
void sqlClose(void) {
    mysql_close(sql);
    mysql_library_end();
}

/* ----------------------------------------------------------------------------------- *
 * Add record to database table
 * ----------------------------------------------------------------------------------- */
bool sqlAddRecord(const char* tableName, const char* value ) {
    bool retval = false;
    if ( sql ) {
        char statement[1024];
        sprintf( statement, "Insert into `%s` (value) VALUES (%s)", tableName, value );
        if (mysql_query(sql,statement) != 0) {
            writeLog(LOG_ERR, "SQL Statement: %s", statement);
        } else {
            writeLog(LOG_DEBUG, "SQL Statement: %s", statement);
            retval = true;
        }
    }
    return retval;
}

/* ----------------------------------------------------------------------------------- *
 * mqtt message received
 * ----------------------------------------------------------------------------------- */
void mqttMessageCB(char* payload, int payloadlen, char *topic) {
    writeLog( LOG_DEBUG, "Received message: <%s> : <%s>", topic, payload);

    if ( payloadlen > MAX_PAYLOAD ) {
        writeLog( LOG_WARNING, "Truncating payload to %d bytes", MAX_PAYLOAD);
        payload[MAX_PAYLOAD-1] = (char)0;
    }
    
    int idx = 0;
    while (mappingTable[idx].topic) {
        bool match;

        mosquitto_topic_matches_sub(mappingTable[idx].topic, topic, &match);
        if ( match ) {
            writeLog(LOG_INFO, "Match for topic <%s>", mappingTable[idx].topic);
            mappingTable[idx].lastRecord = time(NULL);
            
            if (mappingTable[idx].type == UPDATE_T) {
                // every update is persisted
                writeLog(LOG_INFO, "Update: %s/%s, saving new value...", mappingTable[idx].tableName, payload);
                sqlAddRecord( mappingTable[idx].tableName, payload);
            } else if (mappingTable[idx].type == CHANGE_T) {
                // persist only if value changed
                if ( !mappingTable[idx].lastValue || strcmp(mappingTable[idx].lastValue, payload)) {
                    writeLog(LOG_INFO, "Change: %s/%s, saving new value...", mappingTable[idx].tableName, payload);
                    sqlAddRecord( mappingTable[idx].tableName, payload);
                    if (mappingTable[idx].lastValue) {
                        free(mappingTable[idx].lastValue);
                    }
                    mappingTable[idx].lastValue = strdup(payload);
                } else {
                    writeLog(LOG_INFO, "Unchanged: %s/%s, skipping...", mappingTable[idx].tableName, payload);
                }
            }
            break;
        }
        idx++;
    }
}

/* ----------------------------------------------------------------------------------- *
 * M A I N
 * ----------------------------------------------------------------------------------- */
void mainLoop(void) {
    for ( ;; ) {                              // never stop working
        sleep(1);                             // have a rest
    }
}

/* ----------------------------------------------------------------------------------- *
 * Add bridge rule
 * ----------------------------------------------------------------------------------- */
void addBridgeRule( char *topic, char *table, unsigned int type) {
    writeLog(LOG_INFO, "Add Bridge rule: `%s` -> `%s` (type %d)", topic, table, type);
    
    int next=0;
    while (mappingTable[next].topic  != NULL) {
        next++;
    }
    
    if ( next < (MAX_BRIDGE-1) ) {
        mappingTable[next].topic      = topic;
        mappingTable[next].tableName  = table;
        mappingTable[next].lastValue  = NULL;
        mappingTable[next].lastRecord = (time_t)0;
        mappingTable[next].type       = type;
        
        next++;
        
        mappingTable[next].topic      = NULL;
        mappingTable[next].tableName  = NULL;
        mappingTable[next].lastValue  = NULL;
        mappingTable[next].lastRecord = (time_t)0;
        mappingTable[next].type       = 0;
    } else {
        writeLog(LOG_ERR, "Maximum number of mapping table entries (%d) reached", MAX_BRIDGE);
    }
}

/* ----------------------------------------------------------------------------------- *
 * Load configuration
 * ----------------------------------------------------------------------------------- */
char *nextValue( char **cursor) {
    while (**cursor && **cursor != ' ') (*cursor)++;                   /*   skip token */
    **cursor = '\0'; (*cursor)++;                                      /* end of token */
    while (**cursor && **cursor == ' ') (*cursor)++;                   /* skip spaces  */
    return *cursor;
}

bool loadConfig(const char *configFile) {
    bool success = false;
    FILE *fp = NULL;
    
    // set defaul values
    cf.sql_address     = SQL_ADDRESS;
    cf.sql_database    = SQL_DATABASE;
    cf.sql_user        = SQL_USER;
    cf.sql_password    = SQL_PASSWORD;
         
    cf.mqtt_address    = MQTT_ADDRESS;
    cf.mqtt_port       = MQTT_PORT;
    cf.mqtt_keepalive  = MQTT_KEEPALIVE;
        
    if (configFile) {
        fp = fopen(configFile, "rb");
        if (fp) {
            char  *line=NULL, *cursor;
            size_t n, length = getline(&line, &n, fp);
            int lineNo = 1;
                        
            writeLog(LOG_NOTICE, "Loading configuration from %s", configFile );
            
            while ( length != -1) {
                if ( length > 1 ) {                                      // skip empty lines
                    cursor = line;
                    if (line[length-1] == '\n') line[length-1] = '\0';   // remove trailing newline
                    while (*cursor == ' ' || *cursor == '\t') cursor++;  // remove leading whitespace
                    if ( *cursor != '#') {                               // skip '#' comments
                        char *token = cursor;
                        char *value = nextValue(&cursor);
                        
                        writeLog(LOG_DEBUG, "[%s:%04d] Token: \"%s\" Value: \"%s\"", configFile, lineNo, token, value);
                        
                        if (!strcmp(token, "SQL_ADDRESS")) {
                            cf.sql_address = strdup(value);
                        } else if (!strcmp(token, "SQL_DATABASE")) {
                            cf.sql_database = strdup(value);
                        } else if (!strcmp(token, "SQL_USERNAME")) {
                            cf.sql_user = strdup(value);
                        } else if (!strcmp(token, "SQL_PASSWORD")) {
                            cf.sql_password = strdup(value);
                        } else if (!strcmp(token, "CHANGE")) {
                            char *v = value;
                            while ( *v && *v != ' ') v++;
                            *v = '\0';
                            addBridgeRule(strdup(value), strdup(nextValue(&cursor)), 1);
                        } else if (!strcmp(token, "UPDATE")) {
                            char *v = value;
                            while ( *v && *v != ' ') v++;
                            *v = '\0';
                            addBridgeRule(strdup(value), strdup(nextValue(&cursor)), 2);
                        }
                    }
                }
                free(line);
                n=0;
                length = getline(&line, &n, fp);
                lineNo++;
            }
            fclose(fp);
            success = true;
        } else {
            writeLog(LOG_WARNING, "Could not open %s", configFile );
        }
    } else {
        writeLog(LOG_WARNING, "No Config File specified" );
    }
    return success;
}

/* ----------------------------------------------------------------------------------- *
 * M A I N
 * ----------------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
    char *configFile = CONFIG_FILE;
    
    // Process command line options
     for (int i=0; i<argc; i++) {
         if (!strcmp(argv[i], "-d")) {          // '-d' turns debug mode on
             debug++;
         }
         if (!strcmp(argv[i], "-f")) {          // '-f' forces forground mode
             foreground=true;
         }
         if (!strcmp(argv[i], "-c")) {          // '-c' specify configuration file name
             configFile = strdup(argv[++i]);
         }
     }
    
    // initialize logging channel
    initLog(!foreground);
    setLogLevel(LOG_NOTICE+debug);

    mappingTable[0].topic      = NULL;
    mappingTable[0].tableName  = NULL;
    mappingTable[0].lastValue  = NULL;
    mappingTable[0].lastRecord = (time_t)0;
    mappingTable[0].type       = 0;
    
    // load configuration
    loadConfig(configFile);
    
    // Daemonize
    if (!foreground) {
         // run in background
         daemonize(PID_FILE);
    } else {
         writeLog(LOG_NOTICE, "Running in foreground");
    }
         
    // connect to MQTT broker
    if (mqttInit(cf.mqtt_address, cf.mqtt_port, cf.mqtt_keepalive, mappingTable, &mqttMessageCB)) {
        writeLog(LOG_INFO, "Connected to MQTT boker at %s:%d", cf.mqtt_address, cf.mqtt_port);
    
        // connect to SQL server
        if (sqlConnect(argc, argv)) {
        
            // enter main loop
            mainLoop();

            // end server connections
            sqlClose();
            mqttEnd();
        }
    } else {
        writeLog(LOG_INFO, "Could not connect to MQTT boker at %s:%d", cf.mqtt_address, cf.mqtt_port);
    }
    
    return 0;
}
