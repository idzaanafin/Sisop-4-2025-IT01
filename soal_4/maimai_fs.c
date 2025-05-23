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



