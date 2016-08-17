#include "ConfigManager.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <syslog.h>

extern void fp_log(const char *fxn_name, int severity, const char *message, ...);



ConfigManager::ConfigManager() {
    this->current_config = NULL;
    this->complete_args  = NULL;
}


ConfigManager::~ConfigManager() {
    this->unloadConfig();
}


// Dump the currently-running configuration to the logs.
void ConfigManager::dumpCurrentConfig() {
    int i    = 0;
    char *temp_buf = (char *) alloca(256);
    ConfigItem *cur    = this->current_config;
    fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Current configuration:");
    fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "=================================================================");
    while (cur != NULL) {
        memset(temp_buf, 0x00, 256);
        sprintf(temp_buf, "%2d %30s %10s %5s %s", i++, cur->key, (this->isConfKeyRequired(cur->key) ? "(required)" : "          "), (cur->clobberable ? "     " : "(cli)"), cur->value);
        fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "%s", temp_buf);
        cur = cur->next;
    }
    fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "=================================================================");
}


// Call with a key string to get the corresponding value as a string.
//    Returns NULL on failure.
char* ConfigManager::getConfigStringByKey(const char *desired_key){
    ConfigItem *cur    = this->current_config;
    while (cur != NULL) {
        if (strcasecmp(cur->key, desired_key) == 0) return cur->value;
        cur = cur->next;
    }
    return NULL;
}


int ConfigManager::setExistingConfig(const char *desired_key, const char *value){
    int return_value    = -1;
    ConfigItem *cur    = this->current_config;
    while (cur != NULL) {
        if (strcasecmp(cur->key, desired_key) == 0) {
            if (cur->clobberable) {
                free(cur->value);
                cur->value = (char *) malloc(strlen(value));
                strcpy(cur->value, value);
            }
            return 0;
        }
        cur = cur->next;
    }
    return return_value;
}


// Returns 0 if the key does not exist and 1 if it does.
int ConfigManager::configKeyExists(const char *desired_key) {
    ConfigItem *cur    = this->current_config;
    while (cur != NULL) {
        if (strcasecmp(cur->key, desired_key) == 0) return 1;
        cur = cur->next;
    }
    return 0;
}


// Call with a key string to get the corresponding value as an integer.
//    Returns -1 on failure.
int ConfigManager::getConfigIntByKey(const char *desired_key){
    int return_value    = -1;
    char *temp_str    = this->getConfigStringByKey(desired_key);
    if (temp_str != NULL) {
        return atoi(temp_str);
    }
    return return_value;
}


// Call this function to remove an individual configuration item.
void ConfigManager::freeConfigItemByKey(const char *desired_key) {
    ConfigItem *cur    = this->current_config;
    ConfigItem *prior    = NULL;
    while (cur != NULL) {
        if (strcasecmp(cur->key, desired_key) == 0) {
            if (prior != NULL) prior->next = cur->next;
            else current_config = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            return;
        }
        prior    = cur;
        cur = cur->next;
    }
}



// Call this function to blow the entire configuration set away.
//    WARNING: This means you will have no config. Might go without saying, but you never know.
void ConfigManager::unloadConfig(){
    ConfigItem *cur    = this->current_config;
    ConfigItem *nxt    = NULL;
    while (cur != NULL) {
        free(cur->key);
        free(cur->value);
        nxt = cur->next;
        free(cur);
        cur = nxt;
    }
}


/**
* Provision a new ConfigItem and return it.
*/
ConfigItem* ConfigManager::newConfigItem(const char *key, const char *val, bool allow_clobber) {
    ConfigItem* return_value  = (ConfigItem*) malloc(sizeof(ConfigItem));
    return_value->key         = strdup(key);
    return_value->value       = strdup(val);
    return_value->clobberable = allow_clobber;
    return_value->next        = NULL;
    return return_value;
}


/**
* Given a key-value pair, insert the value into the configuration list.
*    If the key doesn't exist, creates a new entry in the list.
*    If the key already exists, replaces the value for that key.
*/
void ConfigManager::insertConfigItem(const char *key, const char *val, bool allow_clobber) {
    if (this->current_config == NULL)      this->current_config = this->newConfigItem(key, val, allow_clobber);
    else if (this->configKeyExists(key))   setExistingConfig(key, val);
    else {
        ConfigItem *cur    = this->current_config;
        while (cur->next != NULL) cur = cur->next;
        cur->next = this->newConfigItem(key, val, allow_clobber);
    }
}


