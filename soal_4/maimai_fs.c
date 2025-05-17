// #define FUSE_USE_VERSION 31
// #include <fuse3/fuse.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <dirent.h>
// #include <unistd.h>
// #include <sys/stat.h>

// #define BASE_PATH "./chiho"
// #define STARTER_EXT ".mai"
// #define MAX_PATH 1024


// void starter_real_path(char *fpath, const char *path) {    
//     if (strncmp(path, "/starter/", 9) == 0) {
//         snprintf(fpath, MAX_PATH, "%s%s%s", BASE_PATH, path, STARTER_EXT);
//     } else {
//         snprintf(fpath, MAX_PATH, "%s%s", BASE_PATH, path);
//     }
// }

// static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);
//     int res = lstat(fpath, stbuf);
//     return res == -1 ? -errno : 0;
// }

// static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
//                       off_t offset, struct fuse_file_info *fi,
//                       enum fuse_readdir_flags flags) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);

//     DIR *dp = opendir(fpath);
//     if (!dp) return -errno;

//     filler(buf, ".", NULL, 0, 0);
//     filler(buf, "..", NULL, 0, 0);

//     struct dirent *de;
//     while ((de = readdir(dp)) != NULL) {
//         if (strncmp(path, "/starter", 8) == 0) {
            
//             if (strstr(de->d_name, STARTER_EXT)) {
//                 char name[NAME_MAX];
//                 strncpy(name, de->d_name, strlen(de->d_name) - strlen(STARTER_EXT));
//                 name[strlen(de->d_name) - strlen(STARTER_EXT)] = '\0';
//                 filler(buf, name, NULL, 0, 0);
//             }
//         } else {
//             filler(buf, de->d_name, NULL, 0, 0);
//         }
//     }

//     closedir(dp);
//     return 0;
// }

// static int fs_open(const char *path, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);
//     int fd = open(fpath, fi->flags);
//     if (fd == -1) return -errno;
//     close(fd);
//     return 0;
// }

// static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);
//     int fd = open(fpath, O_RDONLY);
//     if (fd == -1) return -errno;
//     int res = pread(fd, buf, size, offset);
//     close(fd);
//     return res;
// }

// static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);
//     int fd = open(fpath, O_WRONLY);
//     if (fd == -1) return -errno;
//     int res = pwrite(fd, buf, size, offset);
//     close(fd);
//     return res;
// }

// static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);
//     int fd = creat(fpath, mode);
//     if (fd == -1) return -errno;
//     close(fd);
//     return 0;
// }

// static int fs_unlink(const char *path) {
//     char fpath[MAX_PATH];
//     starter_real_path(fpath, path);
//     int res = unlink(fpath);
//     return res == -1 ? -errno : 0;
// }

// static struct fuse_operations ops = {
//     .getattr = fs_getattr,
//     .readdir = fs_readdir,
//     .open    = fs_open,
//     .read    = fs_read,
//     .write   = fs_write,
//     .create  = fs_create,
//     .unlink  = fs_unlink,
// };

// int main(int argc, char *argv[]) {
//     return fuse_main(argc, argv, &ops, NULL);
// }


// #define FUSE_USE_VERSION 31
// #include <fuse3/fuse.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <dirent.h>
// #include <unistd.h>
// #include <sys/stat.h>

// #define BASE_PATH "/home/idzoyy/sisop-it24/Sisop-4-2025-IT01/soal_4"
// #define ORIGINAL_PATH "/home/idzoyy/sisop-it24/Sisop-4-2025-IT01/soal_4/chiho"
// #define MAX_PATH 1024

// // // ====================== Utility ======================
// void add_ext(char *dest, const char *src, const char *ext) {
//     snprintf(dest, MAX_PATH, "%s%s", src, ext);
// }

// void remove_ext(char *dest, const char *src, const char *ext) {
//     size_t len = strlen(src) - strlen(ext);
//     strncpy(dest, src, len);
//     dest[len] = '\0';
// }

// // // Metro shift
// // void index_shift(char *dst, const char *src) {
// //     int i;
// //     for (i = 0; src[i]; i++) {
// //         dst[i] = (unsigned char)((src[i] + i) % 256);
// //     }
// //     dst[i] = '\0';
// // }

// // void index_unshift(char *dst, const char *src) {
// //     int i;
// //     for (i = 0; src[i]; i++) {
// //         dst[i] = (unsigned char)((src[i] - i + 256) % 256);
// //     }
// //     dst[i] = '\0';
// // }

