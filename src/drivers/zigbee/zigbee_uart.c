#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <drivers/drv_hrt.h>

#include <px4_config.h>
#include <px4_tasks.h>
#include <px4_posix.h>

#include <uORB/uORB.h>
#include <uORB/topics/zigbee_position.h>

//ORB_DEFINE(zigbee_position, struct zigbee_position_s);

__EXPORT int zigbee_main(int argc, char *argv[]);

float qr_pos_1[3];    // the localization of the quadrotor 1
float qr_pos_2[3];    // [0]--x [1]--y [2]--z
static bool thread_should_exit = false;
static bool thread_running = false;
static int daemon_task;

static int uart_init(char * uart_name);
static int set_uart_baudrate(const int fd, unsigned int baud);
void trans_data2array(char* buffer, float* out_arry);
static void usage(const char *reason);
int zigbee_thread_main(int argc, char *argv[]);

int set_uart_baudrate(const int fd, unsigned int baud)
{
    int speed;

    switch (baud) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        default:
            warnx("ERR: baudrate: %d\n", baud);
            return -EINVAL;
    }

    struct termios uart_config;

    int termios_state;

    /* fill the struct for the new configuration */
    tcgetattr(fd, &uart_config);
    /* clear ONLCR flag (which appends a CR for every LF) */
  //  uart_config.c_oflag &= ~ONLCR;
    /* no parity, one stop bit */
    uart_config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                        INLCR | PARMRK | INPCK | ISTRIP | IXON);

    uart_config.c_oflag = 0;


    uart_config.c_cflag &= ~(CSTOPB | PARENB);
   // uart_config.c_cflag &= ~CSIZE;
   // uart_config.c_cflag |= CS8;

    uart_config.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
   
    /* set baud rate */
    if ((termios_state = cfsetispeed(&uart_config, speed)) < 0) {
        warnx("ERR: %d (cfsetispeed)\n", termios_state);
        return false;
    }

    if ((termios_state = cfsetospeed(&uart_config, speed)) < 0) {
        warnx("ERR: %d (cfsetospeed)\n", termios_state);
        return false;
    }

    if ((termios_state = tcsetattr(fd, TCSANOW, &uart_config)) < 0) {
        warnx("ERR: %d (tcsetattr)\n", termios_state);
        return false;
    }

    return true;
}

static void usage(const char *reason)
{
    if (reason) {
        fprintf(stderr, "%s\n", reason);
    }

    fprintf(stderr, "usage: position_estimator_inav {start|stop|status} [param]\n\n");
    exit(1);
}

int uart_init(char * uart_name)
{
    int serial_fd = open(uart_name, O_RDWR | O_NOCTTY);

    if (serial_fd < 0) {
        err(1, "failed to open port: %s", uart_name);
        return false;
    }
    return serial_fd;
}

