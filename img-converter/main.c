#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/socket.h> //socket
#include <netinet/in.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h> // write(socket, message, strlen(message))
#include <string.h> //strlen

#include <png.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"
#define ABS(x) ((x) < 0 ? -(x) : (x))

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
const int total_client = x_split * y_split;
const int h_padding = 100;
const int v_padding = 80;
const int _target_width = 3744;
const int _target_height = 5616;
const float target_width_f = _target_width;
const float target_height_f = _target_height;
const float no_padding_ratio = target_height_f / target_width_f;
const int target_width_with_padding = _target_width + (x_split - 1) * v_padding;
const int target_height_with_padding = _target_height + (y_split - 1) * h_padding;
const int splitted_w = 1872;
const int splitted_h = 1404;

const char idx_to_ip[8][16] = {"192.168.86.46", "192.168.86.44", // bottom left, top right
                               "192.168.86.43", "192.168.86.25",
                               "192.168.86.38", "192.168.86.21",
                               "192.168.86.29", "192.168.86.36"}; // TODO: Replace by rasp ips

typedef struct s_split_input {
    int idx;
    char *filename;
    uint8_t *image;
    int apply_padding;
    int target_width;
    int target_height;
} t_split_input;

char *delim_token = "||";

void free_cmd_list(char **cmds) {
    int i = 0;

    while (cmds[i]) {
        free(cmds[i]);
        ++i;
    }
    free(cmds);
}

char **get_next_command(char buffer[512], int *total_length) {
  int start = -1;
  int end = -1;
  unsigned long delim_token_length = strlen(delim_token);
  char **re = NULL;
  int cmd_index = 0;

    for (int i = 0; i <= *total_length - delim_token_length; ++i) {
        if (!strncmp(&buffer[i], delim_token, delim_token_length)) {
            if (start == -1 || i - start < delim_token_length) {
                start = i;
                i += delim_token_length;
            } else {
                end = i + delim_token_length;

                if (re == NULL) {
                    re = (char**)malloc(sizeof(char*) * 10);
                }

                int cmd_length = end - start - 4;
                char *cmd = malloc((cmd_length + 1) * sizeof(char*));
                cmd[cmd_length] = '\0';
                memcpy(cmd, &buffer[0] + start + 2, end - 4 - start);
                re[cmd_index++] = cmd;
                start = -1;
            }
        }
    }

  if (re) {
    memcpy(&buffer[0], &buffer[end], *total_length - end);
    *total_length = *total_length - end;

    re[cmd_index] = NULL;
  }
  return re;
}


int send_to_server(char *ip, uint16_t port, char *msg) {
    //create a socket, returns -1 of error happened
    /*
     Address Family - AF_INET (this is IP version 4)
     Type - SOCK_STREAM (this means connection oriented TCP protocol)
     Protocol - 0 [ or IPPROTO_IP This is IP protocol]
     */
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create a socket\n");
    }

    //create a server
    struct sockaddr_in server;


    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port); //specify the open port_number

    printf("trying to connect %s\n", ip);
    //connect to the server
    if (connect(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Connect error : %s\n", ip);
        return 1;
    }
    printf("Connected %s\n", ip);

    if (send(socket_desc, msg, strlen(msg), 0) < 0) {
        puts("Send failed\n");
        return 1;
    }
    puts("Data send\n");

    //receive data from server
    int received;
    char received_data[512];
    int index = 0;
    while ((received = recv(socket_desc, received_data + index, 512 - index, 0)) > 0) {
        index += received;
        char **commands = get_next_command(received_data, &index);

        if (!commands) {
            printf("is not a command\n");
            continue;
        }
        int i = 0;
        char *command = commands[0];
        if (strncmp(command, "D", 1) == 0) {
            char *filename = command + 1;
            FILE *fp = fopen(filename, "r");
            printf("Trying to open: %s", filename);
            if (fp == NULL)
            {
                write(socket_desc, "File not found", strlen("File not found"));
                break;
            }

            char send_buffer[4096 * 4];

            int file_read_size;
            while ((file_read_size = fread(send_buffer, sizeof(char), 4096 * 4, fp))) {
                write(socket_desc, send_buffer, file_read_size);
            }

            fclose(fp);
            free_cmd_list(commands);
            break;
        }

        free_cmd_list(commands);
    }
    close(socket_desc);
    return 0;
}


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

void *send_img(void *input_struct) {
    t_split_input *input = (t_split_input*)input_struct;

    char filename[256];
    snprintf(filename, 256, "U%s-%d", input->filename, input->idx);

    send_to_server((char*)idx_to_ip[input->idx], 8888, filename);

    return NULL;
}

void *split_img(void *input_struct) {
    t_split_input *input = (t_split_input*)input_struct;

    char filename[256];
    snprintf(filename, 256, "U%s-%d", input->filename, input->idx);


    int x_idx = input->idx % x_split;
    int y_idx = input->idx / x_split;

    int x_orig = x_idx * splitted_w;
    int y_orig = y_idx * splitted_h;

    if (input->apply_padding) {
        x_orig += x_idx * v_padding;
        y_orig += y_idx * h_padding;
    }

    printf("idx: %d %d, max : %d %d; total_size: %d %d\n", x_idx, y_idx, x_orig + splitted_w, y_orig + splitted_h, input->target_width, input->target_height);
    write_png_file(filename + 1, input->image, x_orig, y_orig, splitted_w, splitted_h, input->target_width, input->target_height);

    send_to_server((char*)idx_to_ip[input->idx], 8888, filename);

    return NULL;
}

