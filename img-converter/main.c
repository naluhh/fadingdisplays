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
int target_width = 3744;
int target_height = 5616;
int splitted_w = 1872;
int splitted_h = 1404;

const char idx_to_ip[8][16] = {"127.0.0.1", "127.0.0.12", "127.0.0.12", "127.0.0.12", "127.0.0.12", "127.0.0.12", "127.0.0.12"}; // TODO: Replace by rasp ips

typedef struct s_split_input {
    int idx;
    char *filename;
    uint8_t *image;
} t_split_input;


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

    //connect to the server
    if (connect(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0) {
        puts("Connect error\n");
        return 1;
    }
    puts("Connected\n");

    if (send(socket_desc, msg, strlen(msg), 0) < 0) {
        puts("Send failed\n");
        return 1;
    }
    puts("Data send\n");

    //receive data from server
    int received;
    char received_data[16];
    if ((received = recv(socket_desc, received_data, 16, 0)) <= 0) {
        return 1;
    }
    close(socket_desc);
    return strncmp(received_data, "OK", 2);
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

    int len = strlen(input->filename) + 1;
    char *filename = malloc(len + 10);
    memcpy(filename, input->filename + 1, len);
    filename[0] = 'S'; // Set Order
    filename[len] = '-';
    filename[len + 1] = input->idx + '0';
    filename[len + 2] = 0;

    send_to_server(idx_to_ip[input->idx], 8889, filename);

    free(filename);

    return NULL;
}

void *split_img(void *input_struct) {
    t_split_input *input = (t_split_input*)input_struct;

    int len = strlen(input->filename) + 1;
    char *filename = malloc(len + 10);
    memcpy(filename, input->filename + 1, len);
    filename[0] = 'S'; // Set Order
    filename[len + 1] = '-';
    filename[len + 2] = input->idx + '0';
    filename[len + 3] = 0;

    int x_orig = input->idx % x_split * splitted_w;
    int y_orig = (input->idx / x_split * target_height / y_split);

    write_png_file(filename + 1, input->image, x_orig, y_orig, splitted_w, splitted_h, target_width, target_height);

    send_to_server(idx_to_ip[input->idx], 8889, filename);

    free(filename);

    return NULL;
}

int check_if_parts_exist(char *filename) {
    int len = strlen(filename);
    char *part_filename = malloc(len + 10);
    memcpy(part_filename, filename, len);
    part_filename[len] = '-';
    part_filename[len + 2] = 0;

    for (int i = 0; i < 8; ++i) {
        part_filename[len + 1] = i + '0';
        if (fopen(part_filename, "r") == NULL) {
            return 1;
        }
    }

    free(part_filename);

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
        //
        if (height < width) {
            image = rotate_90(image, width, height);
            int old_width = width;

            width = height;
            height = old_width;
        }
        //
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

        for (int i = 0; i < total_client; ++i) {
            inputs[i].image = image;
            inputs[i].idx = i;
            inputs[i].filename = filename;

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
        pthread_join(threads[i], NULL);
    }

    return 0;
}


//connection handler
void *connection_handler(void *socket_desc){
    //get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message, client_message[256];
    //send some message to the client

    //Receive a message from client
    while ((read_size = recv(sock, client_message, 256, 0)) > 0 ){
        //Send the message back to client
        if (strncmp(client_message, "S", 1) == 0) {
            char *filename = client_message + 1;
            client_message[read_size] = 0;
            printf("filename: %s", filename);
            int error = set_image(filename);
            write(sock, error ? "404" : "200", 3);
            break;
        } else if (strncmp(client_message, "D", 1) == 0) {
            char *filename = client_message + 1;
            client_message[read_size] = 0;
            FILE *fp = fopen(filename, "r");
            printf("Trying to open: %s", filename);
            if (fp == NULL)
            {
                write(sock, "File not found", strlen("File not found"));
                break;
            }

            char send_buffer[4096 * 4];

            int file_read_size;
            while ((file_read_size = fread(send_buffer, sizeof(char), 4096 * 4, fp))) {
                printf("reading %d\n", file_read_size);
                write(sock, send_buffer, file_read_size);
            }

            fclose(fp);
            break;
        }
    }

    if (read_size == 0){
        puts("Client disconnected\n");
    }
    else if(read_size == -1){
        perror("recv failed\n");
    }

    //Free the socket pointer
    close(sock);
    free(socket_desc);
    return 0;
}



int main(int argc, char **argv) {
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
    listen(socket_desc, 5);
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
