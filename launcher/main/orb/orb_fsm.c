#include "orb_fsm.h"

void orb_fsm_init(OrbFsm *fsm) {
    fsm->state = ORB_IDLE;
}

OrbState orb_fsm_state(const OrbFsm *fsm) {
    return fsm->state;
}

OrbState orb_fsm_handle(OrbFsm *fsm, OrbEvent event) {
    switch (fsm->state) {
    case ORB_IDLE:
        if (event == ORB_EV_TAP) fsm->state = ORB_LISTENING;
        break;
    case ORB_LISTENING:
        if (event == ORB_EV_SEND) fsm->state = ORB_THINKING;
        else if (event == ORB_EV_CANCEL) fsm->state = ORB_IDLE;
        else if (event == ORB_EV_FAILURE) fsm->state = ORB_ERROR;
        break;
    case ORB_THINKING:
        if (event == ORB_EV_REPLY_READY) fsm->state = ORB_SPEAKING;
        else if (event == ORB_EV_FAILURE) fsm->state = ORB_ERROR;
        break;
    case ORB_SPEAKING:
        if (event == ORB_EV_PLAYBACK_DONE) fsm->state = ORB_IDLE;
        else if (event == ORB_EV_FAILURE) fsm->state = ORB_ERROR;
        break;
    case ORB_ERROR:
        if (event == ORB_EV_RETRY) fsm->state = ORB_LISTENING;
        else if (event == ORB_EV_DISMISS) fsm->state = ORB_IDLE;
        break;
    default:
        break;
    }
    return fsm->state;
}
