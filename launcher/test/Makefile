# Host build for the Launcher's pure-logic unit tests (no ESP-IDF required).
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -I../main/orb -I../main/app -I../main/motion -I../main/audio -I../main/settings -I../main/voice

test: orb_fsm_test miniapp_test tilt_test motion_app_test wav_test config_test voice_session_test ambient_test ambient_app_test
	./orb_fsm_test
	./miniapp_test
	./tilt_test
	./motion_app_test
	./wav_test
	./config_test
	./voice_session_test
	./ambient_test
	./ambient_app_test

orb_fsm_test: orb_fsm_test.c ../main/orb/orb_fsm.c ../main/orb/orb_fsm.h
	$(CC) $(CFLAGS) -o $@ orb_fsm_test.c ../main/orb/orb_fsm.c

miniapp_test: miniapp_test.c ../main/app/miniapp.c ../main/app/miniapp.h
	$(CC) $(CFLAGS) -o $@ miniapp_test.c ../main/app/miniapp.c

tilt_test: tilt_test.c ../main/motion/tilt.c ../main/motion/tilt.h
	$(CC) $(CFLAGS) -o $@ tilt_test.c ../main/motion/tilt.c -lm

motion_app_test: motion_app_test.c ../main/motion/motion_app.c ../main/motion/tilt.c ../main/motion/motion_app.h ../main/motion/tilt.h
	$(CC) $(CFLAGS) -o $@ motion_app_test.c ../main/motion/motion_app.c ../main/motion/tilt.c -lm

wav_test: wav_test.c ../main/audio/wav.c ../main/audio/wav.h
	$(CC) $(CFLAGS) -o $@ wav_test.c ../main/audio/wav.c

config_test: config_test.c ../main/settings/config.c ../main/settings/config.h
	$(CC) $(CFLAGS) -o $@ config_test.c ../main/settings/config.c

voice_session_test: voice_session_test.c ../main/voice/voice_session.c ../main/orb/orb_fsm.c ../main/voice/voice_session.h ../main/orb/orb_fsm.h
	$(CC) $(CFLAGS) -o $@ voice_session_test.c ../main/voice/voice_session.c ../main/orb/orb_fsm.c

ambient_test: ambient_test.c ../main/app/ambient.c ../main/app/ambient.h
	$(CC) $(CFLAGS) -o $@ ambient_test.c ../main/app/ambient.c -lm

ambient_app_test: ambient_app_test.c ../main/app/ambient_app.c ../main/app/ambient.c ../main/app/ambient_app.h ../main/app/ambient.h
	$(CC) $(CFLAGS) -o $@ ambient_app_test.c ../main/app/ambient_app.c ../main/app/ambient.c -lm

clean:
	rm -f orb_fsm_test miniapp_test tilt_test motion_app_test wav_test config_test voice_session_test ambient_test ambient_app_test

.PHONY: test clean
