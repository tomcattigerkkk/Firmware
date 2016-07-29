#include <px4_config.h>
#include <px4_tasks.h>
#include <px4_posix.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>

#include <uORB/uORB.h>
#include <uORB/topics/sensor_combined.h>
#include <uORB/topics/vehicle_attitude.h>
#include <uORB/topics/zigbee_position.h>

__EXPORT int zigbee_main(int argc, char *argv[]);

int zigbee_main(int argc, char *argv[])
{
    PX4_INFO("Hello ZigBee!");
    int sensor_sub_fd = orb_subscribe(ORB_ID(sensor_combined));
    printf("sensor sub fd %d\n", sensor_sub_fd);

    struct zigbee_position_s zig_s;

    memset(&zig_s, 0, sizeof(zig_s));
    orb_advert_t zig_pub_fd = orb_advertise(ORB_ID(zigbee_position), &zig_s);

    int error_counter = 0;

    struct pollfd fds[] = {
	{ .fd = sensor_sub_fd,   .events = POLLIN },
	};

   for  (int i = 0; i < 10; i++) {
    /* wait for sensor update of 1 file descriptor for 1000 ms (1 second) */
    int poll_ret = px4_poll(fds, 1, 1000);
    if (poll_ret == 0) {
			/* this means none of our providers is giving us data */
			printf("[px4_simple_app] Got no data within a second\n");
		} else if (poll_ret < 0) {
			/* this is seriously bad - should be an emergency */
			if (error_counter < 10 || error_counter % 50 == 0) {
				/* use a counter to prevent flooding (and slowing us down) */
				printf("[px4_simple_app] ERROR return value from poll(): %d\n"
					, poll_ret);
			}
			error_counter++;
		} else {

    if (fds[0].revents & POLLIN) {
        /* obtained data for the first file descriptor */
        struct sensor_combined_s raw;
        /* copy sensors raw data into local buffer */
        orb_copy(ORB_ID(sensor_combined), sensor_sub_fd, &raw);
        printf("[px4_simple_app] Accelerometer:\t%8.4f\t%8.4f\t%8.4f\n",
                    (double)raw.accelerometer_m_s2[0],
                    (double)raw.accelerometer_m_s2[1],
                    (double)raw.accelerometer_m_s2[2]);

        zig_s.value = 10.0f;
        orb_publish(ORB_ID(zigbee_position), zig_pub_fd, &zig_s);
 	   }
 	}
	}
    return OK;
}