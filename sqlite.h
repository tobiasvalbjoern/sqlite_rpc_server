/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sqlite.h
 * Author: tobias
 *
 * Created on April 20, 2018, 11:52 AM
 */

#ifndef SQLITE_H
#define SQLITE_H

void sqlite_opendb();
void sqlite_closedb();
void sqlite_insert(float value);
float sqlite_getlatest();

#endif /* SQLITE_H */

