#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <drivers/drv_hrt.h>

#include <nuttx/clock.h>
#include <nuttx/arch.h>

/*uORB*/
#include <uORB/uORB.h>
#include <uORB/topics/zigbee_position.h>

#include "drv_zigbee.h"
#include "zigbee.h"

#include <px4_config.h>
#include <px4_tasks.h>
#include <px4_posix.h>


namespace
{
    ZIGBEE   *zig_c = nullptr;
    volatile bool is_zigbee_advertised = false; 
}

ZIGBEE::ZIGBEE(const char *uart_path, int zigbee_anchor_num)
{
	/*store port name*/
	strncpy(_port, uart_path, sizeof(_port));
	/*enforce null termination*/
	_port[sizeof(_port)-1] = '\0';

	/*initial message handle*/
	memset(&_report_zigbee_position, 0, sizeof(_report_zigbee_position));


}

ZIGBEE::~ZIGBEE()
{
        /* tell the task we want it to go away */
    thread_should_exit = true;

    /* spin waiting for the task to stop */
    for (unsigned i = 0; (i < 10) && (_task != -1); i++) {
        /* give it another 100ms */
        usleep(100000);
    }

    /* well, kill it anyway, though this will probably crash */
    if (_task != -1) {
        px4_task_delete(_task);
    }
}

int ZIGBEE::init(int argc, char *argv[])
{
	thread_should_exit = false;
    thread_running = false;
        daemon_task = px4_task_spawn_cmd("zigbee",
                         SCHED_DEFAULT,
                         SCHED_PRIORITY_MAX - 5,
                         2000,
                         (px4_main_t)&ZIGBEE::task_main,
                         (argv) ? (char * const *)&argv[2] : (char * const *)NULL);
        exit(0);

    
}

int ZIGBEE::uart_init(char * uart_name)
{
    int serial_fd = open(uart_name, O_RDWR | O_NOCTTY);

    if (serial_fd < 0) {
        err(1, "failed to open port: %s", uart_name);
        return false;
    }
    return serial_fd;
}

// static void task_main_trampoline(int argc, char *argv[])
// {
//    // zig_c->task_main();
    
// }


void ZIGBEE::task_main(int argc, char *argv[])
{
	if (argc < 2) {
        errx(1, "need a serial port name as argument");
      
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
    int uart_read = zig_c->uart_init(uart_name); // chose TELEM2 port
  //  if(false == uart_read)return -1;
    if(false == zig_c->set_baudradte(uart_read,9600)){          
        printf("set_uart_baudrate is failed\n");
        //return -1;
    }
    printf("uart init is successful in C++\n");

    thread_running = true; 


    struct  zigbee_position_s zigbee_str;
    memset(&zigbee_str, 0, sizeof(zigbee_str));

    orb_advert_t zigbee_pub = orb_advertise(ORB_ID(zigbee_position), &zigbee_str);
    

    
    while(!thread_should_exit)
    {
       
        usleep(20000);
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
                        trans_data2array(&buffer[0], &(zig_c->qr_pos_1[0]));
                        memcpy(zigbee_str.position, zig_c->qr_pos_1, sizeof(zig_c->qr_pos_1));
                        break;
                case '2':
                        trans_data2array(&buffer[0], &zig_c->qr_pos_2[0]);
                        memcpy(zigbee_str.position, zig_c->qr_pos_2, sizeof(zig_c->qr_pos_2));

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

   // return 0;


}

int ZIGBEE::set_baudradte(const int fd, unsigned int baud)
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

void ZIGBEE::trans_data2array(char* buffer, float* out_arry)
{
    *out_arry = (float)(*(buffer+3)-48)+(float)(*(buffer+5)-48)*0.1f
        +(float)(*(buffer+6)-48)*0.01f;
    *(out_arry+1) = (float)(*(buffer+8)-48)+(float)(*(buffer+10)-48)*0.1f
        +(float)(*(buffer+11)-48)*0.01f;
    *(out_arry+2) = (float)(*(buffer+13)-48)+(float)(*(buffer+15)-48)*0.1f
        +(float)(*(buffer+16)-48)*0.01f;
}

namespace zigbee_control
{
    void start(const char *uart_path, int argc, char *argv[]);
    void stop();



    void start(const char *uart_path, int argc, char *argv[])
    {
        if(zig_c != nullptr)
        {
            PX4_WARN("ZigBee is running");
            return;
        }

        zig_c = new ZIGBEE(uart_path, 4);
        zig_c->init(argc, argv);
    }

    void stop()
    {
        delete zig_c;
        zig_c = nullptr;
    }

}

int zigbee_main(int argc, char *argv[])
{
	const char *device_port = ZIGBEE_DEFAULT_UART_PORT;

	PX4_INFO("Hello ZigBee!");    // print a "hello" message
    
	if (argc < 2) {
	        warnx("missing command");
	    }

        if (!strcmp(argv[1], "start")) {
        if (thread_running) {
            warnx("already running\n");
            exit(0);
        }

        zigbee_control::start(device_port, argc, argv);
        
        
    }

    if (!strcmp(argv[1], "stop")) {
        thread_should_exit = true;
        zigbee_control::stop();
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

    warnx("unrecognized command");
    exit(1);
}

