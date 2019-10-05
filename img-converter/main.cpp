#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"
#include <string>
#include <vector>
#include <png.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

void read_png_file(char* file_name)
{
    int x, y;

    int width, height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep * row_pointers;

    char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
        printf("[read_png_file] File %s could not be opened for reading", file_name);
    fread(header, 1, 8, fp);
    if (png_sig_cmp((unsigned char*)header, 0, 8))
        printf("[read_png_file] File %s is not recognized as a PNG file", file_name);


    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
        printf("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        printf("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);


    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
        printf("[read_png_file] Error during read_image");

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (y=0; y<height; y++)
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

    png_read_image(png_ptr, row_pointers);

    fclose(fp);
}

unsigned char *crop_image(unsigned char *origin, int input_width, int input_height, int target_width, int target_height, int channels) {
    int start_x, start_y;
    unsigned char *cropped_pixels = (unsigned char*)malloc(target_width * target_height * channels);

    if (input_width > target_width) {
        int delta = input_width - target_width;

        start_x = delta / 2;
        start_y = 0;
    } else {
        int delta = input_height - target_height;

        start_y = delta / 2;
        start_x = 0;
    }

    printf("cropping with: %d %d\n", start_y, start_x);
    for (int y = 0; y < target_height; ++y) {
        for (int x = 0; x < target_width; ++x) {
            int origin_idx = ((y + start_y) * input_width + (x + start_x)) * channels;
            int idx_dest = (y * target_width + x) * channels;

            cropped_pixels[idx_dest] = origin[origin_idx];
            cropped_pixels[idx_dest + 1] = origin[origin_idx];
            cropped_pixels[idx_dest + 2] = origin[origin_idx];
        }
    }

    return cropped_pixels;
}

struct Options {
    std::vector<std::string> inputs;
    int target_width = -1;
    int target_height = -1;
    bool should_fill = false;
    unsigned char fill_color = 255;
};

void getOptions(int ac, char** av, Options &options) {
    for (int i = 1; i < ac; ++i) {
        if (strcmp(av[i], "--input") == 0) {
            ++i;
            int a = i;

            while (a < ac && av[a][0] != '-') {
                auto to_add = std::string(av[a++]);
                options.inputs.push_back(std::move(to_add));
                printf("adding : %s\n", av[a - 1]);
            }
            i = a - 1;
        } else if (strcmp(av[i], "--export-size") == 0) {
            options.target_width = atoi(av[++i]);
            options.target_height = atoi(av[++i]);
        } else if (strcmp(av[i], "--fill-color") == 0) {
            options.should_fill = true;
            options.fill_color = atoi(av[++i]);
        }
    }
}

void convert(const char *input_file, const char* out_file, int target_width, int target_height, bool should_fill, unsigned char fill_color) {
    int width, height, channels;
    unsigned char *image = stbi_load(input_file,
                                     &width,
                                     &height,
                                     &channels,
                                     0);

    if (target_width <= 0 || target_height <= 0) {
        target_width = width;
        target_height = height;
    }

    printf("input image: %d %d; target: %d %d; %d\n", width, height, target_width, target_height, should_fill);

    if (target_height != height || target_width != width) {
        if (should_fill == false) {
            float biggest_ratio = MAX((float)target_width / width, (float)target_height / height);
            int corrected_target_width = biggest_ratio * width;
            int corrected_target_height = biggest_ratio * height;


            unsigned char *out = (unsigned char*)malloc(corrected_target_width * corrected_target_height * channels);
            stbir_resize_uint8(image, width, height, 0,
                               out, corrected_target_width, corrected_target_height, 0, channels);
            if (corrected_target_height != target_height || corrected_target_width != target_width) {
                image = crop_image(out, corrected_target_width, corrected_target_height, target_width, target_height, channels);
            } else {
                image = out;
            }

            width = target_width;
            height = target_height;
        } else {
            float smallest_ratio = MIN((float)target_width / width, (float)target_height / height);
            int corrected_target_width = smallest_ratio * width;
            int corrected_target_height = smallest_ratio * height;
            unsigned char *out = (unsigned char*)malloc(corrected_target_width * corrected_target_height * channels);
            stbir_resize_uint8(image, width, height , 0,
                               out, corrected_target_width, corrected_target_height, 0, channels);
            unsigned char *cropped_pixels = (unsigned char*)malloc(target_width * target_height * channels);

            int x_delta = (corrected_target_width - target_width) / 2;
            int y_delta = (corrected_target_height - target_height) / 2;

            for (int y = 0; y < target_height; ++y) {
                for (int x = 0; x < target_width; ++x) {
                    int origin_x = x + x_delta;
                    int origin_y = y + y_delta;
                    int idx_dest = (y * target_width + x) * channels;
                    if (origin_x < 0 || origin_y < 0 || origin_x >= corrected_target_width || origin_y >= corrected_target_height) {
                        cropped_pixels[idx_dest] = fill_color;
                        cropped_pixels[idx_dest + 1] = fill_color;
                        cropped_pixels[idx_dest + 2] = fill_color;
                    } else {
                        int origin_idx = ((origin_y) * corrected_target_width + (origin_x)) * channels;

                        cropped_pixels[idx_dest] = out[origin_idx];
                        cropped_pixels[idx_dest + 1] = out[origin_idx];
                        cropped_pixels[idx_dest + 2] = out[origin_idx];
                    }
                }
            }
            image = cropped_pixels;
            width = target_width;
            height = target_height;
        }
    }


    float bw_factors[3] = {0.3, 0.59, 0.11};
    unsigned char *new_image = (unsigned char*)malloc(width * height);
    float *error = (float*)malloc(width * height * sizeof(float));

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            error[y * width + x] = 0.0;
        }
    }
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int pixel_idx = (y * width + x);
            int origin_idx = (y * width + x) * channels;
            int dest_idx = pixel_idx;
            int err_idx = (y * width + x);

            float err = 0.f;

            if (x > 0) {
                err += error[err_idx - 1];
            }
            if (y > 0) {
                err += error[err_idx - width];
            }

            float r, g, b;
            if (channels >= 3) {
                r = image[origin_idx] + err;
                g = image[origin_idx + 1] + err;
                b = image[origin_idx + 2] + err;
            } else {
                r = image[origin_idx] + err;
                g = r;
                b = r;
            }

            float sum = r * bw_factors[0] + g * bw_factors[1] + b * bw_factors[2];
            if (sum > 255) {
                sum = 255;
            }

            int target_v = sum / 16.0;
            int target = target_v * 16;

            float computed_error = (sum - target);

            if (x < width - 1) {
                error[err_idx + 1] = computed_error * 0.5;
            }
            if (y < height - 1) {
                error[err_idx + width] = computed_error * 0.5;
            }

            new_image[dest_idx] = (unsigned char)target;
        }
    }

    stbi_write_png(out_file, width, height, 1, new_image, 0);
    printf("writing %s as %s\n", input_file, out_file);
}

int main(int ac, char** av) {
    Options options;

    getOptions(ac, av, options);
    printf("exporting do: %d %d\n", options.target_width, options.target_height);
    for (std::string &input: options.inputs) {
        size_t lastindex = input.find_last_of(".");
        std::string rawname = input.substr(0, lastindex);
        std::string out = rawname + "-out.png";
        convert(input.c_str(), out.c_str(), options.target_width, options.target_height, options.should_fill, options.fill_color);
    }
}
