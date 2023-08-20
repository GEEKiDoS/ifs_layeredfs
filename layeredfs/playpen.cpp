// The layeredfs playpen is for making sure shit works without having to boot
// the game a million times. BYO copy of AVS 2.17.x

#include <windows.h>

#include "hook.h"
#include "log.hpp"

void boot_avs(void);
bool load_dll(void);

typedef void (*avs_log_writer_t)(const char *chars, uint32_t nchars, void *ctx);
void (*avs_shutdown)();
void (*avs_boot)(node_t config, void *com_heap, size_t sz_com_heap, void *reserved, avs_log_writer_t log_writer, void *log_context);
node_t (*property_search)(property_t prop, node_t node, const char *path);

static bool print_logs = true;

#define QUIET_BOOT

void avs_playpen() {
    /*string path = "testcases.xml";
    void* prop_buffer = NULL;
    property_t prop = NULL;

    auto f = avs_fs_open(path.c_str(), 1, 420);
    if (f < 0)
        return;

    auto memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
    if (memsize < 0) {
        log_warning("Couldn't get memsize %08X", memsize);
        goto FAIL;
    }

    // who knows
    memsize *= 10;

    prop_buffer = malloc(memsize);
    prop = property_create(PROP_READ|PROP_WRITE|PROP_APPEND|PROP_CREATE|PROP_JSON, prop_buffer, memsize);
    if (!prop) {
        log_warning("Couldn't create prop");
        goto FAIL;
    }

    avs_fs_lseek(f, 0, SEEK_SET);
    property_insert_read(prop, 0, avs_fs_read, f);
    avs_fs_close(f);

    property_file_write(prop, "testcases.json");

FAIL:
    if (f)
        avs_fs_close(f);
    if (prop)
        property_destroy(prop);
    if (prop_buffer)
        free(prop_buffer);*/

    /*auto d = avs_fs_opendir(MOD_FOLDER);
    if (!d) {
        log_warning("couldn't d");
        return;
    }
    for (char* n = avs_fs_readdir(d); n; n = avs_fs_readdir(d))
        log_info("dir %s", n);
    avs_fs_closedir(d);*/
    //char name[64];
    //auto playpen = prop_from_file("playpen.xml");
    //auto node = property_search(playpen, NULL, "/");
    //auto start = property_node_traversal(node, TRAVERSE_FIRST_CHILD);
    //auto end = property_node_traversal(start, TRAVERSE_LAST_SIBLING);
    //print_node(node);
    //print_node(start);
    //print_node(end);
    /*for (int i = 0; i <= 8; i++) {
        if (i == 6 || i == 3) continue;
        log_info("Traverse: %d", i);
        auto node = property_search(playpen, NULL, "/root/t2");
        auto nnn = property_node_traversal(node, 8);
        auto nna = property_node_traversal(nnn, TRAVERSE_FIRST_ATTR);
        property_node_name(nna, name, 64);
        log_info("bloop %s", name);
        for (;node;node = property_node_traversal(node, i)) {
            if (!property_node_name(node, name, 64)) {
                log_info("    %s", name);
            }
        }
    }*/
    //prop_free(playpen);
}

int main(int argc, char** argv) {
    log_to_stdout();
    if(!load_dll()) {
        log_fatal("DLL load failed");
        return 1;
    }
    init_avs(); // fails because of Minhook not being initialised, don't care
#ifdef QUIET_BOOT
    print_logs = false;
    boot_avs();
    print_logs = true;
#else
    boot_avs();
#endif

    init(); // this double-hooks some AVS funcs, don't care

    avs_playpen();

    // it's just a bit noisy
    log_info("Shutting down");
    print_logs = false;
    avs_shutdown();

    return 0;
}

#define DEFAULT_HEAP_SIZE 16777216

void log_writer(const char *chars, uint32_t nchars, void *ctx) {
    if(print_logs)
        printf("%.*s", nchars, chars);
}

static const char *boot_cfg = R"(<?xml version="1.0" encoding="SHIFT_JIS"?>
<config>
  <fs>
    <root><device>.</device></root>
    <mounttable>
      <vfs name="boot" fstype="fs" src="dev/raw" dst="/dev/raw" opt="vf=1,posix=1"/>
      <vfs name="boot" fstype="fs" src="dev/nvram" dst="/dev/nvram" opt="vf=0,posix=1"/>
    </mounttable>
  </fs>
  <log><level>misc</level></log>
</config>
)";

static size_t read_str(int32_t context, void *dst_buf, size_t count) {
    memcpy(dst_buf, boot_cfg, count);
    return count;
}

bool load_dll(void) {
    auto avs = LoadLibraryA("avs2-core.dll");
    if(!avs) {
        log_fatal("Playpen: Couldn't load avs dll");
        return false;
    }
    if(!(avs_boot = (decltype(avs_boot))GetProcAddress(avs, "XCgsqzn0000129"))) {
        log_fatal("Playpen: couldn't get avs_boot");
        return false;
    }
    if(!(avs_shutdown = (decltype(avs_shutdown))GetProcAddress(avs, "XCgsqzn000012a"))) {
        log_fatal("Playpen: couldn't get avs_shutdown");
        return false;
    }
    if(!(property_search = (decltype(property_search))GetProcAddress(avs, "XCgsqzn00000a1"))) {
        log_fatal("Playpen: couldn't get property_search");
        return false;
    }

    return true;
}

void boot_avs(void) {
    auto avs_heap = malloc(DEFAULT_HEAP_SIZE);

    int prop_len = property_read_query_memsize(read_str, 0, 0, 0);
    if (prop_len <= 0) {
        log_fatal("error reading config (size <= 0)");
        return;
    }
    auto buffer = malloc(prop_len);
    auto avs_config = property_create(PROP_READ | PROP_WRITE | PROP_CREATE | PROP_APPEND, buffer, prop_len);
    if (!avs_config) {
        log_fatal("cannot create property");
        return;
    }
    if (!property_insert_read(avs_config, 0, read_str, 0)) {
        log_fatal("avs-core", "cannot read property");
        return;
    }

    auto avs_config_root = property_search(avs_config, 0, "/config");
    if(!avs_config_root) {
        log_fatal("no root config node");
        return;
    }

    avs_boot(avs_config_root, avs_heap, DEFAULT_HEAP_SIZE, NULL, log_writer, NULL);
}