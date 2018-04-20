#include "ArduinoJson/ArduinoJson.h"
#include <iostream>
#include <syslog.h>
#include <string>
#include <cstring>
#include "sqlite.h"
using namespace std;

int jsonrpc_debug(char * jsonrpc) {
    DynamicJsonBuffer jsonBuffer;
    //Allocates and populate a JsonObject called root from a JSON string.
    //returns a reference to the new JsonObject
    JsonObject& root = jsonBuffer.parseObject(jsonrpc);

    string jsonrpc_version = root["jsonrpc"];
    string method = root["method"];
    JsonVariant params = root["params"];
    long id = root["id"];

    //Tells if the array is valid, which can be used:
    //to check if the array was successfully parsed, or
    //to check if the array was successfully allocated.
    if (!root.success()) {
        cout << "Not a valid JSON object" << endl;
        return -1;
    }

    if (jsonrpc_version != "2.0"  || !method.length() ) {
        cout << "Not a valid JSONRPC object" << endl;
        return -1;
    }

    cout << "----------JSONRPC-----------" << endl;
    cout << "Version: " << jsonrpc_version << endl;

    if (id) {
        cout << "ID: " << id << endl;
    } else {
        cout << "Notification" << endl;
    }

    cout << "Method: " << method << endl;
    //tests if params is JsonArray
    if (params.is<JsonArray>()) {
        cout << "Parameter Array" << endl;
        int i = 0;
        for( const auto& kv : params.as<JsonObject>() ) {
            cout << "[" << kv.key << "] " << kv.value.as<char*>() << endl;
        }
    }
    if (params.is<JsonObject>()) {
        cout << "Parameter Object" << endl;
        for( const auto& kv : params.as<JsonObject>() ) {
            cout << kv.key << " -> " << kv.value.as<char*>() << endl;
        }
    }

    cout << "-------ENDJSONRPC-----------" << endl;
    return 0;
}

string jsonrpc_handler(string jsonrpc) {
    //For debugging purposes
    //cout << "Dump: ##" << jsonrpc << "##" << endl;
    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonrpc);

    string jsonrpc_version = root["jsonrpc"];
    string method = root["method"];
    JsonVariant params = root["params"];
    long id = root["id"];

    if (!root.success()) {
        //cout << "Not a valid JSON object" << endl;
        syslog(LOG_INFO, "Not a valid JSON object, -32700\n");
        return "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32700, \"message\": \"Parse error\"}, \"id\": null}\n";
    }
    
    if (jsonrpc_version != "2.0" || !method.length() ) {
        //cout << "Not a valid JSONRPC object" << endl;
        syslog(LOG_INFO, "Not a valid JSONRPC object, -32600\n");
        return "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32600, \"message\": \"Invalid request\"}, \"id\": null}\n";
    }
    
    if(method == "getTemp") {
        //For debugging purposes.
        //cout << "Temp was requested" << endl;
        syslog(LOG_INFO, "Temp was requested\n");
        root["result"] = sqlite_getlatest();
        //sqlite_getlatest();
    } 
    
    root.remove("method");
    root.remove("params");
    string output;
    root.printTo<string>(output);
    output.append("\n");
    return output;
}