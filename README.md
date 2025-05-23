# LAPORAN PRAKTIKUM SISTEM OPERASI MODUL 4 KELOMPOK IT01

  |       Nama        |     NRP    |
  |-------------------|------------|
  | Ahmad Idza Anafin | 5027241017 |
  | Ivan Syarifuddin  | 5027241045 |
  | Diva Aulia Rosa   | 5027241003 |


# Soal 1
```
#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <curl/curl.h>
#include <zip.h>

#define EXTRACT_DIR "anomali"
#define IMAGE_DIR "image"
#define LOG_FILE "conversion.log"
#define ZIP_FILE "anomali.zip"
#define ZIP_URL "https://drive.usercontent.google.com/u/0/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"

int download_file(const char *url, const char *outfile) {
    CURL *curl;
    FILE *fp = fopen(outfile, "wb");
    CURLcode res = CURLE_OK;

    if (fp) {
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }
        } else {
            fprintf(stderr, "curl_easy_init() failed\n");
            res = CURLE_FAILED_INIT;
        }
        fclose(fp);
    } else {
        fprintf(stderr, "Failed to open/create output file: %s\n", outfile);
        res = CURLE_WRITE_ERROR;
    }
    return (res == CURLE_OK) ? 0 : 1;
}

int extract_zip(const char *zip_path, const char *extract_dir) {
    int res = 0;
    zip_t *zip = zip_open(zip_path, 0, NULL);
    if (!zip) {
        fprintf(stderr, "cannot open zip archive '%s'\n", zip_path);
        return -1;
    }

    mkdir(extract_dir, 0777);

    int num_entries = zip_get_num_entries(zip, 0);
    for (int i = 0; i < num_entries; ++i) {
        const char *name = zip_get_name(zip, i, 0);
        if (name) {
            zip_file_t *zf = zip_fopen_index(zip, i, 0);
            if (!zf) {
                fprintf(stderr, "cannot open zip entry '%s'\n", name);
                res = -1;
                continue;
            }

            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", extract_dir, name);

            char *last_slash = strrchr(full_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                mkdir(full_path, 0777);
                *last_slash = '/';
            }

            FILE *outfile = fopen(full_path, "wb");
            if (!outfile) {
                fprintf(stderr, "cannot open output file '%s'\n", full_path);
                zip_fclose(zf);
                res = -1;
                continue;
            }

            char buf[4096];
            zip_int64_t bytes_read;
            while ((bytes_read = zip_fread(zf, buf, sizeof(buf))) > 0) {
                fwrite(buf, 1, bytes_read, outfile);
            }

            fclose(outfile);
            zip_fclose(zf);
        }
    }
    zip_close(zip);
    unlink(zip_path);
    return res;
}

void hex_to_png(const char *hex_file, const char *output_file) {
    FILE *fin = fopen(hex_file, "r");
    if (!fin) return;

    FILE *fout = fopen(output_file, "wb");
    if (!fout) {
        fclose(fin);
        return;
    }

    fprintf(fout, "P5\n");
    int width = 256;
    int height = 256;
    fprintf(fout, "%d %d\n", width, height);
    fprintf(fout, "255\n");

    int c, i = 0;
    unsigned char byte_buffer[width * height];
    size_t byte_count = 0;
    char hex[3] = {0};

    while ((c = fgetc(fin)) != EOF && byte_count < sizeof(byte_buffer)) {
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            hex[i++] = c;
            if (i == 2) {
                unsigned char byte = (unsigned char)strtol(hex, NULL, 16);
                byte_buffer[byte_count++] = byte;
                i = 0;
                hex[0] = 0;
                hex[1] = 0;
            }
        }
    }

    fwrite(byte_buffer, 1, byte_count, fout);

    fclose(fin);
    fclose(fout);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "convert %s %s", output_file, output_file);
    system(cmd);
}

void log_conversion(const char *src_name, const char *dst_name) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char date[11], timebuf[9];
    strftime(date, sizeof(date), "%Y-%m-%d", t);
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);

    fprintf(log, "[%s][%s]: Successfully converted hexadecimal text %s to %s\n",
            date, timebuf, src_name, dst_name);
    fclose(log);
}

void process_hex_files() {
    DIR *dir = opendir(EXTRACT_DIR);
    if (!dir) {
        mkdir(EXTRACT_DIR, 0777);
        dir = opendir(EXTRACT_DIR);
        if (!dir) {
            perror("Failed to open extract directory");
            return;
        }
    }

    mkdir(IMAGE_DIR, 0777);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".txt") == 0) {
                char input_path[512], output_base_untruncated[512], output_base[256], output_path_pgm[512];
                snprintf(input_path, sizeof(input_path), "%s/%s", EXTRACT_DIR, entry->d_name);

                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                char timestamp[32];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", t);

                char base_name[256];
                strncpy(base_name, entry->d_name, strcspn(entry->d_name, "."));
                base_name[strcspn(base_name, ".")] = '\0';

                snprintf(output_base_untruncated, sizeof(output_base_untruncated), "%s_image_%s", base_name, timestamp);

                size_t max_base_len = sizeof(output_base) - 1;
                strncpy(output_base, output_base_untruncated, max_base_len);
                output_base[max_base_len] = '\0';
                if (strlen(output_base_untruncated) > max_base_len) {
                    strcat(output_base, "...");
                }

                snprintf(output_path_pgm, sizeof(output_path_pgm), "%s/%s.pgm", IMAGE_DIR, output_base);

                hex_to_png(input_path, output_path_pgm);
                log_conversion(entry->d_name, output_base);
                sleep(1);
            }
        }
    }
    closedir(dir);
}

void download_and_extract() {
    mkdir(EXTRACT_DIR, 0777);
    printf("Downloading %s to %s...\n", ZIP_URL, ZIP_FILE);
    if (download_file(ZIP_URL, ZIP_FILE) == 0) {
        printf("Download successful. Extracting...\n");
        if (extract_zip(ZIP_FILE, EXTRACT_DIR) == 0) {
            printf("Extraction successful.\n");
        } else {
            fprintf(stderr, "Error during zip extraction.\n");
        }
    } else {
        fprintf(stderr, "Error during download.\n");
    }
}

static int getattr_cb(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } else if (strcmp(path, "/image") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    } else {
        char real_path[512];
        if (strncmp(path, "/image/", strlen("/image/")) == 0) {
            snprintf(real_path, sizeof(real_path), "%s/%s", IMAGE_DIR, path + strlen("/image/"));
        } else if (strlen(path) > 1) {
            if (strcmp(path + 1, LOG_FILE) == 0) {
                snprintf(real_path, sizeof(real_path), "%s", LOG_FILE);
            } else {
                char extracted_file[256];
                strncpy(extracted_file, path + 1, sizeof(extracted_file) - 1);
                extracted_file[sizeof(extracted_file) - 1] = '\0';
                snprintf(real_path, sizeof(real_path), "%s/%s", EXTRACT_DIR, extracted_file);
            }
        } else {
            return -ENOENT;
        }

        if (access(real_path, F_OK) == 0) {
            if (strncmp(path, "/image/", strlen("/image/")) == 0) {
                stbuf->st_mode = S_IFREG | 0644;
                stbuf->st_nlink = 1;
                stat(real_path, stbuf);
                return 0;
            } else if (strcmp(path + 1, LOG_FILE) == 0) {
                stbuf->st_mode = S_IFREG | 0644;
                stbuf->st_nlink = 1;
                stat(real_path, stbuf);
                return 0;
            } else {
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stat(real_path, stbuf);
                return 0;
            }
        }
        return -ENOENT;
    }
}

static int readdir_cb(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (strcmp(path, "/") == 0) {
        DIR *d_extract = opendir(EXTRACT_DIR);
        if (d_extract) {
            struct dirent *de;
            while ((de = readdir(d_extract)) != NULL) {
                if (de->d_type == DT_REG && strstr(de->d_name, ".txt")) {
                    filler(buf, de->d_name, NULL, 0, 0);
                }
            }
            closedir(d_extract);
        }
        if (access(LOG_FILE, F_OK) == 0) {
            filler(buf, LOG_FILE, NULL, 0, 0);
        }
        filler(buf, "image", NULL, 0, 0);
    } else if (strcmp(path, "/image") == 0) {
        DIR *d_image = opendir(IMAGE_DIR);
        if (d_image) {
            struct dirent *de;
            while ((de = readdir(d_image)) != NULL) {
                if (de->d_type == DT_REG && strstr(de->d_name, "_image_")) {
                    filler(buf, de->d_name, NULL, 0, 0);
                }
            }
            closedir(d_image);
        } else {
            return -errno;
        }
    }

    return 0;
}

static int open_cb(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/" LOG_FILE) == 0) {
        if ((fi->flags & O_ACCMODE) != O_RDONLY)
            return -EACCES;
    } else if (strncmp(path, "/image/", strlen("/image/")) == 0) {
        if ((fi->flags & O_ACCMODE) != O_RDONLY)
            return -EACCES;
    } else if (strlen(path) > 1 && strstr(path, ".txt")) {
        if ((fi->flags & O_ACCMODE) != O_RDONLY)
            return -EACCES;
    }
    return 0;
}

static int read_cb(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    FILE *fp = NULL;
    size_t res = 0;
    char real_path[512];

    if (strcmp(path, "/" LOG_FILE) == 0) {
        snprintf(real_path, sizeof(real_path), "%s", LOG_FILE);
        fp = fopen(real_path, "r");
    } else if (strncmp(path, "/image/", strlen("/image/")) == 0) {
        snprintf(real_path, sizeof(real_path), "%s/%s", IMAGE_DIR, path + strlen("/image/"));
        fp = fopen(real_path, "rb");
    } else if (strlen(path) > 1 && strstr(path, ".txt")) {
        char extracted_file[256];
        strncpy(extracted_file, path + 1, sizeof(extracted_file) - 1);
        extracted_file[sizeof(extracted_file) - 1] = '\0';
        snprintf(real_path, sizeof(real_path), "%s/%s", EXTRACT_DIR, extracted_file);
        fp = fopen(real_path, "r");
    } else {
        return -ENOENT;
    }

    if (fp) {
        fseek(fp, offset, SEEK_SET);
        res = fread(buf, 1, size, fp);
        fclose(fp);
    }
    return res;
}

static const struct fuse_operations shorekeeper_ops = {
    .getattr = getattr_cb,
    .readdir = readdir_cb,
    .open    = open_cb,
    .read    = read_cb,
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <mountpoint>\n", argv[0]);
        return 1;
    }

    download_and_extract();
    process_hex_files();

    return fuse_main(argc, argv, &shorekeeper_ops, NULL);
}

```
TIDAK BISA SELESAI KARENA PERMISSION DENIED 
# Soal 2
Full Code
## Baymax.c
```
#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>

#define RELICS_DIR "relics"
#define LOG_FILE "activity.log"
#define MAX_FRAGMENT_SIZE 1024
#define MAX_FRAGMENTS 1000

void log_activity(const char *fmt, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(log, fmt, args);
    fprintf(log, "\n");
    va_end(args);
    fclose(log);
}

static int fs_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    memset(st, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    } else {
        char base[256];
        snprintf(base, sizeof(base), "%s", path + 1);
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.000", RELICS_DIR, base);

        FILE *f = fopen(frag_path, "rb");
        if (!f) return -ENOENT;

        st->st_mode = S_IFREG | 0444;
        st->st_nlink = 1;

        size_t total_size = 0;
        for (int i = 0; i < MAX_FRAGMENTS; i++) {
            snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELICS_DIR, base, i);
            FILE *part = fopen(frag_path, "rb");
            if (!part) break;
            fseek(part, 0, SEEK_END);
            total_size += ftell(part);
            fclose(part);
        }
        st->st_size = total_size;
        return 0;
    }
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *dir = opendir(RELICS_DIR);
    if (!dir) return -ENOENT;

    struct dirent *dp;
    char listed[256][256];
    int listed_count = 0;

    while ((dp = readdir(dir)) != NULL) {
        if (strchr(dp->d_name, '.') == NULL) continue;
        char name[256];
        strncpy(name, dp->d_name, sizeof(name));
        char *dot = strrchr(name, '.');
        if (dot && strlen(dot) == 4 && isdigit(dot[1])) {
            *dot = '\0';
            int already_listed = 0;
            for (int i = 0; i < listed_count; i++) {
                if (strcmp(listed[i], name) == 0) {
                    already_listed = 1;
                    break;
                }
            }
            if (!already_listed) {
                strcpy(listed[listed_count++], name);
                filler(buf, name, NULL, 0, 0);
            }
        }
    }
    closedir(dir);
    return 0;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
    (void) fi;
    char base[256];
    snprintf(base, sizeof(base), "%s", path + 1);
    char frag_path[512];
    snprintf(frag_path, sizeof(frag_path), "%s/%s.000", RELICS_DIR, base);

    FILE *f = fopen(frag_path, "rb");
    if (!f) return -ENOENT;
    fclose(f);

    log_activity("READ: %s", base);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t bytes_read = 0;
    char base[256];
    snprintf(base, sizeof(base), "%s", path + 1);

    int start_frag = offset / MAX_FRAGMENT_SIZE;
    int start_offset = offset % MAX_FRAGMENT_SIZE;

    for (int i = start_frag; bytes_read < size; i++) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELICS_DIR, base, i);
        FILE *part = fopen(frag_path, "rb");
        if (!part) break;

        fseek(part, (i == start_frag) ? start_offset : 0, SEEK_SET);
        size_t to_read = size - bytes_read;
        if (to_read > MAX_FRAGMENT_SIZE) to_read = MAX_FRAGMENT_SIZE;

        size_t r = fread(buf + bytes_read, 1, to_read, part);
        bytes_read += r;
        fclose(part);

        if (r < to_read) break;
    }
    return bytes_read;
}

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;
    (void) fi;
    return 0; // handled in write
}

static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    if (offset != 0) return 0; // write only at creation time

    char base[256];
    snprintf(base, sizeof(base), "%s", path + 1);

    int num_parts = 0;
    for (size_t i = 0; i < size; i += MAX_FRAGMENT_SIZE, num_parts++) {
        char frag_path[512];
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELICS_DIR, base, num_parts);
        FILE *f = fopen(frag_path, "wb");
        if (!f) return -EIO;
        size_t to_write = (size - i > MAX_FRAGMENT_SIZE) ? MAX_FRAGMENT_SIZE : size - i;
        fwrite(buf + i, 1, to_write, f);
        fclose(f);
    }

    log_activity("WRITE: %s -> %s.000 to %s.%03d", base, base, base, num_parts - 1);
    return size;
}

static int fs_unlink(const char *path) {
    char base[256];
    snprintf(base, sizeof(base), "%s", path + 1);
    char frag_path[512];

    int i;
    for (i = 0; i < MAX_FRAGMENTS; i++) {
        snprintf(frag_path, sizeof(frag_path), "%s/%s.%03d", RELICS_DIR, base, i);
        if (unlink(frag_path) != 0) break;
    }

    if (i == 0) return -ENOENT;

    log_activity("DELETE: %s.000 - %s.%03d", base, base, i - 1);
    return 0;
}

static const struct fuse_operations operations = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open    = fs_open,
    .read    = fs_read,
    .write   = fs_write,
    .create  = fs_create,
    .unlink  = fs_unlink,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &operations, NULL);
}

```
Soal ini meminta kita untuk menjankan sebuah sistem FUSE untuk menggabungkan dan memecah file 

