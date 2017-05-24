
#include "aimtrak.h"


#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


#ifdef DEBUG
#define log(X) printf X
#define log_debug(X) printf X
#else
#define log(X)
#define log_debug(X)
#endif


#define AIMTRAK_VENDOR_ID   ((unsigned short)0xd209)



static int test_bit(unsigned bit, unsigned char* evtype_bitmask)
{
    return (evtype_bitmask[bit/8] & (1 << (bit % 8))) != 0;
}


/* test if at least one of the feature is present */
static int test_bit_feature(int f, unsigned bit, unsigned char* evtype_bitmask, const int* feature)
{
	unsigned i;
	unsigned char bitmask[64];

    if (!test_bit(bit, evtype_bitmask))
		return 0;

	memset(bitmask, 0, sizeof(bitmask));

	if (ioctl(f, EVIOCGBIT(bit, sizeof(bitmask)), bitmask) < 0) {
        log(("error in ioctl(EVIOCGBIT(0x%x))\n", bit));
		return 0;
	}

	for(i=0;feature[i] >= 0;++i)
        if (test_bit(feature[i], bitmask)) {
            return 1;
	}

	return 0;
}


static int is_abs_mouse(int f, unsigned char* evtype_bitmask)
{
    int axis[] = {ABS_X, ABS_Y, -1};
    int buttons[] = {BTN_LEFT, BTN_MIDDLE, BTN_RIGHT, -1};
    return ((test_bit_feature(f, EV_ABS, evtype_bitmask, axis) ) &&
             test_bit_feature(f, EV_KEY, evtype_bitmask, buttons) );

}


static int init_axis(struct aimtrak_context* dev_p)
{
    unsigned char abs_bitmask[ABS_MAX/8 + 1];
    int idx;
    int features[5];
    struct aimtrak_axis* axis_p;
    int axis[] = {ABS_X, ABS_Y};

    memset(abs_bitmask, 0, sizeof(abs_bitmask));
    if (ioctl(dev_p->fd, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask) < 0) {
        log(("event: error in ioctl(EVIOCGBIT(EV_ABS,%d))\n", (int)ABS_MAX));
        return 0;
    }

    for (idx=0 ; idx<MAX_AXIS ; ++idx) {
        axis_p = &dev_p->state.axis[idx];

        if (test_bit(axis[idx], abs_bitmask)) {
            memset(features, 0, sizeof(features));
            if (ioctl(dev_p->fd, EVIOCGABS(axis[idx]), features) >= 0) {
                axis_p->min = features[1];
                axis_p->max = features[2];
                axis_p->value = (axis_p->max - axis_p->min) / 2;
                continue;
            }
        }

        log(("axis not available\n"));
        return 0;
    }

    return 1;
}


static int aimtrak_read(int f, int* type, int* code, int* value)
{
	int size;
	struct input_event e;

	size = read(f, &e, sizeof(e));

	if (size == -1 && errno == EAGAIN) {
		/* normal exit if data is missing */
		return -1;
	}

	if (size != sizeof(e)) {
        log(("invalid read size %d on the event interface, errno %d (%s)\n", size, errno, strerror(errno)));
		return -1;
	}

    log_debug(("read time %ld.%06ld, type %d, code %d, value %d\n", e.time.tv_sec, e.time.tv_usec, e.type, e.code, e.value));

	*type = e.type;
	*code = e.code;
	*value = e.value;

	return 0;
}


int aimtrak_open(const char* file, unsigned char* evtype_bitmask, unsigned evtype_size)
{
    int f;

    f = open(file, O_RDONLY | O_NONBLOCK);
    if (f == -1) {
        if (errno != ENODEV) {
            log(("error opening device %s, errno %d (%s)\n", file, errno, strerror(errno)));
        }
        goto err;
    }

    if (evtype_bitmask) {
        memset(evtype_bitmask, 0, evtype_size);
        if (ioctl(f, EVIOCGBIT(0, EV_MAX), evtype_bitmask) < 0) {
            log(("error in ioctl(EVIOCGBIT(0,%d)) on device %s\n", (int)EV_MAX, file));
            goto err_close;
        }
    }

    return f;

err_close:
    close(f);
err:
    return -1;
}


int aimtrak_locate(int product, struct aimtrak_context* dev_p)
{
	unsigned i;
    #define EVENT_MAX 20

    for( i=0 ; i<EVENT_MAX ; ++i)
    {
        char file[128];
		unsigned short device_info[4];
		unsigned char evtype_bitmask[EV_MAX/8 + 1];


        snprintf(file, sizeof(file), "/dev/input/event%d", i);

        /* reset struct */
        memset(dev_p, 0, sizeof(struct aimtrak_context));

        dev_p->fd = aimtrak_open(file, evtype_bitmask, sizeof(evtype_bitmask));
        if (dev_p->fd == -1) {
            if (errno == EACCES) {
                log(("can't open device %s\n", file));
			}
			continue;
		}

        /* we have found a device: get info */
        if (ioctl(dev_p->fd, EVIOCGID, &device_info)) {
            log(("error in ioctl(EVIOCGID)\n"));
            aimtrak_close(dev_p);
			continue;
		}

        /* test device */
        if( (device_info[ID_VENDOR] == AIMTRAK_VENDOR_ID ) &&
            ( (product == -1) || (device_info[ID_PRODUCT] == (unsigned short)product) ) &&
            is_abs_mouse(dev_p->fd, evtype_bitmask) )
        {
            /* we have found our device */
            /* init axis */
            if (init_axis(dev_p))
                return 1;
        }

        aimtrak_close(dev_p);
	}

    /* no device found */
    return 0;
}


int aimtrak_poll(struct aimtrak_context* dev_p)
{
    int type, code, value;
    int nbEvents = 0;
    int index = -1;

    while(aimtrak_read(dev_p->fd, &type, &code, &value) == 0) {
        index = -1;
        switch(type) {
            case EV_ABS:
                switch(code) {
                    case ABS_X: index = IDX_X; break;
                    case ABS_Y: index = IDX_Y; break;
                    default: {}
                }
                if (index>=0) {
                    dev_p->state.axis[index].value = value;
                    nbEvents++;
                }
                break;
            case EV_KEY:
                switch(code) {
                    case BTN_LEFT: index = IDX_LEFT; break;
                    case BTN_MIDDLE: index = IDX_MIDDLE; break;
                    case BTN_RIGHT: index = IDX_RIGHT; break;
                    default: {}
                }
                if (index>=0) {
                    dev_p->state.btns[index].pressed = (value != 0);
                    nbEvents++;
                }
                break;
            default: {}
         }
    }

    return nbEvents;
}


void aimtrak_close(struct aimtrak_context* dev_p)
{
    if(-1 != dev_p->fd) {
        close(dev_p->fd);
        dev_p->fd = -1;
    }
}
