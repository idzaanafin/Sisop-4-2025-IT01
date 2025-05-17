#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


#define NUM_PARTS 14
#define PART_SIZE 1024
#define FILE_NAME "Baymax.jpeg"
#define LOG_PATH "activity.log"
#define SOURCE_DIR "relics"

static const char *filename = FILE_NAME;

static void log_activity(const char *msg) {
    FILE *log = fopen(LOG_PATH, "a");
    if (log) {
        fprintf(log, "%s\n", msg);
        fclose(log);
    }
}

static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi;
    memset(st, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    } else if (strcmp(path + 1, filename) == 0) {
        st->st_mode = S_IFREG | 0444;
        st->st_nlink = 1;
        st->st_size = NUM_PARTS * PART_SIZE;
    } else {
        return -ENOENT;
    }
    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, filename, NULL, 0, 0);

    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path + 1, filename) != 0)
        return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    log_activity("Opened Baymax.jpeg");
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    (void) fi;

    if (strcmp(path + 1, filename) != 0)
        return -ENOENT;

    if (offset >= NUM_PARTS * PART_SIZE)
        return 0;

    if (offset + size > NUM_PARTS * PART_SIZE)
        size = NUM_PARTS * PART_SIZE - offset;

    size_t bytes_read = 0;
    while (bytes_read < size) {
        int part_index = (offset + bytes_read) / PART_SIZE;
        int part_offset = (offset + bytes_read) % PART_SIZE;
        char part_path[256];
        snprintf(part_path, sizeof(part_path), "%s/%s.%03d", SOURCE_DIR, filename, part_index);

        FILE *part = fopen(part_path, "rb");
        if (!part)
            break;

        fseek(part, part_offset, SEEK_SET);
        size_t to_read = PART_SIZE - part_offset;
        if (to_read > size - bytes_read)
            to_read = size - bytes_read;

        size_t r = fread(buf + bytes_read, 1, to_read, part);
        fclose(part);
        bytes_read += r;
        if (r < to_read) break;
    }

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Read %zu bytes from offset %ld", bytes_read, (long) offset);
    log_activity(log_msg);

    return bytes_read;
}

static const struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &fs_oper, NULL);
}