### struktur :
```
├── Baymax_output.jpeg
├── activity.log
├── baymax.c
├── baymaxfs
├── mount_dir
│   └── Baymax.jpeg
└── relics
    ├── Baymax.jpeg.000
    ├── Baymax.jpeg.001
    ├── Baymax.jpeg.002
    ├── Baymax.jpeg.003
    ├── Baymax.jpeg.004
    ├── Baymax.jpeg.005
    ├── Baymax.jpeg.006
    ├── Baymax.jpeg.007
    ├── Baymax.jpeg.008
    ├── Baymax.jpeg.009
    ├── Baymax.jpeg.010
    ├── Baymax.jpeg.011
    ├── Baymax.jpeg.012
    └── Baymax.jpeg.013
```
## Penjelasan kode: 
### logging
```
void log_activity(const char *fmt, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(log, fmt, args);
    fprintf(log, "\n");
    va_end(args);
    fclose(log);
}
```


# Soal 3

### Dockerfile
```

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    gcc make libfuse3-dev fuse3 \
    && rm -rf /var/lib/apt/lists/*


COPY antink.c /app/antink.c

WORKDIR /app

RUN gcc antink.c -o antink -lfuse3

CMD bash -c "mkdir -p /antink_mount && ./antink /antink_mount & tail -f /var/log/it24.log"

```

