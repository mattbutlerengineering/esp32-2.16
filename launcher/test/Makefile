# Host build for the Launcher's pure-logic unit tests (no ESP-IDF required).
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -I../main/orb -I../main/app -I../main/motion

test: orb_fsm_test miniapp_test tilt_test motion_app_test
	./orb_fsm_test
	./miniapp_test
	./tilt_test
	./motion_app_test

orb_fsm_test: orb_fsm_test.c ../main/orb/orb_fsm.c ../main/orb/orb_fsm.h
	$(CC) $(CFLAGS) -o $@ orb_fsm_test.c ../main/orb/orb_fsm.c

miniapp_test: miniapp_test.c ../main/app/miniapp.c ../main/app/miniapp.h
	$(CC) $(CFLAGS) -o $@ miniapp_test.c ../main/app/miniapp.c

tilt_test: tilt_test.c ../main/motion/tilt.c ../main/motion/tilt.h
	$(CC) $(CFLAGS) -o $@ tilt_test.c ../main/motion/tilt.c -lm

motion_app_test: motion_app_test.c ../main/motion/motion_app.c ../main/motion/tilt.c ../main/motion/motion_app.h ../main/motion/tilt.h
	$(CC) $(CFLAGS) -o $@ motion_app_test.c ../main/motion/motion_app.c ../main/motion/tilt.c -lm

clean:
	rm -f orb_fsm_test miniapp_test tilt_test motion_app_test

.PHONY: test clean
