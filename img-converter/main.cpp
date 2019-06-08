#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

int main(int ac, char** av) {
    if (ac < 3) {
        printf("usage %s: input output", av[0]);
    }

    int width, height, channels;
    unsigned char *image = stbi_load(av[1],
                                     &width,
                                     &height,
                                     &channels,
                                     STBI_rgb);

    printf("image loaded with %d %d %d\n", width, height, channels);


    float bw_factors[3] = {0.3, 0.59, 0.11};
    unsigned char *new_image = (unsigned char*)malloc(width * height * 3);
    float *error = (float*)malloc(width * height * sizeof(float));
//    memset(error, 0, width * height);

//    for (int x = 0; x < width; ++x) {
//        for (int y = 0; y < height; ++y) {
//            int idx = (y * width + x) * channels;
//
//            float r = image[idx];
//            float g = image[idx + 1];
//            float b = image[idx + 2];
//
//            float sum = r * 0.3 + g * 0.59 + b * 0.11;
////            sum = (int)(sum + 0.5) / 16 * 16; 16 colors
//
//            new_image[idx] = (unsigned char)sum;
//            new_image[idx + 1] = (unsigned char)sum;
//            new_image[idx + 2] = (unsigned char)sum;
//        }
//    }

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
