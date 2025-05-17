#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>

const char *real_path = "/original_files";
const char *log_path = "/var/log/it24.log";

// ROT13 encryption
void apply_rot13(char* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (buf[i] >= 'A' && buf[i] <= 'Z')
            buf[i] = ((buf[i] - 'A' + 13) % 26) + 'A';
        else if (buf[i] >= 'a' && buf[i] <= 'z')
            buf[i] = ((buf[i] - 'a' + 13) % 26) + 'a';
    }
}

// Cek apakah file mencurigakan (mengandung nafis / kimcun)
int is_suspicious(const char *filename) {
    char lower[1024];
    strncpy(lower, filename, sizeof(lower));
    for (int i = 0; lower[i]; i++) lower[i] = tolower(lower[i]);
    return strstr(lower, "nafis") || strstr(lower, "kimcun");
}

// Membalik nama file
void reverse_name(const char *src, char *dst) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; i++) dst[i] = src[len - 1 - i];
    dst[len] = '\0';
}

// Logging
void log_event(const char *level, const char *message) {
    FILE *fp = fopen(log_path, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);

    fprintf(fp, "[%s] [%s] %s\n", level, timebuf, message);
    fclose(fp);
}

static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    int res;
    char fpath[1024];

    const char *relpath = path + 1;
    char realname[1024];
    if (is_suspicious(relpath)) {
        reverse_name(relpath, realname);
    } else {
        strcpy(realname, relpath);
    }
    snprintf(fpath, sizeof(fpath), "%s/%s", real_path, realname);
    res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;
    (void) flags;

    dp = opendir(real_path);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st = {0};
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char display_name[1024];
        if (is_suspicious(de->d_name)) {
            reverse_name(de->d_name, display_name);
            char log_msg[1024];
            snprintf(log_msg, sizeof(log_msg), "File %s has been reversed: %s", de->d_name, display_name);
            log_event("REVERSE", log_msg);
        } else {
            strcpy(display_name, de->d_name);
        }
        filler(buf, display_name, &st, 0, 0);
    }
    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char fpath[1024];
    const char *relpath = path + 1;
    char realname[1024];
    if (is_suspicious(relpath)) {
        reverse_name(relpath, realname);
        char log_msg[1024];
        snprintf(log_msg, sizeof(log_msg), "Anomaly detected %s in file: /%s", strstr(realname, "nafis") ? "nafis" : "kimcun", realname);
        log_event("ALERT", log_msg);
    } else {
        strcpy(realname, relpath);
    }
    snprintf(fpath, sizeof(fpath), "%s/%s", real_path, realname);

    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char fpath[1024];
    const char *relpath = path + 1;
    char realname[1024];

    if (is_suspicious(relpath)) {
        reverse_name(relpath, realname);
    } else {
        strcpy(realname, relpath);
    }
    snprintf(fpath, sizeof(fpath), "%s/%s", real_path, realname);

    int fd = open(fpath, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    close(fd);
    if (res == -1) return -errno;

    if (!is_suspicious(relpath)) {
        apply_rot13(buf, res);
        char log_msg[1024];
        snprintf(log_msg, sizeof(log_msg), "File %s has been encrypted", relpath);
        log_event("ENCRYPT", log_msg);
    } else {
        char log_msg[1024];
        snprintf(log_msg, sizeof(log_msg), "Read Without ROT13: %s", relpath);
        log_event("READ", log_msg);
    }

    return res;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open = antink_open,
    .read = antink_read,
};

int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    return fuse_main(args.argc, args.argv, &antink_oper, NULL);
}

