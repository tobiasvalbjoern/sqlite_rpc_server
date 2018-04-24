/*
 * A simple program using C/C++ for SQLite
 */
#include <stdlib.h>
#include <sqlite3.h> 
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <syslog.h>

#include "tserver.h"
#include "jsonrpc.h"
#include "sqlite.h"

const char* INTERFACE= "0.0.0.0"; //0.0.0.0 - Listening on all interfaces
const char* PORT="1955" ;

using namespace std;

int main(int argc, char* argv[]) {
    sqlite_opendb();  

    tserver_init(INTERFACE, PORT, jsonrpc_handler); //Starts a new threaded server 
    while(1){
    sleep(1);
    }
}