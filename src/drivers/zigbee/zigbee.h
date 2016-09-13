#ifndef _ZIGBEE_H
#define _ZIGBEE_H

static bool thread_running = false;
static bool thread_should_exit = false;

class zigbee_position_info
{
public:
    struct zigbee_position_s    _pos;
};

class ZIGBEE
{
public:
    ZIGBEE(const char *uart_path, int zigbee_anchor_num);
    virtual ~ZIGBEE();
    int init(int argc, char *argv[]);
    
   

    float qr_pos_1[3];    // the localization of the quadrotor 1
    float qr_pos_2[3];    // [0]--x [1]--y [2]--z
private:

	volatile int			_task;	
    unsigned        _baudrate;    //current baudrate
    struct zigbee_position_s _report_zigbee_position;    //msg
    orb_advert_t    _report_zigbee_position_pub;         //mavlink
    int             _serial_fd;     // serial interface to ZigBee
    char            _port[20];     //device/serial port path
    volatile int     daemon_task;   // worker task

    static void task_main(int argc, char *argv[]);
    int set_baudradte(const int fd, unsigned int baud);
    static void trans_data2array(char* buffer, float* out_arry);
    void publish(void);  //publish the zigbee position
    int pollorread(uint8_t *buf, size_t buf_length, int timeout); //poll on serial used

    int uart_init(char * uart_name);

    /**
     * Shim for calling task_main from task_create.
     */
   // static void task_main_trampoline(int argc, char *argv[]);
};

extern "C" __EXPORT int zigbee_main(int argc, char *argv[]);

#endif  //_ZIGBEE_H