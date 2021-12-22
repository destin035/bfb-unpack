#include "bluefield_boot.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/limits.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static struct {
        unsigned long id;
        const char *name;
} bfb_image_list[] = {
        /* Images for ATF */
        {36,  "psc-bl"},
        {37,  "psc-fw"},
        {31,  "bl2r-cert"},
        {30,  "bl2r"},
        {6,   "bl2-cert"},
        {1,   "bl2"},
        {29,  "sys"},
        {38,  "ddr-cert"},
        {32,  "ddr_ini"},
        {33,  "snps_images"},
        {34,  "ddr_ate_imem"},
        {35,  "ddr_ate_dmem"},
        {8,   "bl30-key-cert"},
        {12,  "bl30-cert"},
        {2,   "bl30"},
        {7,   "trusted-key-cert"},
        {9,   "bl31-key-cert"},
        {13,  "bl31-cert"},
        {3,   "bl31"},
        {10,  "bl32-key-cert"},
        {14,  "bl32-cert"},
        {4,   "bl32"},
        {11,  "bl33-key-cert"},
        {15,  "bl33-cert"},
        {5,   "bl33"},
        {52,  "capsule"},

        /* Images for UEFI */
        {55,  "boot-acpi"},
        {56,  "boot-dtb"},
        {57,  "boot-desc"},
        {58,  "boot-path"},
        {59,  "boot-args"},
        {60,  "boot-timeout"},
        {61,  "uefi-tests"},
        {54,  "ramdisk"},
        {62,  "image"},
        {63,  "initramfs"}
};

static void check(int test, const char *message, ...)
{
        if (test) {
                va_list args;
                va_start(args, message);
                vfprintf(stderr, message, args);
                va_end(args);
                fprintf(stderr, "\n");
                exit(EXIT_FAILURE);
        }
}

static size_t bfb_num_padding_bytes(size_t len)
{
        return ((len + 7) & ~7) - len;
}

static const char *bfb_get_image_name(unsigned long image_id)
{
        int i;
        static const int image_num = sizeof(bfb_image_list) / sizeof(bfb_image_list[0]);

        for (i = 0; i < image_num && bfb_image_list[i].id != image_id; i++) {
        }

        return i < image_num ? bfb_image_list[i].name : NULL;
}

static void bfb_unpack(const unsigned char *buf, size_t len)
{
        size_t offset = 0;

        while (offset < len) {
                int fd;
                const char *image_name;
                char image_path[PATH_MAX];

                union boot_image_header *hdr = (union boot_image_header *)(buf + offset);
                check(hdr->data.magic != BFB_IMGHDR_MAGIC, "magic error at offset %d", offset);
                offset += hdr->data.hdr_len << 3;

                image_name = bfb_get_image_name(hdr->data.image_id);
                check(image_name == NULL, "unknown image id %d", hdr->data.image_id);
                snprintf(image_path, PATH_MAX, "image_%s", image_name);
                fd = open(image_path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
                write(fd, buf + offset, hdr->data.image_len);
                close(fd);

                offset += hdr->data.image_len;
                offset += bfb_num_padding_bytes(offset);
        }
}

int main(int argc, char *argv[])
{
        int fd;
        const char *file_name;
        struct stat s;
        int status;
        size_t size;
        void *mapped;

        check(argc < 2, "Usage: %s <bfb file>", argv[0]);
        file_name = argv[1];

        fd = open(file_name, O_RDONLY);
        check(fd < 0, "open %s failed: %s", file_name, strerror(errno));

        status = fstat(fd, &s);
        check(status < 0, "stat %s failed: %s", file_name, strerror(errno));
        size = s.st_size;

        mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        check(mapped == MAP_FAILED, "mmap %s failed: %s", file_name, strerror(errno));
        close(fd);

        bfb_unpack((unsigned char *)mapped, size);
        munmap(mapped, size);

        return 0;
}
