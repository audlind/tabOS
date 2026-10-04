#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>

int main(void) {
    int fd = open("/dev/input/event3", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/input/event3");
        return 1;
    }

    struct input_absinfo abs_x, abs_y, abs_z;
    ioctl(fd, EVIOCGABS(ABS_X), &abs_x);
    ioctl(fd, EVIOCGABS(ABS_Y), &abs_y);
    ioctl(fd, EVIOCGABS(ABS_Z), &abs_z);

    int x = abs_x.value;
    int y = abs_y.value;
    int z = abs_z.value;

    printf("Initial: X=%d, Y=%d, Z=%d\n", x, y, z);

    struct input_event ev;
    int sample_count = 0;
    while (read(fd, &ev, sizeof(ev)) > 0) {
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_X) x = ev.value;
            else if (ev.code == ABS_Y) y = ev.value;
            else if (ev.code == ABS_Z) z = ev.value;
        } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
            sample_count++;
            if (sample_count % 5 == 0) {
                /* Bestem foreløpig orientering basert på aksene */
                const char *pos = "FLAT";
                int ax = abs(x);
                int ay = abs(y);
                int az = abs(z);

                if (az > ax && az > ay && az > 15000) {
                    pos = "FLAT (Lying on desk)";
                } else if (ax > ay) {
                    pos = (x > 0) ? "LANDSCAPE NORMAL" : "LANDSCAPE INVERTED";
                } else {
                    pos = (y > 0) ? "PORTRAIT NORMAL" : "PORTRAIT INVERTED";
                }

                printf("X=%6d  Y=%6d  Z=%6d  => %s\n", x, y, z, pos);
                fflush(stdout);
            }
        }
    }

    close(fd);
    return 0;
}