int zigbee_main(int argc, char *argv[])
{
    PX4_INFO("Hello ZigBee!");    // print a "hello" message
    
    if (argc < 2) {
        usage("missing command");
    }

    if (!strcmp(argv[1], "start")) {
        if (thread_running) {
            warnx("already running\n");
            exit(0);
        }

        thread_should_exit = false;
        daemon_task = px4_task_spawn_cmd("zigbee",
                         SCHED_DEFAULT,
                         SCHED_PRIORITY_MAX - 5,
                         2000,
                         zigbee_thread_main,
                         (argv) ? (char * const *)&argv[2] : (char * const *)NULL);
        exit(0);
    }

    if (!strcmp(argv[1], "stop")) {
        thread_should_exit = true;
        exit(0);
    }

    if (!strcmp(argv[1], "status")) {
        if (thread_running) {
            warnx("running");

        } else {
            warnx("stopped");
        }

        exit(0);
    }

    usage("unrecognized command");
    exit(1);

//     char data = '0';
//     char buffer[19] = "";
//     /*
//      * TELEM1 : /dev/ttyS1
//      * TELEM2 : /dev/ttyS2
//      * GPS    : /dev/ttyS3
//      * NSH    : /dev/ttyS5
//      * SERIAL4: /dev/ttyS6
//      * N/A    : /dev/ttyS4
//      * IO DEBUG (RX only):/dev/ttyS0
//      */
//     int uart_read = uart_init("/dev/ttyS2");
//     if(false == uart_read)return -1;
//     if(false == set_uart_baudrate(uart_read,9600))   // 9600 is suggested
//     {
//         printf("set_uart_baudrate is failed\n");
//         return -1;
//     }
//     printf("uart init is successful\n");
    

//     struct  zigbee_position_s zigbee_str;
//     memset(&zigbee_str, 0, sizeof(zigbee_str));

//     orb_advert_t zigbee_pub = orb_advertise(ORB_ID(zigbee_position), &zigbee_str);

//     char index= 0;
//     int loop = 0;
//     while(loop<6){
//         loop++;
//         usleep(50000);
//         read(uart_read,&data,1);

//         if(data == 'Q'){

//             buffer[0]= 'Q';
//             for(int i = 1;i <19;++i){
//                 read(uart_read,&data,1);
//                 buffer[i] = data;
//                 data = '0';
//             }
//             if(buffer[0]=='Q'&&buffer[2]=='['&&buffer[4]=='.'&&buffer[17]==']'
//              &&buffer[14]=='.'&&buffer[9]=='.')
//           //      printf("%s\n",buffer);

//             // copy string to the structure of zigbee 
//            memcpy(zigbee_str.zig_msg, buffer, sizeof(buffer)); 

//            index = buffer[1];  // index indicates the message comes from w
//           // printf("%s\n", index );
//             switch (index)
//             {   
//                 case '1':
//                         trans_data2array(&buffer[0], &qr_pos_1[0]);
//                         memcpy(zigbee_str.position, qr_pos_1, sizeof(qr_pos_1));
//                         break;
//                 case '2':
//                         trans_data2array(&buffer[0], &qr_pos_2[0]);
//                         memcpy(zigbee_str.position, qr_pos_2, sizeof(qr_pos_2));

//                          printf("x position %8.4f\n", (double)qr_pos_2[0]);
//                         // printf("y position %8.4f\n", (double)qr_pos_2[1]);
//                         // printf("z position %8.4f\n", (double)qr_pos_2[2]);
//                         break;
//                 default:
//                         printf("message index error");
//                         break;
//             }
//             orb_publish(ORB_ID(zigbee_position), zigbee_pub, &zigbee_str);
//             memset(buffer, 0, sizeof(buffer));
//    }




// }

    

    return 0;
}


int zigbee_thread_main(int argc, char *argv[])
{
    if (argc < 2) {
        errx(1, "need a serial port name as argument");
        usage("eg:");
    }

     char *uart_name = argv[1];

    warnx("opening port %s", uart_name);

    char data = '0';
    char buffer[19] = "";
    
    char index; 

    /*
     * TELEM1 : /dev/ttyS1
     * TELEM2 : /dev/ttyS2
     * GPS    : /dev/ttyS3
     * NSH    : /dev/ttyS5
     * SERIAL4: /dev/ttyS6
     * N/A    : /dev/ttyS4
     * IO DEBUG (RX only):/dev/ttyS0
     */
    int uart_read = uart_init(uart_name); // chose TELEM2 port
    if(false == uart_read)return -1;
    if(false == set_uart_baudrate(uart_read,9600)){          
        printf("set_uart_baudrate is failed\n");
        return -1;
    }
    printf("uart init is successful\n");

    thread_running = true; 


    struct  zigbee_position_s zigbee_str;
    memset(&zigbee_str, 0, sizeof(zigbee_str));

    orb_advert_t zigbee_pub = orb_advertise(ORB_ID(zigbee_position), &zigbee_str);
    

    
    while(!thread_should_exit)
    {
       
        usleep(50000);
        read(uart_read,&data,1);

        if(data == 'Q'){

            buffer[0]= 'Q';
            for(int i = 1;i <19;++i){
                read(uart_read,&data,1);
                buffer[i] = data;
                data = '0';
            }
            if(buffer[0]=='Q'&&buffer[2]=='['&&buffer[4]=='.'&&buffer[17]==']'
             &&buffer[14]=='.'&&buffer[9]=='.')
          //      printf("%s\n",buffer);

            // copy string to the structure of zigbee 
           memcpy(zigbee_str.zig_msg, buffer, sizeof(buffer)); 

           index = buffer[1];  // index indicates the message comes from w
          // printf("%s\n", index );
            switch (index)
            {   
                case '1':
                        trans_data2array(&buffer[0], &qr_pos_1[0]);
                        memcpy(zigbee_str.position, qr_pos_1, sizeof(qr_pos_1));
                        break;
                case '2':
                        trans_data2array(&buffer[0], &qr_pos_2[0]);
                        memcpy(zigbee_str.position, qr_pos_2, sizeof(qr_pos_2));

                        // printf("x position %8.4f\n", (double)qr_pos_2[0]);
                        // printf("y position %8.4f\n", (double)qr_pos_2[1]);
                        // printf("z position %8.4f\n", (double)qr_pos_2[2]);
                        break;
                default:
                        printf("message index error");
                        break;
            }
            orb_publish(ORB_ID(zigbee_position), zigbee_pub, &zigbee_str);
            memset(buffer, 0, sizeof(buffer));

          }   


    }

    return 0;
}

