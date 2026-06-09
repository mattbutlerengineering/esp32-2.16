# Host build for the Launcher's pure-logic unit tests (no ESP-IDF required).
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -I../main/orb -I../main/app

test: orb_fsm_test miniapp_test
	./orb_fsm_test
	./miniapp_test

orb_fsm_test: orb_fsm_test.c ../main/orb/orb_fsm.c ../main/orb/orb_fsm.h
	$(CC) $(CFLAGS) -o $@ orb_fsm_test.c ../main/orb/orb_fsm.c

miniapp_test: miniapp_test.c ../main/app/miniapp.c ../main/app/miniapp.h
	$(CC) $(CFLAGS) -o $@ miniapp_test.c ../main/app/miniapp.c

clean:
	rm -f orb_fsm_test miniapp_test

.PHONY: test clean
