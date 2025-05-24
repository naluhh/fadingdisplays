#include "IT8951.h"
#include <microhttpd.h>
#include <png.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER;
#define TRANSFER_READ_SIZE 4096 * 4
#define MAX_UPLOAD_SIZE (10 * 1024 * 1024)
#define TEMP_FILENAME "/tmp/uploaded_image.png"

int IT8951_started = 0;
uint8_t *buffer_to_write = NULL;
int target_screen_width = 1872;
int target_screen_height = 1404;
int should_revert = 0;
uint32_t last_command_time = 0;

void abort_(const char * s) {
    printf("%s\n", s);
    abort();
}

int read_png_file(char* file_name, int* width_ptr, int* height_ptr, png_byte *color_type_ptr, png_byte *bit_depth_ptr, uint8_t *buffer_to_write) {
    char header[8];
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        printf("[read_png_file] File %s could not be opened for reading errno = %d\n", file_name, errno);
        return 1;
    }
    fread(header, 1, 8, fp);
    if (png_sig_cmp((png_const_bytep)header, 0, 8)) {
        printf("[read_png_file] File %s is not recognized as a PNG file\n", file_name);
        fclose(fp);
        return 1;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) abort_("[read_png_file] png_create_read_struct failed");

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr))) abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    *width_ptr = png_get_image_width(png_ptr, info_ptr);
    *height_ptr = png_get_image_height(png_ptr, info_ptr);
    *color_type_ptr = png_get_color_type(png_ptr, info_ptr);
    *bit_depth_ptr = png_get_bit_depth(png_ptr, info_ptr);

    if (*width_ptr != target_screen_width || *height_ptr != target_screen_height) {
        printf("Image should be %dx%d but it's %dx%d\n", target_screen_width, target_screen_height, *width_ptr, *height_ptr);
        fclose(fp);
        return 1;
    }

    png_read_update_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) abort_("[read_png_file] Error during read_image");

    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * *height_ptr);
    png_bytep all_bytes = (png_bytep)buffer_to_write;
    all_bytes[0] = 0;
    all_bytes[1] = 0;
    int row_size = png_get_rowbytes(png_ptr, info_ptr);
    for (int y = 0; y < *height_ptr; y++)
        row_pointers[y] = 2 + all_bytes + (y * row_size);

    png_read_image(png_ptr, row_pointers);
    free(row_pointers);
    fclose(fp);

    return 0;
}

void start_board() {
    pthread_mutex_lock(&board_mutex);
    if (!(buffer_to_write = IT8951_Init(target_screen_width, target_screen_height, should_revert))) {
        printf("IT8951_Init error, exiting\n");
        exit(1);
    } else {
        IT8951_started = 1;
        last_command_time = time(NULL);
        printf("IT8951 started\n");
    }
    pthread_mutex_unlock(&board_mutex);
}

void stop_board() {
    pthread_mutex_lock(&board_mutex);
    buffer_to_write = NULL;
    IT8951_Cancel();
    IT8951_started = 0;
    printf("Board is now stopped\n");
    pthread_mutex_unlock(&board_mutex);
}

void *stop_board_loop(void *data) {
    while (1) {
        if (IT8951_started) {
            if (time(NULL) - last_command_time > 30) stop_board();
            sleep(30);
        } else {
            sleep(30);
        }
    }
    return NULL;
}

int display_4bpp_filename(char *filename) {
    last_command_time = time(NULL);
    if (!IT8951_started) {
        printf("Update without previous start command. Starting automatically\n");
        start_board();
    }

    png_byte color_type;
    png_byte bit_depth;
    int width, height;

    printf("Reading file: %s\n", filename);
    if (read_png_file(filename, &width, &height, &color_type, &bit_depth, buffer_to_write)) return 1;

    if (width != target_screen_width || height != target_screen_height) {
        printf("Image should be %dx%d but it's %dx%d\n", target_screen_width, target_screen_height, width, height);
        return 1;
    }
    printf("Updating screen for file: %s\n", filename);
    pthread_mutex_lock(&board_mutex);
    IT8951_Display4BppBuffer();
    pthread_mutex_unlock(&board_mutex);

    return 0;
}

int handle_display_request(const void *data, size_t size) {
    FILE *fp = fopen(TEMP_FILENAME, "wb");
    if (!fp) {
        perror("Failed to write temp PNG");
        return 1;
    }
    fwrite(data, 1, size, fp);
    fclose(fp);

    return display_4bpp_filename((char *)TEMP_FILENAME);
}

struct RequestData {
    char *image_data;
    size_t total_received;
};

int answer_to_connection(void *cls, struct MHD_Connection *connection,
                         const char *url, const char *method,
                         const char *version, const char *upload_data,
                         size_t *upload_data_size, void **con_cls) {
    if (strcmp(method, "POST") == 0 && strcmp(url, "/display") == 0) {
        struct RequestData *req = *con_cls;

        if (!req) {
            req = calloc(1, sizeof(struct RequestData));
            *con_cls = req;
            return MHD_YES;
        }

        if (*upload_data_size > 0) {
            if (!req->image_data)
                req->image_data = malloc(MAX_UPLOAD_SIZE);

            if (req->total_received + *upload_data_size > MAX_UPLOAD_SIZE) {
                free(req->image_data);
                free(req);
                return MHD_NO;
            }

            memcpy(req->image_data + req->total_received, upload_data, *upload_data_size);
            req->total_received += *upload_data_size;
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            int status = handle_display_request(req->image_data, req->total_received);
            free(req->image_data);
            free(req);

            const char *response = status == 0 ? "OK\n" : "ERROR\n";
            struct MHD_Response *mhd_response = MHD_create_response_from_buffer(strlen(response),
                (void*)response, MHD_RESPMEM_PERSISTENT);
            return MHD_queue_response(connection, status == 0 ? MHD_HTTP_OK : MHD_HTTP_BAD_REQUEST, mhd_response);
        }
    }

    const char *not_found = "Not Found\n";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(not_found),
        (void*)not_found, MHD_RESPMEM_PERSISTENT);
    return MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
}

int main() {
    should_revert = fopen("reverted", "r") != NULL;
    pthread_t check_board_on;
    pthread_create(&check_board_on, NULL, stop_board_loop, NULL);

    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, 8890, NULL, NULL,
                                                 &answer_to_connection, NULL, MHD_OPTION_END);
    if (!daemon) {
        printf("Failed to start HTTP server\n");
        return 1;
    }

    printf("HTTP server running on port 8890...\n");
    getchar();
    MHD_stop_daemon(daemon);
    return 0;
}