// // ====================== Real Path Mapper ======================
// void get_real_path(char *fpath, const char *path) {
//     // if (strncmp(path, "/starter/", 9) == 0) {
//     //     // snprintf(fpath, MAX_PATH, "%s%s%s", ORIGINAL_PATH, path, ".mai");
//     //     add_ext(fpath, path, ".mai");
//     // }
//     // else if (strncmp(path, "/metro/", 7) == 0) {
//     //     const char *filename = path + 7;
//     //     char shifted[NAME_MAX];
//     //     index_shift(shifted, filename);
//     //     snprintf(fpath, MAX_PATH, "%s/metro/%s%s", BASE_PATH, shifted, METRO_EXT);
//     // }
//     // else {
//         snprintf(fpath, MAX_PATH, "%s%s", ORIGINAL_PATH, path);
//     // }
// }

// // ====================== Operation Implementations ======================
// static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int res = lstat(fpath, stbuf);
//     return res == -1 ? -errno : 0;
// }

// static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
//                       off_t offset, struct fuse_file_info *fi,
//                       enum fuse_readdir_flags flags) {
//     char dir_path[MAX_PATH];
//     struct dirent *de;
//     DIR *dp;

//     if (strcmp(path, "/") == 0) {
//         snprintf(dir_path, MAX_PATH, "%s", ORIGINAL_PATH);
//     }
//     // else if (strncmp(path, "/starter", 8) == 0) {
//     //     snprintf(dir_path, MAX_PATH, "%s/starter", ORIGINAL_PATH);
//     // }
//     // else if (strncmp(path, "/metro", 6) == 0) {
//     //     snprintf(dir_path, MAX_PATH, "%s/metro", ORIGINAL_PATH);
//     // }
//     else {
//         snprintf(dir_path, MAX_PATH, "%s%s", ORIGINAL_PATH, path);
//     }

//     dp = opendir(dir_path);
//     if (!dp) return -errno;

//     filler(buf, ".", NULL, 0, 0);
//     filler(buf, "..", NULL, 0, 0);

//     while ((de = readdir(dp)) != NULL) {
//         // if (strncmp(path, "/starter", 8) == 0) {
//         //     if (strstr(de->d_name, ".mai")) {
//         //         char clean_name[NAME_MAX];
//         //         remove_ext(clean_name, de->d_name, ".mai");
//         //         filler(buf, clean_name, NULL, 0, 0);
//         //     }
//         // }
//         // else if (strncmp(path, "/metro", 6) == 0) {
//         //     if (strstr(de->d_name, METRO_EXT)) {
//         //         char raw_name[NAME_MAX], unshifted[NAME_MAX];
//         //         remove_ext(raw_name, de->d_name, METRO_EXT);
//         //         index_unshift(unshifted, raw_name);
//         //         filler(buf, unshifted, NULL, 0, 0);
//         //     }
//         // }
//         // else {
//             filler(buf, de->d_name, NULL, 0, 0);
//         // }
//     }

//     closedir(dp);
//     return 0;
// }

// static int fs_open(const char *path, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = open(fpath, fi->flags);
//     if (fd == -1) return -errno;
//     close(fd);
//     return 0;
// }

// static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = open(fpath, O_RDONLY);
//     if (fd == -1) return -errno;
//     int res = pread(fd, buf, size, offset);
//     close(fd);
//     return res;
// }

// static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = open(fpath, O_WRONLY);
//     if (fd == -1) return -errno;
//     int res = pwrite(fd, buf, size, offset);
//     close(fd);
//     return res;
// }

// static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = creat(fpath, mode);
//     if (fd == -1) return -errno;
//     close(fd);
//     return 0;
// }

// static int fs_unlink(const char *path) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int res = unlink(fpath);
//     return res == -1 ? -errno : 0;
// }

// // ====================== Main ======================
// static struct fuse_operations ops = {
//     .getattr = fs_getattr,
//     .readdir = fs_readdir,
//     .open    = fs_open,
//     .read    = fs_read,
//     .write   = fs_write,
//     .create  = fs_create,
//     .unlink  = fs_unlink,
// };

// int main(int argc, char *argv[]) {
//     return fuse_main(argc, argv, &ops, NULL);
// }




// #define FUSE_USE_VERSION 31
// #include <fuse3/fuse.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <dirent.h>
// #include <unistd.h>
// #include <sys/stat.h>

// #define BASE_PATH "/home/idzoyy/sisop-it24/Sisop-4-2025-IT01/soal_4"
// #define ORIGINAL_PATH "/home/idzoyy/sisop-it24/Sisop-4-2025-IT01/soal_4/chiho"
// #define MAX_PATH 1024

// // ====================== Real Path Mapper ======================
// void get_real_path(char *fpath, const char *path) {
//         snprintf(fpath, MAX_PATH, "%s%s", ORIGINAL_PATH, path);
// }

// // ====================== Operation Implementations ======================
// static int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int res = lstat(fpath, stbuf);
//     return res == -1 ? -errno : 0;
// }

