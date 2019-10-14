#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

#include <png.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"

void abort_(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

const int x_split = 2;
const int y_split = 4;
int target_width = 3744;
int target_height = 5616;
int splitted_w = 1872;
int splitted_h = 1404;

void write_png_file(char* file_name, uint8_t *image, int x_orig, int y_orig, int tar_width, int tar_height, int total_width, int total_height)
{
    png_structp png_ptr;
    png_infop info_ptr;

    printf("write file %s orig:%d %d tar:%d %d total:%d %d\n", file_name, x_orig, y_orig, tar_width, tar_height, total_width, total_height);
    /* create file */
    FILE *fp = fopen(file_name, "wb");
    if (!fp)
        abort_("[write_png_file] File %s could not be opened for writing", file_name);

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        abort_("[write_png_file] png_create_write_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        abort_("[write_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during init_io");

    png_init_io(png_ptr, fp);

    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during writing header");

    uint8_t output_bpp = 4;
    png_set_IHDR(png_ptr, info_ptr, tar_width, tar_height,
                 output_bpp, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during writing bytes");

    uint8_t **new_row_ptr = malloc(sizeof(*new_row_ptr) * tar_height);

    for (int y = 0; y < tar_height; ++y) {
        uint8_t *row = &image[(y + y_orig) * total_width + x_orig];
        new_row_ptr[y] = malloc(sizeof(**new_row_ptr) * tar_width / 2);

        for (int x = 0; x < tar_width; x += 2) {
            uint8_t to_write_first = row[x];
            uint8_t to_write_second = row[x + 1];
            new_row_ptr[y][x / 2] = (to_write_first >> 4) | (to_write_second >> 4 << 4);
        }
    }

    png_write_image(png_ptr, new_row_ptr);
    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);

    /* cleanup heap allocation */
    for (int y=0; y < tar_height; ++y)
        free(new_row_ptr[y]);
    free(new_row_ptr);

    fclose(fp);
}

uint8_t *switch_to_bw(uint8_t *image, int width, int height, int channels) {
    uint8_t *re = malloc(width * height);
    float bw_factors[3] = {0.3, 0.59, 0.11};

    for (int y = 0; y<height; ++y) {
        for (int x = 0; x<width; ++x) {
            int orig_offset = (x + y * width) *channels;
            uint8_t r = image[orig_offset];
            uint8_t g = image[orig_offset + 1];
            uint8_t b = image[orig_offset + 2];

            float bw_value = (float)r * bw_factors[0] + (float)g * bw_factors[1] + (float)b * bw_factors[2];
            re[x + y * width] = (float)(bw_value + 0.5);
        }
    }

    STBI_FREE(image);
    return re;
}

uint8_t *rotate_90(uint8_t *image, int width, int height) {
    uint8_t *re = malloc(width * height);


    int target_width = height;
    int target_height = width;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int target_y = x;
            int target_x = height - y - 1;

            re[target_x + target_y * target_width] = image[x + y * width];
        }
    }

    STBI_FREE(image);
    return re;
}

void apply_dithering_16(uint8_t *image, int width, int height) {
    float *error = (float*)malloc(width * 2 * sizeof(float));

    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < width; ++x) {
            error[y * width + x] = 0.0;
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            error[x] = error[width + x];
            error[width + x] = 0.0;
        }

        for (int x = 0; x < width; ++x) {
            int pixel_idx = (y * width + x);
            int origin_idx = (y * width + x);
            int dest_idx = pixel_idx;

            float err = 0.f;

            if (x > 0) {
                err += error[x + width - 1] * 0.5;
            }
            if (y > 0) {
                err += error[x] * 0.5;
            }

            float sum = image[origin_idx] + err;

            if (sum > 255.0) {
                sum = 255.0;
            }

            int target_v = (sum + 0.5) / 16.0;
            int target = target_v * 16;

            error[x + width] = (sum - target);
            image[dest_idx] = (uint8_t)target;
        }
    }

    free(error);
}

void write_column(uint8_t *image, int column) {
    int x_orig = 0;
    int y_orig = (column * target_height / y_split);
    char filename[12] = "out-?-?.png";
    filename[4] = '0';
    filename[6] = column + '0';

    write_png_file(filename, image, x_orig, y_orig, splitted_w, splitted_h, target_width, target_height);
    filename[4] = '1';
    x_orig = splitted_w;
    write_png_file(filename, image, x_orig, y_orig, splitted_w, splitted_h, target_width, target_height);
}

void *thread_1(void *image) {
    write_column(image, 0);
    pthread_exit(NULL);
}

void *thread_2(void *image) {
    write_column(image, 1);
    pthread_exit(NULL);
}

void *thread_3(void *image) {
    write_column(image, 2);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    if (argc != 3)
        abort_("Usage: program_name <file_in> <file_out>");

    int width, height, channels;

    uint8_t *image = stbi_load(argv[1],
                                     &width,
                                     &height,
                                     &channels,
                                     0);


    printf("loaded \n");
    if (image == NULL) {
        printf("image loading failed\n");
        return 1;
    }

    if (channels != 1) {
        image = switch_to_bw(image, width, height, channels);
        printf("switched to bw \n");
        channels = 1;
    }

    if (height < width) {
        image = rotate_90(image, width, height);
        int old_width = width;

        width = height;
        height = old_width;
    }

    if (width != target_width || height != target_height) {
        unsigned char *new_image = (unsigned char*)malloc(target_width * target_height * channels);

        stbir_resize_uint8(image, width, height, 0,
                           new_image, target_width, target_height, 0, channels);
        printf("resized \n");
        STBI_FREE(image);

        image = new_image;
    }


    apply_dithering_16(image, target_width, target_height);
    printf("dithered \n");


//    stbi_write_png("test-out.png", target_width, target_height, 1, image, 0);
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;

    if (pthread_create(&thread1, NULL, thread_1, image) || pthread_create(&thread2, NULL, thread_2, image) || pthread_create(&thread3, NULL, thread_3, image)) {
        printf("failed to create threads\n");
        return 1;
    }

    write_column(image, 3);


    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    return 0;
}
