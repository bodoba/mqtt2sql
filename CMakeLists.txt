# *********************************************************************************** #
#                                                                                     #
#  Copyright (c) 2020 by Bodo Bauer <bb@bb-zone.com>                                  #
#                                                                                     #
#  This program is free software: you can redistribute it and/or modify               #
#  it under the terms of the GNU General Public License as published by               #
#  the Free Software Foundation, either version 3 of the License, or                  #
#  (at your option) any later version.                                                #
#                                                                                     #
#  This program is distributed in the hope that it will be useful,                    #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of                     #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      #
#  GNU General Public License for more details.                                       #
#                                                                                     #
#  You should have received a copy of the GNU General Public License                  #
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.              #
# *********************************************************************************** #
include(CheckIncludeFile)
cmake_minimum_required(VERSION 3.6)

project (mqtt2sql)

# MariaDB SQL Connector
find_library(LIB_SQL  mariadb)
find_path(INC_SQL
  NAMES mysql.h
  PATH_SUFFIXES mariadb mysql
)

# Mosquitto MQTT Client
find_library(LIB_MQTT mosquitto)

set (CMAKE_C_FLAGS "-std=gnu11 -Wall -I${INC_SQL}")

# all executables end up in bin
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_executable(mqtt2sql mqtt2sql.c mqttGateway.c logging.c daemon.c)

target_link_libraries(mqtt2sql "${LIB_MQTT}" "${LIB_SQL}")

set(CMAKE_INSTALL_PREFIX /)
INSTALL(PROGRAMS bin/mqtt2sql DESTINATION usr/sbin)
#add_subdirectory(Contrib)