// static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
//                       off_t offset, struct fuse_file_info *fi,
//                       enum fuse_readdir_flags flags) {
//     char dir_path[MAX_PATH];
//     struct dirent *de;
//     DIR *dp;

//     if (strcmp(path, "/") == 0) {
//         snprintf(dir_path, MAX_PATH, "%s", ORIGINAL_PATH);
//     }

//     else {
//         snprintf(dir_path, MAX_PATH, "%s%s", ORIGINAL_PATH, path);
//     }

//     dp = opendir(dir_path);
//     if (!dp) return -errno;

//     filler(buf, ".", NULL, 0, 0);
//     filler(buf, "..", NULL, 0, 0);

//     while ((de = readdir(dp)) != NULL) {
//             filler(buf, de->d_name, NULL, 0, 0);
//     }

//     closedir(dp);
//     return 0;
// }

// static int fs_open(const char *path, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = open(fpath, fi->flags);
//     if (fd == -1) return -errno;
//     close(fd);
//     return 0;
// }

// static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = open(fpath, O_RDONLY);
//     if (fd == -1) return -errno;
//     int res = pread(fd, buf, size, offset);
//     close(fd);
//     return res;
// }

// static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = open(fpath, O_WRONLY);
//     if (fd == -1) return -errno;
//     int res = pwrite(fd, buf, size, offset);
//     close(fd);
//     return res;
// }

// static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int fd = creat(fpath, mode);
//     if (fd == -1) return -errno;
//     close(fd);
//     return 0;
// }

// static int fs_unlink(const char *path) {
//     char fpath[MAX_PATH];
//     get_real_path(fpath, path);
//     int res = unlink(fpath);
//     return res == -1 ? -errno : 0;
// }

// // ====================== Main ======================
// static struct fuse_operations ops = {
//     .getattr = fs_getattr,
//     .readdir = fs_readdir,
//     .open    = fs_open,
//     .read    = fs_read,
//     .write   = fs_write,
//     .create  = fs_create,
//     .unlink  = fs_unlink,
// };

// int main(int argc, char *argv[]) {
//     return fuse_main(argc, argv, &ops, NULL);
// }


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

// Cek apakah path berada dalam folder starter
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
int is_in_77sref(const char *path) {
    return strncmp(path, "/77sref/", 7) == 0;
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


static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1000];

    // Tangani folder virtual "/7sref"
    if (strcmp(path, "/7sref") == 0) {
        memset(stbuf, 0, sizeof(struct stat));
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    // Tangani file virtual dalam "/7sref/"
    if (strncmp(path, "/7sref/", 6) == 0) {
        const char *vname = path + 6;
        char folder[256], filename[256];

        const char *underscore = strchr(vname, '_');
        if (!underscore) return -ENOENT;

        size_t folder_len = underscore - vname;
        strncpy(folder, vname, folder_len);
        folder[folder_len] = '\0';
        strcpy(filename, underscore + 1);

        char realpath[1000];
        sprintf(realpath, "/%s/%s", folder, filename);

        return xmp_getattr(realpath, stbuf);  // rekursif
    }

    append_ext_if_needed(fpath, path);
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

    // Tambahkan virtual folder '7sref' jika sedang membaca root
    if (strcmp(path, "/") == 0) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFDIR | 0755;
        filler(buf, "7sref", &st, 0);
    }

    // Jika membaca folder 7sref virtual
    if (strcmp(path, "/7sref") == 0) {
        DIR *subdp;
        struct dirent *subde;

        dp = opendir(dirpath);
        if (dp == NULL) return -errno;

        while ((de = readdir(dp)) != NULL) {
            // Lewati . dan ..
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;

            char subdirpath[1000];
            sprintf(subdirpath, "%s/%s", dirpath, de->d_name);

            struct stat st;
            if (stat(subdirpath, &st) == -1) continue;
            if (!S_ISDIR(st.st_mode)) continue;

            subdp = opendir(subdirpath);
            if (!subdp) continue;

            while ((subde = readdir(subdp)) != NULL) {
                if (strcmp(subde->d_name, ".") == 0 || strcmp(subde->d_name, "..") == 0)
                    continue;

                char vname[1000];
                sprintf(vname, "%s_%s", de->d_name, subde->d_name); // subdir_filename

                struct stat vst;
                memset(&vst, 0, sizeof(vst));
                vst.st_mode = S_IFREG | 0444;  // anggap file biasa, readonly
                strip_ext_if_needed(vname, subde->d_name);

                filler(buf, vname, &vst, 0);
            }

            closedir(subdp);
        }

        closedir(dp);
        return 0;
    }

    // Untuk direktori selain /7sref
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
    
    int res = 0;
    (void) fi;
    if (strncmp(path, "/7sref/", 6) == 0) {
        const char *vname = path + 6; // lewati "/7sref/"
        char folder[256], filename[256];

        // Pisahkan nama folder dan file
        const char *underscore = strchr(vname, '_');
        if (!underscore) return -ENOENT;

        size_t folder_len = underscore - vname;
        strncpy(folder, vname, folder_len);
        folder[folder_len] = '\0';
        strcpy(filename, underscore + 1);

        // Susun path asli
        char realpath[1000];
        sprintf(realpath, "/%s/%s", folder, filename);

        // Rekursif panggil xmp_read ke path asli
        return xmp_read(realpath, buf, size, offset, fi);
    }
    append_ext_if_needed(fpath, path);

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
        unsigned char key[32] = "thisis32bytepasswordforaesencrypt!"; // key sama dengan write
        unsigned char iv[16];
        FILE *fp = fopen(fpath, "rb");
        if (!fp) return -errno;

        fread(iv, 1, sizeof(iv), fp); // baca IV
        fseek(fp, 0, SEEK_END);
        long filesize = ftell(fp);
        long ciphertext_len = filesize - sizeof(iv);
        fseek(fp, sizeof(iv) + offset, SEEK_SET);

        unsigned char *ciphertext = malloc(ciphertext_len);
        unsigned char *plaintext = malloc(ciphertext_len); // plaintext max = ciphertext_len
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

    // Skystreet -> dekompresi zlib
    else if (is_in_skystreet(path)) {
        FILE *fp = fopen(fpath, "rb");
        if (!fp) return -errno;

        fseek(fp, 0, SEEK_END);
        long comp_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        unsigned char *comp_buf = malloc(comp_size);
        if (!comp_buf) {
            fclose(fp);
            return -ENOMEM;
        }

        fread(comp_buf, 1, comp_size, fp);
        fclose(fp);

        uLongf decomp_size = size * 4; // asumsi dekompresi maksimal 4x size
        unsigned char *decomp_buf = malloc(decomp_size);
        if (!decomp_buf) {
            free(comp_buf);
            return -ENOMEM;
        }

        if (uncompress(decomp_buf, &decomp_size, comp_buf, comp_size) != Z_OK) {
            free(comp_buf);
            free(decomp_buf);
            return -EIO;
        }

        if (offset < decomp_size) {
            int available = decomp_size - offset;
            int to_copy = (available < size) ? available : size;
            memcpy(buf, decomp_buf + offset, to_copy);
            res = to_copy;
        } else {
            res = 0;
        }

        free(comp_buf);
        free(decomp_buf);
        return res;
    }

    return -EINVAL; // area tidak dikenali
}