void trans_data2array(char* buffer, float* out_arry)
{
    *out_arry = (float)(*(buffer+3)-48)+(float)(*(buffer+5)-48)*0.1f
        +(float)(*(buffer+6)-48)*0.01f;
    *(out_arry+1) = (float)(*(buffer+8)-48)+(float)(*(buffer+10)-48)*0.1f
        +(float)(*(buffer+11)-48)*0.01f;
    *(out_arry+2) = (float)(*(buffer+13)-48)+(float)(*(buffer+15)-48)*0.1f
        +(float)(*(buffer+16)-48)*0.01f;
}

         // if(data == 'Q'){
         //    buffer[0] = data;            // put the first "Q" in a string
         //    for(int i = 1;i <18;i++){
         //        read(uart_read,&data,1);
         //        buffer[i] = data;
         //        data = '0';
         //    }
         //    printf("%s\n",buffer); // print out the received localization message
         //    usleep(50000);
        //    // copy string to the structure of zigbee 
        //    memcpy(zigbee_str.zig_msg, buffer, sizeof(buffer)); 

        //    index = buffer[1];  // index indicates the message comes from w

        //     switch (index)
        //     {   
        //         case '1':
        //             trans_data2array(&buffer[0], &qr_pos_1[0]);
        //             memcpy(zigbee_str.position, qr_pos_1, sizeof(qr_pos_1));
        //                 break;
        // case '2':
        //                 trans_data2array(&buffer[0], &qr_pos_2[0]);
        //                 memcpy(zigbee_str.position, qr_pos_2, sizeof(qr_pos_2));
        //                 break;
        //         default:
        //                 printf("message index error");
        //                 break;
        //     }


            // publish the data 
             // orb_publish(ORB_ID(zigbee_position), zigbee_pub, &zigbee_str);
        

         // if(buffer[0]=='Q'&&buffer[2]=='['&&buffer[4]=='.'&&buffer[17]==']'
         //    &&buffer[14]=='.'&&buffer[9]=='.')
         // printf("%s\n",buffer);
        // if(data == 'Q')  // && update
        // {
        //     memset(buffer, 0, sizeof(buffer));
        //     buffer[0]='Q';
        //     update = false;

        // }
        // else if (!update)
        // {
        //     //for(int i = 1;i <18;i++)
        //     {
        //        // read(uart_read,&data,1);
        //         i++;
        //         buffer[i] = data;
        //         data = '0';
        //         if (i == 17 )
        //         {
        //             update = true;
        //             i= 0; 
        //             printf("%s\n",buffer);
        //         }
            
        //     }
            
        // }





