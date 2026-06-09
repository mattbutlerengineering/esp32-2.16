# Host build for the Orb state-machine unit tests (no ESP-IDF required).
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -I../main/orb

test: orb_fsm_test
	./orb_fsm_test

orb_fsm_test: orb_fsm_test.c ../main/orb/orb_fsm.c ../main/orb/orb_fsm.h
	$(CC) $(CFLAGS) -o $@ orb_fsm_test.c ../main/orb/orb_fsm.c

clean:
	rm -f orb_fsm_test

.PHONY: test clean
