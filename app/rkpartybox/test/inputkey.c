#include <linux/version.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>

#define DEFAULT_DURATION 20
#define MAX_CODES 0x300
#define MAX_NODES 10

struct KeyCode {
    int code;
    int type;
};

struct KeyNode {
    int eventx;
    char name[20]; // Limit name size to save memory
    int code_count;
    struct KeyCode codes[MAX_CODES];
};

struct KeyNode* key_nodes;
int key_node_count = 0;

#define check_write_result(a) \
    do {if(a == -1) {fprintf(stderr, "%s %d: %s\n", __func__, __LINE__, strerror(errno));}} while (0)

struct input_key_pair {
    int code;
    char* name;
};

static const struct input_key_pair INPUT_KEY_TABLE[] = {
    /*kernel    user*/
    {373,       "KEY_MODE"}, //KEY_MODE
    {207,       "KEY_PLAY"}, //KEY_PLAY
    {209,       "KEY_3A"},
    {115,       "KEY_VOLUME_UP"}, //KEY_VOLUMEUP
    {114,       "KEY_VOLUME_DOWN"}, //HKEY_VOLDOWN
    {248,       "KEY_VOCAL_SPLIT"}, //KEY_MICMUTE
    {0x21e,     "KEY_LIGHTS_TOOGLE"},
};

const char* getKeyCodeName(int code) {
	for (int i = 0; i < sizeof(INPUT_KEY_TABLE)/sizeof(struct input_key_pair); i++) {
		if (INPUT_KEY_TABLE[i].code == code)
			return INPUT_KEY_TABLE[i].name;
	}

	return "UNKOWN";
}
void print_nodes() {
    printf("\nRegistered %d key nodes:\n", key_node_count);
    for (int i = 0; i < key_node_count; i++) {
		printf("/dev/input/event%d:	%s\n", key_nodes[i].eventx, key_nodes[i].name);
        for (int j = 0; j < key_nodes[i].code_count; j++) {
            printf("    Event type %s	code %d (%s)\n",
                key_nodes[i].codes[j].type == EV_KEY ? "EV_KEY" : "EV_REL",
                key_nodes[i].codes[j].code,
				getKeyCodeName(key_nodes[i].codes[j].code)
				);
        }
    }
}

void print_help() {
    printf("Usage: inputkey [OPTIONS]\n");
    printf("Options:\n");
    printf("  --code, -c   Event code to simulate (required)\n");
    printf("  --index, -i  Event node index (e.g., x for /dev/input/eventx)\n");
    printf("  --time, -t   Duration in ms (default: %d)\n", DEFAULT_DURATION);
    printf("  --dir, -d    Direction (1 for forward, -1 for reverse, only for EV_REL)\n");
    printf("  --help, -h   Show this help message\n");
    printf("Examples:\n");
    printf("  inputkey -c 207 -i 4 -t 30\n");
    printf("  inputkey -c 207\n");
    printf("  inputkey -c 0 -i 0 -d -1\n");
}

void simulate_key_event(int eventx, int type, int code, int duration, int direction) {
    ssize_t result;
    char path[32];
    snprintf(path, sizeof(path), "/dev/input/event%d", eventx);
    printf("%s, %s\n", __func__, path);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    struct input_event event = {0};
    struct timeval time;
    gettimeofday(&time, NULL);

    event.time = time;
    event.type = type;
    event.code = code;
    event.value = (type == EV_REL) ? direction : 1;
	result= write(fd, &event, sizeof(event));
	check_write_result(result);

	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	result= write(fd, &event, sizeof(event));
    check_write_result(result);

    if (type == EV_KEY) {
        usleep(duration * 1000);

        gettimeofday(&time, NULL);
        event.time = time;
        event.type = EV_KEY;
        event.code = code;
        event.value = 0; // Key release
        result= write(fd, &event, sizeof(event));
        check_write_result(result);

        event.type = EV_SYN;
        event.code = SYN_REPORT;
        event.value = 0;
        result= write(fd, &event, sizeof(event));
        check_write_result(result);
	}

    close(fd);
}

int is_event_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}

int is_valid_name(const char *name) {
    return strstr(name, "rotary") || strstr(name, "adc-key") || strstr(name, "gpio-key");
}

int my_version_sort(const struct dirent **a, const struct dirent **b) {
    return strverscmp((*a)->d_name, (*b)->d_name);
}

