#include <lyr/input.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef LYR_KBD_DEVICE
#define LYR_KBD_DEVICE "/dev/input/event0"
#endif

extern "C" {
int lyr_kbd_open(lyr_kbd_t *kbd) {
	int fd;

	if (!kbd) {
		errno = EINVAL;
		return -1;
	}

	fd = open(LYR_KBD_DEVICE, O_RDONLY);
	if (fd < 0)
		return -1;

	kbd->fd = fd;
	return 0;
}

int lyr_kbd_close(lyr_kbd_t *kbd) {
	int r;

	if (!kbd || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	r = close(kbd->fd);
	kbd->fd = -1;

	return r;
}

int lyr_kbd_read(lyr_kbd_t *kbd, lyr_key_event_t *ev) {
	ssize_t r;

	if (!kbd || !ev || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	r = read(kbd->fd, ev, sizeof(*ev));
	if (r < 0)
		return -1;

	if ((size_t)r != sizeof(*ev)) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int lyr_kbd_read_many(lyr_kbd_t *kbd, lyr_key_event_t *buf, size_t *count) {
	size_t max_events;
	size_t max_bytes;
	ssize_t r;

	if (!kbd || !buf || !count || *count == 0 || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	max_events = *count;
	max_bytes = max_events * sizeof(*buf);

	if (max_events != 0 && max_bytes / sizeof(*buf) != max_events) {
		errno = EOVERFLOW;
		*count = 0;
		return -1;
	}

	r = read(kbd->fd, buf, max_bytes);
	if (r < 0) {
		*count = 0;
		return -1;
	}

	if (r == 0) {
		*count = 0;
		return 0;
	}

	*count = (size_t)r / sizeof(*buf);

	if (*count == 0) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int lyr_kbd_flush(lyr_kbd_t *kbd) {
	if (!kbd || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(kbd->fd, LYR_KBDIOCFLUSH, 0);
}

int lyr_kbd_poll(lyr_kbd_t *kbd, int timeout_ms) {
	struct pollfd pfd;
	int r;

	if (!kbd || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	pfd.fd = kbd->fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	r = poll(&pfd, 1, timeout_ms);
	if (r < 0)
		return -1;

	if (r == 0)
		return 0;

	if (pfd.revents & POLLIN)
		return 1;

	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int lyr_kbd_set_layout(lyr_kbd_t *kbd, const char *path) {
	if (!kbd || !path || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(kbd->fd, LYR_KBDIOCSMAP, path);
}

int lyr_kbd_get_layout(lyr_kbd_t *kbd, char *buf) {
	if (!kbd || !buf || kbd->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	return ioctl(kbd->fd, LYR_KBDIOCGMAP, buf);
}

int lyr_key_pressed(const lyr_key_event_t *ev, uint16_t keycode) {
	return ev && ev->down && ev->keycode == keycode;
}

int lyr_key_released(const lyr_key_event_t *ev, uint16_t keycode) {
	return ev && !ev->down && ev->keycode == keycode;
}

int lyr_key_mod(const lyr_key_event_t *ev, uint16_t mod_mask) {
	return ev && (ev->mods & mod_mask);
}
}
