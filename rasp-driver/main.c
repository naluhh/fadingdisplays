#include "IT8951.h"
#include <png.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER;
#define TRANSFER_READ_SIZE 4096 * 4

int IT8951_started = 0;
uint8_t *buffer_to_write = NULL;
int target_screen_width = 1872;
int target_screen_height = 1404;
int should_revert = 0;
uint32_t last_command_time = 0;

void abort_(const char * s)
{
    printf("%s\n", s);
    abort();
}

int read_png_file(char* file_name, int* width_ptr, int* height_ptr, png_byte *color_type_ptr, png_byte *bit_depth_ptr, uint8_t *buffer_to_write)
{
    char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        printf("[read_png_file] File %s could not be opened for reading errno = %d\n", file_name, errno);
        return 1;
    }
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        printf("[read_png_file] File %s is not recognized as a PNG file\n", file_name);
        return 1;
    }


    /* initialize stuff */
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        abort_("[read_png_file] png_create_read_struct failed");
        return 1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        abort_("[read_png_file] png_create_info_struct failed");
        return 1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        abort_("[read_png_file] Error during init_io");
        return 1;
    }

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

    int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(png_ptr))) {
        abort_("[read_png_file] Error during read_image");
        return 1;
    }


    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * *height_ptr);
    png_bytep all_bytes = (png_bytep)buffer_to_write;
    all_bytes[0] = 0;
    all_bytes[1] = 0;
    int row_size = png_get_rowbytes(png_ptr,info_ptr);
    for (int y = 0; y < *height_ptr; y++)
        row_pointers[y] = 2 + all_bytes + (y * row_size);

    png_read_image(png_ptr, row_pointers);

    free(row_pointers);
    fclose(fp);

    return 0;
}

void start_board() {
    pthread_mutex_lock(&board_mutex);
    if(!(buffer_to_write = IT8951_Init(target_screen_width, target_screen_height, should_revert))) {
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
            if (time(NULL) - last_command_time > 30) {
                stop_board();
            }
            sleep(30);
        } else {
            sleep(30);
        }
    }

    return NULL;
}

int display_4bpp_filename(char *filename) {
    last_command_time = time(NULL);
//    int started_automatically = 0;
    if (!IT8951_started) {
        printf("Update without previous start command. Starting automatically\n");
//        started_automatically = 1;
        start_board();
    }

    png_byte color_type;
    png_byte bit_depth;
    int width, height;

    printf("Reading file: %s\n", filename);
    if (read_png_file(filename, &width, &height, &color_type, &bit_depth, buffer_to_write)) {
        return 1;
    }
    if (width != target_screen_width || height != target_screen_height) {
        printf("Image should be %dx%d but it's %dx%d\n", target_screen_width, target_screen_height, width, height);
        return 1;
    }
    printf("Updating screen for file: %s\n", filename);
    pthread_mutex_lock(&board_mutex);
    IT8951_Display4BppBuffer();
    pthread_mutex_unlock(&board_mutex);

//    if (started_automatically) {
//        printf("Stopping the board because it started automatically\n");
//        stop_board();
//    }

    return 0;
}

void *connection_handler(void *socket_desc) {
    //get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char client_message[256];

    //Receive a message from client
    while ((read_size = recv(sock, client_message, 255, 0)) > 0) {
        printf("Read size: %d\n", read_size);
        if (strncmp(client_message, "S", 1) == 0) {
            if (IT8951_started) {
                printf("CAUTION, Board is already started\n");
            } else {
                start_board();
            }
            break;
        } else if (strncmp(client_message, "U", 1) == 0) {
            char *filename = client_message + 1; // Offset by one to remove the "U"
            client_message[read_size] = 0;

            // Remove \n so it's easier to try with netcat
            int idx = read_size;
            while (idx > 0) {
                --idx;
                if (client_message[idx] == '/') {
                    filename = &client_message[idx + 1];
                    break;
                }
                if (client_message[idx] == '\n') {
                    client_message[idx] = 0;
                }
            }

            FILE *test = fopen(filename, "r");
            if (test != NULL) {
                fclose(test);
                display_4bpp_filename(filename);
            }
            else {
                client_message[0] = 'D';

                char *request = malloc(read_size + 5);
                sprintf(request, "||%s||", client_message);
                printf("%s: file not found\n", filename);
                write(sock, request, strlen(request)); // request for the file
                free(request);
                char file_buffer[TRANSFER_READ_SIZE];
                FILE *fp = fopen(filename, "ab+");
                while ((read_size = recv(sock, file_buffer, TRANSFER_READ_SIZE, 0)) > 0) {
                    fwrite(file_buffer, read_size, sizeof(char), fp);
                }
                fclose(fp);
                display_4bpp_filename(filename);
            }
            break;
        } else if (strncmp(client_message, "C", 1) == 0) {
            if (!IT8951_started) {
                printf("CAUTION, Board is already stopped\n");
            } else {
                stop_board();
            }
        } else {
            client_message[read_size] = 0;
            printf("UN-handled message: %s\n", client_message);
        }
    }

    if (read_size == 0) {
        printf("Client disconnected\n");
    }
    else if (read_size == -1) {
        printf("recv failed\n");
    }

    // Free the socket pointer
    close(sock);
    free(socket_desc);
    return 0;
}

int start_server(unsigned short port) {
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create a socket\n");
        return 1;
    }

    //create a server
    struct sockaddr_in server;

    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(port); //specify the open port_number

    if (bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0 ) {
        printf("Bind failed\n");
        return 1;
    }
    printf("Bind done\n");

    //listen for new connections:
    listen(socket_desc, 1);
    printf("Waiting for new connections...\n");

    int c = sizeof(struct sockaddr_in);
    // Client to be connected
    struct sockaddr_in client;
    // New socket for client
    int new_socket, *new_sock;
    while ((new_socket = accept( socket_desc, (struct sockaddr*)&client, (socklen_t*)&c ))) {
        // Get the IP address of a client
        char *CLIENT_IP = inet_ntoa(client.sin_addr);
        int CLIENT_PORT = ntohs(client.sin_port);
        printf("New Client = {%s:%d}\n", CLIENT_IP, CLIENT_PORT);

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*)new_sock) < 0) {
            printf("could not create thread\n");
            return 1;
        }
        // Now join the thread, so that we dont terminate before the thread
        pthread_join(sniffer_thread, NULL);
    }

    if (new_socket < 0) {
        printf("Accept failed\n");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    should_revert = fopen("reverted", "r") != NULL;
    pthread_t check_board_on;
    pthread_create(&check_board_on, NULL, stop_board_loop, NULL);

    while (1) {
        start_server(8888);
        sleep(1);
    }
    return 0; // Specify the port
}


