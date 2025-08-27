#ifndef __KSU_H_UID_OBSERVER
#define __KSU_H_UID_OBSERVER

void ksu_throne_tracker_init();

void ksu_throne_tracker_exit();

void track_throne();

bool is_uid_allow(uid_t uid);

#endif