void scan_key_nodes() {
    struct KeyNode *node;
    struct dirent **namelist;
    int ndev = scandir("/dev/input", &namelist, is_event_device, my_version_sort);

    key_node_count = 0;
    if (ndev <= 0) {
        perror("No event devices found");
        return;
    }

    for (int i = 0; i < ndev && key_node_count < MAX_NODES; i++) {
        if (strncmp(namelist[i]->d_name, "event", 5) != 0)
            continue;

        char path[32];
        snprintf(path, sizeof(path), "/dev/input/%s", namelist[i]->d_name);

        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            free(namelist[i]);
            continue;
        }

        struct KeyNode *new_node = realloc(key_nodes, (key_node_count+1)*sizeof(struct KeyNode));
        if (!new_node) {
            fprintf(stderr, "realloc node failed, key_node_count:%d\n", key_node_count);
            free(key_nodes);
        }
        key_nodes = new_node;

        node = &key_nodes[key_node_count];
        node->eventx = atoi(&namelist[i]->d_name[5]);
        if (ioctl(fd, EVIOCGNAME(sizeof(node->name)), node->name) < 0 || !is_valid_name(node->name)) {
            close(fd);
            free(namelist[i]);
            continue;
        }

        unsigned long event_types[2] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(event_types)), event_types) < 0) {
            close(fd);
            free(namelist[i]);
            continue;
        }

        node->code_count = 0;
        for (int type = EV_KEY; type <= EV_REL; type++) {
            if (event_types[type / 8] & (1 << (type % 8))) {
                unsigned char codes[MAX_CODES] = {0};
                if (ioctl(fd, EVIOCGBIT(type, sizeof(codes)), codes) < 0)
                    continue;

                for (int code = 0; code < MAX_CODES && node->code_count < MAX_CODES; code++) {
                    if (codes[code / 8] & (1 << (code % 8))) {
                        node->codes[node->code_count].code = code;
                        node->codes[node->code_count].type = type;
                        node->code_count++;
                    }
                }
            }
        }

        if (node->code_count > 0)
            key_node_count++;

        close(fd);
        free(namelist[i]);
    }
    free(namelist);
}

int main(int argc, char *argv[]) {
    int opt, event_index = -1, event_code = -1, duration = DEFAULT_DURATION, direction = 1;

    static struct option long_options[] = {
        {"code", required_argument, 0, 'c'},
        {"index", required_argument, 0, 'i'},
        {"time", required_argument, 0, 't'},
        {"dir", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    scan_key_nodes();

    if (argc < 2) {
        goto exit_failed;
    }

    while ((opt = getopt_long(argc, argv, "c:i:t:d:h", long_options, NULL)) != -1) {
        switch (opt) {
        case 'c': event_code = atoi(optarg); break;
        case 'i': event_index = atoi(optarg); break;
        case 't': duration = atoi(optarg); break;
        case 'd': direction = atoi(optarg) > 0? 1: -1; break;
        case 'h':
        default: goto exit_failed;
        }
    }

    if (event_code < 0) {
        fprintf(stderr, "-c or --code para must needed and postive:%d\n", event_code);
        goto exit_failed;
    }

    if (event_index >= 0) {
        struct KeyNode *node = &key_nodes[0];
        int found = -1;

        for (int i = 0; i < key_node_count; i++, node++) {
            if(node->eventx == event_index) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            fprintf(stderr, "%s: can't found match: /dev/input/event%d with code %d\n", __func__, event_index, event_code);
            goto exit_failed;
        }

        node = &key_nodes[found];
        for (int j = 0; j < node->code_count; j++) {
            if (node->codes[j].code == event_code) {
                simulate_key_event(
                    node->eventx,
                    node->codes[j].type,
                    node->codes[j].code,
                    duration,
                    (node->codes[j].type == EV_REL) ? direction : 0);
                goto exit_normal;
            }
        }
    } else {
        for (int i = 0; i < key_node_count; i++) {
            struct KeyNode *node = &key_nodes[i];
            for (int j = 0; j < node->code_count; j++) {
                if (node->codes[j].code == event_code) {
                    simulate_key_event(
                        node->eventx,
                        node->codes[j].type,
                        node->codes[j].code,
                        duration,
                        (node->codes[j].type == EV_REL) ? direction : 0);
                    goto exit_normal;
                }
            }
        }
    }

exit_normal:
    if(key_nodes) free(key_nodes);
    return 0;

exit_failed:
    print_help();
    print_nodes();
    if(key_nodes) free(key_nodes);
    return -1;
}