/**
* Given a key-value pair, insert the value into the configuration list.
* Override to support easy calling that allows clobber.
*/
void ConfigManager::insertConfigItem(const char *key, const char *val) {
    this->insertConfigItem(key, val, true);
}


/**
* Variadic function to set the list of required configuration keys.
* The first parameter is the number of configuration keys to be defined. This MUST
*   match the cardinality of the var_args. If it does not, the behavior is undefined.
* Can be called several times without hurting existing defs.
*/
void ConfigManager::setRequiredConfKeys(int count, ...) {
    RequiredConfig *temp_r_conf = NULL;
    RequiredConfig *current_r_conf = this->complete_args;

    va_list marker;
    va_start(marker, count);
    for (int i = 0; i < count; i++) {
        temp_r_conf = (RequiredConfig*) malloc(sizeof(RequiredConfig));
        if (temp_r_conf != NULL) {
            memset(temp_r_conf, 0x00, sizeof(RequiredConfig));
            temp_r_conf->key = strdup(va_arg(marker, char*));
            if (this->complete_args == NULL) {
                this->complete_args = temp_r_conf;
            }
            else {
                current_r_conf = this->complete_args;
                while (current_r_conf->next != NULL) current_r_conf = current_r_conf->next;   // Fly to the end of the list...
                current_r_conf->next = temp_r_conf;
                current_r_conf = current_r_conf->next;
            }
        }
    }
    va_end(marker);
}


bool ConfigManager::isConfKeyRequired(char *test_key) {
    RequiredConfig *current_r_conf = this->complete_args;
    while (current_r_conf != NULL) {
        if (strcasecmp(current_r_conf->key, test_key) == 0) return true;
        current_r_conf = current_r_conf->next;
    }
    return false;
}


/**
* Returns true if the config is complete enough to start the program.
* False otherwise.
*
* Note that this class depends on the caller to define what a 'complete config' is.
* If no definition of completeness was given, return true.
*/
bool ConfigManager::isConfigComplete(void) {
    bool return_value = true;
    if (this->complete_args != NULL) {
        RequiredConfig *temp_key = this->complete_args;
        while (temp_key != NULL) {
            if (this->configKeyExists(temp_key->key) == 0) {
                fp_log(__PRETTY_FUNCTION__, LOG_ERR, "Missing the configuration item %s.", temp_key->key);
                return_value = false;
            }
            temp_key = temp_key->next;
        }
    }
    return return_value;
}


/*
* Loads the configuration options from a MySQL database and integrates them into our existing current_config.
* Returns the number of configuration directives that were loaded from the database.
*/
int ConfigManager::loadConfigFromDb(MySQLConnector *db){
    int return_value = -1;    // Fail by default.

    if (db->dbConnected()) {
        if (db->node_id != NULL) {
            this->insertConfigItem("node_id", db->node_id);
        }
        if (db->r_query("SELECT conf_key,conf_value FROM configuration WHERE component='DAEMON';")) {
            MYSQL_ROW   row;
            MYSQL_RES   *result;
            result = db->result;
            if (result != NULL) {
                int num_rows    = mysql_num_rows(result);
                if (num_rows > 0) {                        // The nominal case.
                    return_value = 0;  // At this point, we know we have not failed.
                    while ((row = mysql_fetch_row(result))) {
                        this->insertConfigItem(row[0], row[1]);
                        return_value++;
                    }
                }
                else if (num_rows == 0) {
                    fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Zero rows were returned while looking for configuration data.");
                }
                else {
                    fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Experienced an error while looking in the database for configuration data: %s", mysql_error(db->mysql));
                }
                mysql_free_result(result);
            }
            else {
                fp_log(__PRETTY_FUNCTION__, LOG_NOTICE, "Expected a result set, but none was provided.");
            }
        }
        this->dumpCurrentConfig();
    }
    else {
        fp_log(__PRETTY_FUNCTION__, LOG_WARNING, "loadConfigFromDb(): Could not establish a connection to a database. Therefore, no config was read from it.");
    }
    return return_value;
}
