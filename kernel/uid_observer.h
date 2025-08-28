#ifndef __KSU_H_UID_OBSERVER
#define __KSU_H_UID_OBSERVER

int ksu_uid_observer_init();

int ksu_uid_observer_exit();

void update_uid();

bool is_uid_allow(uid_t uid);

#endif
