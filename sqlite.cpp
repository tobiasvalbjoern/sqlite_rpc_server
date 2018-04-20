/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <sqlite3.h> 
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <string.h>
const char* data = "Callback function called";
sqlite3 *db;
int rc;
char *sql;
char *zErrMsg = 0;
float result = 0;

struct fetch_t {
    float result;
};

fetch_t Fetch;

//We want to print the content of the table. 

static int callback(void *data, int argc, char **argv, char **azColName) {
    int i;
    fprintf(stderr, "%s: ", (const char*) data);

    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

static int callback2(void *data, int argc, char **argv, char **azColName) {
    int i;
    fetch_t *Fetch = (fetch_t *) data;

    for (i = 0; i < argc; i++) {
        if (strncmp(azColName[i], "TEMPERATURE",5) == 0) {
            Fetch->result = atof(argv[i]);
        }
    }

    printf("\n");
    return 0;
}

int sqlite_opendb() {
    rc = sqlite3_open("test.db", &db);

    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    // Create table SQL statement. The first column is an autoincrementing
    // primary key called entry, which is pointing to rowid

    sql = (char*) "CREATE TABLE TEMPERATURELOG("\
         "ENTRY INTEGER PRIMARY KEY,"\
         "Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"\
         "TEMPERATURE    FLOAT     NOT NULL);";


    // Execute SQL statement
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in CREATE TABLE: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully\n");
    }
}

void sqlite_closedb() {
    sqlite3_close(db);
}

void sqlite_insert(float value) {

    sqlite_opendb();
    char buffer[200];
    sprintf(buffer, "INSERT INTO TEMPERATURELOG (TEMPERATURE) VALUES (%.1f);", value);
    printf("Buffer: %s\n", buffer);
    // Create INSERT statement
    sql = buffer;
    // Execute SQL statement 
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in INSERT: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Records created successfully\n");
    }
    sqlite_closedb();
}

float sqlite_getlatest() {

    // Create SELECT statement to fetch results latest results
    sql = (char*) "SELECT * FROM TEMPERATURELOG ORDER BY TIMESTAMP DESC LIMIT 1;";

    // Execute SQL statement 
    rc = sqlite3_exec(db, sql, callback2, &Fetch, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in SELECT: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Select Operation done successfully\n");
    }

    return Fetch.result;

}

