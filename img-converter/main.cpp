#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

unsigned char *crop_image(unsigned char *origin, int input_width, int input_height, int target_width, int target_height, int channels) {
    int start_x, start_y;
    unsigned char *cropped_pixels = (unsigned char*)malloc(target_width * target_height * channels);

    if (input_width > target_width) {
        int delta = input_width - target_width;

        start_x = delta / 2;
        start_y = 0;
    } else {
        int delta = input_height - input_height;

        start_y = delta / 2;
        start_x = 0;
    }

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

int main(int ac, char** av) {
    int target_width = -1;
    int target_height = -1;
    bool fill_with_color = false;
    unsigned char fill_color = 255;

    if (ac < 3) {
        printf("usage %s: input output", av[0]);
    }

    if (ac > 4) {
        target_width = atoi(av[3]);
        target_height = atoi(av[4]);
    }
    if (ac > 5) {
        fill_with_color = atoi(av[5]) != 0;
    }
    if (ac > 6) {
        fill_color = atoi(av[6]);
    }

    int width, height, channels;
    unsigned char *image = stbi_load(av[1],
                                     &width,
                                     &height,
                                     &channels,
                                     STBI_rgb);
    if (target_height != height && target_width != width) {
        if (fill_with_color == false) {
            float biggest_ratio = MAX((float)target_width / width, (float)target_height / height);
            int corrected_target_width = biggest_ratio * width;
            int corrected_target_height = biggest_ratio * height;


            unsigned char *out = (unsigned char*)malloc(corrected_target_width * corrected_target_height * channels);
            stbir_resize_uint8(image, width, height , 0,
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

            printf("delta is : %d, %d, fill_color is %d\n", x_delta, y_delta, fill_color);
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
    unsigned char *new_image = (unsigned char*)malloc(width * height * 3);
    float *error = (float*)malloc(width * height * sizeof(float));

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            error[y * width + x] = 0.0;
        }
    }
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            int idx = (y * width + x) * channels;
            int err_idx = (y * width + x);


            float err = 0.f;

            if (x > 0) {
                err += error[err_idx - 1];
            }
            if (y > 0) {
                err += error[err_idx - width];
            }

            float r = image[idx] + err;
            float g = image[idx + 1] + err;
            float b = image[idx + 2] + err;

            float sum = r * 0.3 + g * 0.59 + b * 0.11;
            if (sum > 255) {
                sum = 255;
            }

            int target = (int)(sum) / 16 * 16;
            float computed_error = (sum - target);
//            printf("target is: %d vs %f, error is %f, old is %f\n", target, sum, computed_error, err);

            if (x < width - 1) {
                error[err_idx + 1] = computed_error * 0.5;
            }
            if (y < height - 1) {
                error[err_idx + width] = computed_error * 0.5;
            }

            new_image[idx] = (unsigned char)target;
            new_image[idx + 1] = (unsigned char)target;
            new_image[idx + 2] = (unsigned char)target;
        }
    }



    stbi_write_png(av[2], width, height, 3, new_image, 0);
//    int stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);
}