### Docker-compose.yml
```

services:
  antink:
    build:
      context: .
      dockerfile: Dockerfile
    privileged: true
    cap_add:
      - SYS_ADMIN
    security_opt:
      - apparmor:unconfined
    volumes:
      - ./it24_host:/original_files:ro    
      - ./antink_mount:/antink_mount          
      - ./antink-logs:/var/log            
    devices:
      - /dev/fuse
    container_name: antink
    restart: always

```
### antink.c
```
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

```

### pada soal ini kita diperlukan untuk menjalankan FUSE melalui docker

- ![image](https://github.com/user-attachments/assets/d4dca1bd-3d46-4ca7-b131-60aecd018eab)
  Struktur Directory

### Lihat file via docker 
  Disini file yang ada "nafis" atau "kimcun" nama dari filenya akan dibalik 
  ![image](https://github.com/user-attachments/assets/26a7b5dd-852c-4876-ae79-ddb739dcbb1c)

### Enkripsi ROT13 untuk isi file yang bersih
  Jika file tidak mengandung "nafis" atau "kimcun" untuk penamaan maka isi file akan di enkripsi ROT13

  ![image](https://github.com/user-attachments/assets/284eebd6-6523-490b-9762-056ec4e069f5)

### Cek log
  ![image](https://github.com/user-attachments/assets/7abec00e-a3e2-453b-a4f7-23fcf1c6c71f)



# Soal 4
pada intinya membuat fuse dir yang memiliki fungsionalitas transform data sesuai subdirektorinya dengan constraint tertenttu
## Full code
   ```
  #define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <zlib.h>


static const char *dirpath = "/home/idzoyy/sisop-it24/Sisop-4-2025-IT01/soal_4/chiho";

int is_in_starter(const char *path) {
    return strncmp(path, "/starter/", 9) == 0;
}

int is_in_blackrose(const char *path) {
    return strncmp(path, "/blackrose/", 11) == 0;
}
int is_in_dragon(const char *path) {
    return strncmp(path, "/dragon/", 8) == 0;
}
int is_in_heaven(const char *path) {
    return strncmp(path, "/heaven/", 8) == 0;
}
int is_in_metro(const char *path) {
    return strncmp(path, "/metro/", 7) == 0;
}
int is_in_skystreet(const char *path) {
    return strncmp(path, "/skystreet/", 11) == 0;
}
int is_in_7sref(const char *path) {
    return strncmp(path, "/7sref/", 7) == 0;
}

void append_ext_if_needed(char *fpath, const char *path) {
    if (is_in_starter(path)) {
        sprintf(fpath, "%s%s.mai", dirpath, path);
    } else if (is_in_blackrose(path)) {
        sprintf(fpath, "%s%s.bin", dirpath, path);
    } else if (is_in_dragon(path)) {
        sprintf(fpath, "%s%s.rot", dirpath, path);
    } else if (is_in_heaven(path)) {
        sprintf(fpath, "%s%s.enc", dirpath, path);
    } else if (is_in_metro(path)) {
        sprintf(fpath, "%s%s.ccc", dirpath, path);
    } else if (is_in_skystreet(path)) {
        sprintf(fpath, "%s%s.gz", dirpath, path);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }
}


void strip_ext_if_needed(char *newname, const char *filename) {
    const char *last_dot = strrchr(filename, '.');
    if (last_dot != NULL) {
        size_t len = last_dot - filename;
        strncpy(newname, filename, len);
        newname[len] = '\0';
    } else {
        strcpy(newname, filename);
    }
}

void strip_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if (dot) *dot = '\0';
}


static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1000];

    if (strcmp(path, "/7sref") == 0) {
        memset(stbuf, 0, sizeof(struct stat));
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    append_ext_if_needed(fpath, path);
    if (strncmp(path, "/7sref/", 7) == 0) {
        const char *filename = path + 7;
        char folder[256], file[256];

        if (sscanf(filename, "%[^_]_%s", folder, file) == 2) {
            char realpath[1024];
            snprintf(realpath, sizeof(realpath), "%s/%s/%s", dirpath, folder, file);
            // snprintf(realpath, sizeof(realpath), "%s/%s",dirpath,folder);
            // snprintf(realpath, sizeof(realpath), "%s/starter/a.txt.mai", dirpath);
            // strip_ext(realpath);
            struct stat st;
            if (stat(realpath, &st) == -1) return -errno;
            *stbuf = st;
            return 0;
        }
    }

    res = lstat(fpath, stbuf);
    if (res == -1) return -errno;
    return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    if (strcmp(path, "/") == 0) {
        sprintf(fpath, "%s", dirpath);
    } else {
        sprintf(fpath, "%s%s", dirpath, path);
    }

    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") == 0) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFDIR | 0755;
        filler(buf, "7sref", &st, 0);
    }

    if (strcmp(path, "/7sref") == 0) {
        DIR *dp_root = opendir(dirpath);
        if (dp_root == NULL) return -errno;

        struct dirent *de_folder;
        while ((de_folder = readdir(dp_root)) != NULL) {
            if (de_folder->d_type != DT_DIR) continue;

            if (strcmp(de_folder->d_name, ".") == 0 || strcmp(de_folder->d_name, "..") == 0) continue;
            if (strcmp(de_folder->d_name, "7sref") == 0) continue;

            char subdir_path[1024];
            snprintf(subdir_path, sizeof(subdir_path), "%s/%s", dirpath, de_folder->d_name);

            DIR *dp_sub = opendir(subdir_path);
            if (dp_sub == NULL) continue;

            struct dirent *de_file;
            while ((de_file = readdir(dp_sub)) != NULL) {
                if (de_file->d_type != DT_REG) continue;

                char virtual_name[1024];
                snprintf(virtual_name, sizeof(virtual_name), "%s_%s", de_folder->d_name, de_file->d_name);
                strip_ext(virtual_name);
                filler(buf, virtual_name, NULL, 0);
            }

            closedir(dp_sub);
        }

        closedir(dp_root);
        return 0;
    }
    dp = opendir(fpath);
    if (dp == NULL) return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char namebuf[1000];
        strip_ext_if_needed(namebuf, de->d_name);
        if (strlen(namebuf) == 0) continue;

        filler(buf, namebuf, &st, 0);
        
    }

    closedir(dp);
    return 0;
}



static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    append_ext_if_needed(fpath, path);
    int res = 0;
    (void) fi;

    if (strncmp(path, "/7sref/", 7) == 0) {
        const char *filename = fpath + 7;
        char folder[256], file[256];

        if (sscanf(filename, "%[^_]_%s", folder, file) == 2) {
            char realpath[1024];
            snprintf(realpath, sizeof(realpath), "%s/%s/%s", dirpath, folder, file);
            // snprintf(realpath, sizeof(realpath), "%s", fpath);
            // snprintf(realpath, sizeof(realpath), "%s/starter/a.txt.mai", dirpath);
            int fd = open(realpath, O_RDONLY);
            if (fd == -1) return -errno;
            int res = pread(fd, buf, size, offset);
            if (res == -1) res = -errno;
            close(fd);
            return res;
        }
    }

    
    // Starter -> baca langsung
    if (is_in_starter(path)) {
        int fd = open(fpath, O_RDONLY);
        if (fd == -1) return -errno;
        res = pread(fd, buf, size, offset);
        if (res == -1) res = -errno;
        close(fd);
        return res;
    }

    // Blackrose -> baca byte mentah langsung
    else if (is_in_blackrose(path)) {
        FILE *fp = fopen(fpath, "rb");
        if (!fp) return -errno;
        fseek(fp, offset, SEEK_SET);
        res = fread(buf, 1, size, fp);
        fclose(fp);
        return res;
    }

    // Dragon -> ROT13
    else if (is_in_dragon(path)) {
        char *enc_buf = malloc(size);
        if (!enc_buf) return -ENOMEM;
        FILE *fp = fopen(fpath, "rb");
        if (!fp) {
            free(enc_buf);
            return -errno;
        }
        fseek(fp, offset, SEEK_SET);
        res = fread(enc_buf, 1, size, fp);
        fclose(fp);

        for (size_t i = 0; i < res; i++) {
            char c = enc_buf[i];
            if (c >= 'A' && c <= 'Z')
                buf[i] = ((c - 'A' + 13) % 26) + 'A';
            else if (c >= 'a' && c <= 'z')
                buf[i] = ((c - 'a' + 13) % 26) + 'a';
            else
                buf[i] = c;
        }
        free(enc_buf);
        return res;
    }

    // Heaven -> AES-256-CBC decrypt
    else if (is_in_heaven(path)) {
        unsigned char key[32] = "thisis32bytepasswordforaesencrypt!"; 
        unsigned char iv[16];
        FILE *fp = fopen(fpath, "rb");
        if (!fp) return -errno;

        fread(iv, 1, sizeof(iv), fp); // baca IV
        fseek(fp, 0, SEEK_END);
        long filesize = ftell(fp);
        long ciphertext_len = filesize - sizeof(iv);
        fseek(fp, sizeof(iv) + offset, SEEK_SET);

        unsigned char *ciphertext = malloc(ciphertext_len);
        unsigned char *plaintext = malloc(ciphertext_len);
        if (!ciphertext || !plaintext) {
            fclose(fp);
            free(ciphertext);
            free(plaintext);
            return -ENOMEM;
        }

        res = fread(ciphertext, 1, ciphertext_len, fp);
        fclose(fp);

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            free(ciphertext);
            free(plaintext);
            return -ENOMEM;
        }

        int len, plaintext_len = 0;
        EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, res);
        plaintext_len = len;
        EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
        plaintext_len += len;

        if (offset < plaintext_len) {
            int available = plaintext_len - offset;
            int to_copy = (available < size) ? available : size;
            memcpy(buf, plaintext + offset, to_copy);
            res = to_copy;
        } else {
            res = 0;
        }

        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext);
        free(plaintext);
        return res;
    }

    // Metro -> undo karakter shift berdasarkan indeks
    else if (is_in_metro(path)) {
        char *enc_buf = malloc(size);
        if (!enc_buf) return -ENOMEM;

        FILE *fp = fopen(fpath, "rb");
        if (!fp) {
            free(enc_buf);
            return -errno;
        }

        fseek(fp, offset, SEEK_SET);
        res = fread(enc_buf, 1, size, fp);
        fclose(fp);

        for (size_t i = 0; i < res; i++) {
            buf[i] = enc_buf[i] - i % 256;
        }

        free(enc_buf);
        return res;
    }

    else if (is_in_skystreet(path)) {
        gzFile gzfp = gzopen(fpath, "rb");
        if (!gzfp) return -errno;

        uLongf decomp_size = size * 4;
        unsigned char *decomp_buf = malloc(decomp_size);
        if (!decomp_buf) {
            gzclose(gzfp);
            return -ENOMEM;
        }

        // Baca hasil dekompresi dari file gzip
        int bytes_read = gzread(gzfp, decomp_buf, decomp_size);
        if (bytes_read < 0) {
            free(decomp_buf);
            gzclose(gzfp);
            return -EIO;
        }

        gzclose(gzfp);

        if (offset < bytes_read) {
            int available = bytes_read - offset;
            int to_copy = (available < size) ? available : size;
            memcpy(buf, decomp_buf + offset, to_copy);
            res = to_copy;
        } else {
            res = 0;
        }

        free(decomp_buf);
        return res;
    }


    return -EINVAL; 
}


static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    append_ext_if_needed(fpath, path);

    int res = 0;
    (void) fi;        

    // Starter -> tulis langsung
    if (is_in_starter(path)) {
        int fd = open(fpath, O_WRONLY);
        if (fd == -1) return -errno;
        res = pwrite(fd, buf, size, offset);
        if (res == -1) res = -errno;
        close(fd);
        return res;
    }

    // Blackrose -> simpan byte mentah
    else if (is_in_blackrose(path)) {
        FILE *fp = fopen(fpath, "wb");
        if (!fp) return -errno;
        res = fwrite(buf, 1, size, fp);
        fclose(fp);
        return res;
    }

    // Dragon -> ROT13
    else if (is_in_dragon(path)) {
        char *rot_buf = malloc(size);
        if (!rot_buf) return -ENOMEM;
        for (size_t i = 0; i < size; i++) {
            char c = buf[i];
            if ((c >= 'A' && c <= 'Z'))
                rot_buf[i] = ((c - 'A' + 13) % 26) + 'A';
            else if ((c >= 'a' && c <= 'z'))
                rot_buf[i] = ((c - 'a' + 13) % 26) + 'a';
            else
                rot_buf[i] = c;
        }
        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(rot_buf);
            return -errno;
        }
        res = fwrite(rot_buf, 1, size, fp);
        fclose(fp);
        free(rot_buf);
        return res;
    }

    // Heaven -> AES-256-CBC
    else if (is_in_heaven(path)) {
        unsigned char key[32] = "thisis32bytepasswordforaesencrypt!"; 
        unsigned char iv[16];
        RAND_bytes(iv, sizeof(iv));

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -ENOMEM;

        unsigned char *ciphertext = malloc(size + EVP_MAX_BLOCK_LENGTH);
        int len, ciphertext_len;

        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char *)buf, size);
        ciphertext_len = len;
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
        ciphertext_len += len;

        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(ciphertext);
            EVP_CIPHER_CTX_free(ctx);
            return -errno;
        }

        fwrite(iv, 1, sizeof(iv), fp);  
        fwrite(ciphertext, 1, ciphertext_len, fp);
        fclose(fp);

        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return ciphertext_len + sizeof(iv);
    }

    // Metro -> shift karakter berdasarkan indeks
    else if (is_in_metro(path)) {
        char *shift_buf = malloc(size);
        if (!shift_buf) return -ENOMEM;
        for (size_t i = 0; i < size; i++) {
            shift_buf[i] = buf[i] + i % 256;
        }
        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(shift_buf);
            return -errno;
        }
        res = fwrite(shift_buf, 1, size, fp);
        fclose(fp);
        free(shift_buf);
        return res;
    }

    
    else if (is_in_skystreet(path)) {
        gzFile gzfp = gzopen(fpath, "wb");
        if (!gzfp) return -errno;

    
        int written = gzwrite(gzfp, buf, size);
        if (written == 0) {
            gzclose(gzfp);
            return -EIO;
        }

        gzclose(gzfp);
        return written;
    }


    return -EINVAL;
}


static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1000];
    int res = 0;
    (void) fi;
    append_ext_if_needed(fpath, path);
    res = creat(fpath, mode);
    if (res == -1) return -errno;

    close(res);
    return 0;
}

static int xmp_unlink(const char *path) {
    char fpath[1000];
    int res = 0;

    append_ext_if_needed(fpath, path);

    res = unlink(fpath);
    if (res == -1) return -errno;

    return 0;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .read = xmp_read,
    .write = xmp_write,
    .create = xmp_create,
    .unlink = xmp_unlink,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
   ```

## blackrose
transformasi menjadi raw bytes langsung dengan ekstensi .bin
#### write 
    
    else if (is_in_blackrose(path)) {
        FILE *fp = fopen(fpath, "wb");
        if (!fp) return -errno;
        res = fwrite(buf, 1, size, fp);
        fclose(fp);
        return res;
    }
    
#### read
    
    else if (is_in_blackrose(path)) {
        FILE *fp = fopen(fpath, "rb");
        if (!fp) return -errno;
        fseek(fp, offset, SEEK_SET);
        res = fread(buf, 1, size, fp);
        fclose(fp);
        return res;
    }
    
![image](https://github.com/user-attachments/assets/e94d77a2-49f4-45ba-9327-651a7fb48129)
![image](https://github.com/user-attachments/assets/8c274572-0793-4ceb-8725-d754911782a8)

## dragon
transformasi ROT13 dan ekstensi menjadi .rot
#### write 
    
    else if (is_in_dragon(path)) {
        char *rot_buf = malloc(size);
        if (!rot_buf) return -ENOMEM;
        for (size_t i = 0; i < size; i++) {
            char c = buf[i];
            if ((c >= 'A' && c <= 'Z'))
                rot_buf[i] = ((c - 'A' + 13) % 26) + 'A';
            else if ((c >= 'a' && c <= 'z'))
                rot_buf[i] = ((c - 'a' + 13) % 26) + 'a';
            else
                rot_buf[i] = c;
        }
        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(rot_buf);
            return -errno;
        }
        res = fwrite(rot_buf, 1, size, fp);
        fclose(fp);
        free(rot_buf);
        return res;
    }
    
#### read
    
    else if (is_in_dragon(path)) {
        char *enc_buf = malloc(size);
        if (!enc_buf) return -ENOMEM;
        FILE *fp = fopen(fpath, "rb");
        if (!fp) {
            free(enc_buf);
            return -errno;
        }
        fseek(fp, offset, SEEK_SET);
        res = fread(enc_buf, 1, size, fp);
        fclose(fp);

        for (size_t i = 0; i < res; i++) {
            char c = enc_buf[i];
            if (c >= 'A' && c <= 'Z')
                buf[i] = ((c - 'A' + 13) % 26) + 'A';
            else if (c >= 'a' && c <= 'z')
                buf[i] = ((c - 'a' + 13) % 26) + 'a';
            else
                buf[i] = c;
        }
        free(enc_buf);
        return res;
    }
    
  ![image](https://github.com/user-attachments/assets/0820d50e-974f-44c9-aacf-04146afe989c)

## heaven
transformasi enkripsi AES CBC, dan ekstensi .enc
#### write 
    
    else if (is_in_heaven(path)) {
        unsigned char key[32] = "thisis32bytepasswordforaesencrypt!"; 
        unsigned char iv[16];
        RAND_bytes(iv, sizeof(iv));

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -ENOMEM;

        unsigned char *ciphertext = malloc(size + EVP_MAX_BLOCK_LENGTH);
        int len, ciphertext_len;

        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char *)buf, size);
        ciphertext_len = len;
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
        ciphertext_len += len;

        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(ciphertext);
            EVP_CIPHER_CTX_free(ctx);
            return -errno;
        }

        fwrite(iv, 1, sizeof(iv), fp);  
        fwrite(ciphertext, 1, ciphertext_len, fp);
        fclose(fp);

        free(ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return ciphertext_len + sizeof(iv);
    }
    
#### read
    
    else if (is_in_heaven(path)) {
        unsigned char key[32] = "thisis32bytepasswordforaesencrypt!"; 
        unsigned char iv[16];
        FILE *fp = fopen(fpath, "rb");
        if (!fp) return -errno;

        fread(iv, 1, sizeof(iv), fp); // baca IV
        fseek(fp, 0, SEEK_END);
        long filesize = ftell(fp);
        long ciphertext_len = filesize - sizeof(iv);
        fseek(fp, sizeof(iv) + offset, SEEK_SET);

        unsigned char *ciphertext = malloc(ciphertext_len);
        unsigned char *plaintext = malloc(ciphertext_len);
        if (!ciphertext || !plaintext) {
            fclose(fp);
            free(ciphertext);
            free(plaintext);
            return -ENOMEM;
        }

        res = fread(ciphertext, 1, ciphertext_len, fp);
        fclose(fp);

        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            free(ciphertext);
            free(plaintext);
            return -ENOMEM;
        }

        int len, plaintext_len = 0;
        EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, res);
        plaintext_len = len;
        EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
        plaintext_len += len;

        if (offset < plaintext_len) {
            int available = plaintext_len - offset;
            int to_copy = (available < size) ? available : size;
            memcpy(buf, plaintext + offset, to_copy);
            res = to_copy;
        } else {
            res = 0;
        }

        EVP_CIPHER_CTX_free(ctx);
        free(ciphertext);
        free(plaintext);
        return res;
    }
    
  ![image](https://github.com/user-attachments/assets/69d1d04f-6330-4ce6-a1a4-270532f5b422)

## metro
transsformasi shift sesuai index, dan ekstensi .ccc
#### write 
    
    else if (is_in_metro(path)) {
        char *shift_buf = malloc(size);
        if (!shift_buf) return -ENOMEM;
        for (size_t i = 0; i < size; i++) {
            shift_buf[i] = buf[i] + i % 256;
        }
        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(shift_buf);
            return -errno;
        }
        res = fwrite(shift_buf, 1, size, fp);
        fclose(fp);
        free(shift_buf);
        return res;
    }
    
#### read
    
    else if (is_in_metro(path)) {
        char *enc_buf = malloc(size);
        if (!enc_buf) return -ENOMEM;

        FILE *fp = fopen(fpath, "rb");
        if (!fp) {
            free(enc_buf);
            return -errno;
        }

        fseek(fp, offset, SEEK_SET);
        res = fread(enc_buf, 1, size, fp);
        fclose(fp);

        for (size_t i = 0; i < res; i++) {
            buf[i] = enc_buf[i] - i % 256;
        }

        free(enc_buf);
        return res;
    }
    
  ![image](https://github.com/user-attachments/assets/bfa23353-7534-4be0-a544-1861c9ce9cb1)

## skystreet
transformasi compress file dengan ekstensi .gz
#### write 
    
    else if (is_in_skystreet(path)) {
        gzFile gzfp = gzopen(fpath, "wb");
        if (!gzfp) return -errno;

        // gzwrite mengembalikan jumlah *byte* yang berhasil ditulis
        int written = gzwrite(gzfp, buf, size);
        if (written == 0) {
            gzclose(gzfp);
            return -EIO;
        }

        gzclose(gzfp);
        return written;
    }
    
#### read
    
    else if (is_in_skystreet(path)) {
      gzFile gzfp = gzopen(fpath, "rb");
      if (!gzfp) return -errno;
  
      uLongf decomp_size = size * 4;
      unsigned char *decomp_buf = malloc(decomp_size);
      if (!decomp_buf) {
          gzclose(gzfp);
          return -ENOMEM;
    }
    
  ![image](https://github.com/user-attachments/assets/0b6af0bd-6b00-4f85-8936-a6653c6456f1)

## starter
transformasi hanya merubah ekstensi dan isi file masih sama
#### write 
    
    if (is_in_starter(path)) {
        int fd = open(fpath, O_WRONLY);
        if (fd == -1) return -errno;
        res = pwrite(fd, buf, size, offset);
        if (res == -1) res = -errno;
        close(fd);
        return res;
    }
    
#### read
    
    if (is_in_starter(path)) {
        int fd = open(fpath, O_RDONLY);
        if (fd == -1) return -errno;
        res = pread(fd, buf, size, offset);
        if (res == -1) res = -errno;
        close(fd);
        return res;
    }
    ```
![image](https://github.com/user-attachments/assets/a86bebef-317f-431f-99da-3443e9887ed6)
