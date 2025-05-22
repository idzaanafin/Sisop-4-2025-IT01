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