int check_if_parts_exist(char *filename) {
    char part_filename[256];


    for (int i = 0; i < 8; ++i) {
        snprintf(part_filename, 256, "%s-%d", filename, i);
        if (fopen(part_filename, "r") == NULL) {
            return 1;
        }
    }

    printf("No need to generate, all parts already exists\n");
    return 0;
}

int set_image(char *filename) {
    pthread_t threads[total_client];
    t_split_input inputs[total_client];

    int alread_generated = check_if_parts_exist(filename);
    if (alread_generated) {
        int width, height, channels;

        uint8_t *image = stbi_load(filename,
                                   &width,
                                   &height,
                                   &channels,
                                   0);
        float ratio = width / (float)height;
        int should_apply_paddings = ABS(ratio - no_padding_ratio) > 0.01;
        int computed_width = should_apply_paddings ? target_width_with_padding : _target_width;
        int computed_height = should_apply_paddings ? target_height_with_padding : _target_height;

        printf("loaded with padding: %d  ratio is %f, delta is %f \n", should_apply_paddings, ratio, ABS(ratio - no_padding_ratio));
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

        if (width != computed_width || height != computed_height) {
            unsigned char *new_image = (unsigned char*)malloc(computed_width * computed_height * channels);

            stbir_resize_uint8(image, width, height, 0,
                               new_image, computed_width, computed_height, 0, channels);
            printf("resized \n");
            STBI_FREE(image);

            image = new_image;
        }

        apply_dithering_16(image, computed_width, computed_height);
        printf("dithered \n");

        for (int i = 0; i < total_client; ++i) {
            inputs[i].image = image;
            inputs[i].idx = i;
            inputs[i].filename = filename;
            inputs[i].apply_padding = should_apply_paddings;
            inputs[i].target_width = computed_width;
            inputs[i].target_height = computed_height;

            if (pthread_create(&threads[i], NULL, split_img, &inputs[i])) {
                printf("failed to create threads\n");
                return 1;
            }
        }
    } else {
        for (int i = 0; i < total_client; ++i) {
            inputs[i].idx = i;
            inputs[i].filename = filename;

            if (pthread_create(&threads[i], NULL, send_img, &inputs[i])) {
                printf("failed to create threads\n");
                return 1;
            }
        }
    }

    for (int i = 0; i < total_client; ++i) {
        printf("join on %d\n", inputs[i].idx);
        pthread_join(threads[i], NULL);
    }
    printf("waiting finished");

    return 0;
}


//connection handler
void *connection_handler(void *socket_desc){
    //get the socket descriptor
    int sock = *(int*)socket_desc;
    int received;
    char *message, received_data[512];
    //send some message to the client

    //Receive a message from client
    int index = 0;
    while ((received = recv(sock, received_data + index, 512 - index, 0)) > 0) {
        index += received;
        char **commands = get_next_command(received_data, &index);

        if (!commands) {
            continue;
        }
        int i = 0;
        while (commands[i]) {
            char *command = commands[i++];

            if (strncmp(command, "S", 1) == 0) {
                char *filename = command + 1;
                printf("filename: %s\n", filename);
                int error = set_image(filename);
                write(sock, error ? "404" : "200", 3);
            } else if (strncmp(command, "D", 1) == 0) {
                char *filename = command + 1;
                FILE *fp = fopen(filename, "r");
                printf("Trying to open: %s\n", filename);
                if (fp == NULL)
                {
                    write(sock, "File not found", strlen("File not found"));
                    continue;
                }

                char send_buffer[4096 * 4];

                int file_read_size;
                while ((file_read_size = fread(send_buffer, sizeof(char), 4096 * 4, fp))) {
                    printf("reading %d\n", file_read_size);
                    write(sock, send_buffer, file_read_size);
                }

                fclose(fp);
                close(sock);
                free(socket_desc);
                free_cmd_list(commands);
                return 0;
            }
        }
        free_cmd_list(commands);
    }

    if (received == 0){
        puts("Client disconnected\n");
    }
    else if (received == -1){
        perror("recv failed\n");
    }

    //Free the socket pointer
    close(sock);
    free(socket_desc);
    return 0;
}

int start_server() {
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create a socket\n");
    }

    //create a server
    struct sockaddr_in server;

    char *IP = "127.0.0.1";
    unsigned short OPEN_PORT = 8889;

    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(OPEN_PORT); //specify the open port_number

    if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0 ) {
        printf("Bind failed\n");
        return 1;
    }
    printf("Bind done\n");

    //listen for new connections:
    listen(socket_desc, 12);
    puts("Waiting for new connections...");

    int c = sizeof(struct sockaddr_in);
    //client to be connected
    struct sockaddr_in client;
    //new socket for client
    int new_socket, *new_sock;
    while ((new_socket = accept( socket_desc, (struct sockaddr*)&client, (socklen_t*)&c ))) {
        puts("Connection accepted");
        //get the IP address of a client
        char *CLIENT_IP = inet_ntoa(client.sin_addr);
        int CLIENT_PORT = ntohs(client.sin_port);
        printf("       CLIENT = {%s:%d}\n", CLIENT_IP, CLIENT_PORT);
        //reply to the client
        char* message = "Hello, client! I have received your connection, now I will assign a handler for you!\n";
        write(new_socket, message, strlen(message));

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*)new_sock) < 0  ){
            perror("could not create thread");
            return 1;
        }
        //Now join the thread , so that we dont terminate before the thread
        //        pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }
    if (new_socket < 0 ) {
        perror("Accept failed");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    while (1) {
        start_server();
        sleep(1);
    }
    return 0;
}
