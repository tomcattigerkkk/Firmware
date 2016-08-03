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

__EXPORT int zigbee_main(int argc, char *argv[]);

static int uart_init(char * uart_name);
static int set_uart_baudrate(const int fd, unsigned int baud);

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
    char data = '0';
    char buffer[19] = "";
    /*
     * TELEM1 : /dev/ttyS1
     * TELEM2 : /dev/ttyS2
     * GPS    : /dev/ttyS3
     * NSH    : /dev/ttyS5
     * SERIAL4: /dev/ttyS6
     * N/A    : /dev/ttyS4
     * IO DEBUG (RX only):/dev/ttyS0
     */
    int uart_read = uart_init("/dev/ttyS2");
    if(false == uart_read)return -1;
    if(false == set_uart_baudrate(uart_read,9600)){
        printf("set_uart_baudrate is failed\n");
        return -1;
    }
    printf("uart init is successful\n");
    //bool  update  =  true;
    //int i = 0;
    while(true){

        usleep(50000);
         read(uart_read,&data,1);
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
        if(data == 'Q'){

            buffer[0]= 'Q';
            for(int i = 1;i <19;++i){
                read(uart_read,&data,1);
                buffer[i] = data;
                data = '0';
            }
            if(buffer[0]=='Q'&&buffer[2]=='['&&buffer[4]=='.'&&buffer[17]==']'
             &&buffer[14]=='.'&&buffer[9]=='.')
                printf("%s\n",buffer);
            memset(buffer, 0, sizeof(buffer));
   }
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
        
    

    return 0;
}