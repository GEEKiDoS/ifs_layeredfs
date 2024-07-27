#pragma once

#include <set>
#include <string>

#include "utils.hpp"

typedef struct {
    bool verbose_logs;
    bool developer_mode;
    bool disable;
    const char *logfile;
    const char *laochan_data_dir;
    std::set<std::string, CaseInsensitiveCompare> allowlist;
    std::set<std::string, CaseInsensitiveCompare> blocklist;
} config_t;

#define DEFAULT_LOGFILE "ifs_hook.log"

extern config_t config;

void load_config(void);
void print_config(void);