static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char fpath[1000];
    append_ext_if_needed(fpath, path);

    int res = 0;
    (void) fi;
        // Tangani file virtual di /7sref/
    if (strncmp(path, "/7sref/", 6) == 0) {
        const char *vname = path + 6; // lewati "/7sref/"
        char folder[256], filename[256];

        const char *underscore = strchr(vname, '_');
        if (!underscore) return -ENOENT;

        size_t folder_len = underscore - vname;
        strncpy(folder, vname, folder_len);
        folder[folder_len] = '\0';
        strcpy(filename, underscore + 1);

        char realpath[1000];
        sprintf(realpath, "/%s/%s", folder, filename);

        return xmp_write(realpath, buf, size, offset, fi);
    }

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
        unsigned char key[32] = "thisis32bytepasswordforaesencrypt!"; // key 32 byte
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

        fwrite(iv, 1, sizeof(iv), fp);  // Simpan IV di awal file
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

    // Skystreet -> kompresi zlib
    else if (is_in_skystreet(path)) {
        uLongf comp_size = compressBound(size);
        unsigned char *comp_buf = malloc(comp_size);
        if (!comp_buf) return -ENOMEM;

        if (compress(comp_buf, &comp_size, (const Bytef *)buf, size) != Z_OK) {
            free(comp_buf);
            return -EIO;
        }

        FILE *fp = fopen(fpath, "wb");
        if (!fp) {
            free(comp_buf);
            return -errno;
        }

        res = fwrite(comp_buf, 1, comp_size, fp);
        fclose(fp);
        free(comp_buf);
        return comp_size;
    }

    return -EINVAL; // tidak sesuai area
}


static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1000];
    int res = 0;
    (void) fi;

    // Tangani file virtual dalam "/7sref/"
    if (strncmp(path, "/7sref/", 6) == 0) {
        const char *vname = path + 6;
        char folder[256], filename[256];

        const char *underscore = strchr(vname, '_');
        if (!underscore) return -ENOENT;

        size_t folder_len = underscore - vname;
        strncpy(folder, vname, folder_len);
        folder[folder_len] = '\0';
        strcpy(filename, underscore + 1);

        char realpath[1000];
        sprintf(realpath, "/%s/%s", folder, filename);

        return xmp_create(realpath, mode, fi);  // rekursif panggil ke path asli
    }

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
