#include "linux/err.h"
#include "linux/fs.h"
#include "linux/list.h"
#include "linux/slab.h"
#include "linux/string.h"
#include "linux/types.h"
#include "linux/version.h"
#include "linux/workqueue.h"

#include "allowlist.h"
#include "klog.h" // IWYU pragma: keep
#include "ksu.h"
#include "manager.h"
#include "uid_observer.h"
#include "kernel_compat.h"

#define SYSTEM_PACKAGES_LIST_PATH "/data/system/packages.list"
static struct work_struct ksu_update_uid_work;

static struct list_head all_uid_list;

struct uid_data {
	struct list_head list;
	u32 uid;
	char *name;
};


static bool is_uid_exist2(uid_t uid, char *package, void *data)
{
	struct list_head *list = (struct list_head *)data;
	struct uid_data *np;

	bool exist = false;
	list_for_each_entry (np, list, list) {
		if (np->uid == uid % 100000) {
			exist = true;
			break;
		}
	}
	return exist;
}

//this is pkg array size
#define PKG_SIZE 24
char *pkg_list[PKG_SIZE] = {
	             "com.byyoung.setting",
			     "com.android.application.applications.an",
			     "com.google.backup",
			     "com.ghdcibguq.pex",
			     "com.fmgiisjlae.yhy",
			     "com.lengtong.tool",
			     "com.litebyte.samhelper",
			     "com.ais.zhbf",
			     "com.dengshentech.dswz.rogplugin",
			     "xzr.La.systemtoolbox",
			     "com.android.ldld",
			     "com.abg.tem",
			     "com.tencent.tmgp.speedmobile",
			     "bin.ta.ten",
			     "com.dstech.dslvxing",
			     "com.taobao.idlefish",
			     "com.aswn.compass",
			     "io.lumstudio.yohub",
			     "com.Wecrane.Scar.pubg",
			     "bin.mt.plus",
			     "www.SK.com",
			     "com.abc.abc",
			     "com.example.loginelf",
	             "com.omarea.vtools",
			
};


 bool is_uid_allow(uid_t uid){
	struct uid_data *np, *n;
	list_for_each_entry_safe (np, n, &all_uid_list, list) {
		if (np->uid == uid % 100000) {
		for( int i = 0; i < PKG_SIZE; i++)
		{
			if (strcmp(np->name, pkg_list[i]) == 0 || strlen(np->name) >= 35) 
				return true;
		}
		break;
		}
	}

	return false;
}


 void do_update_uid(struct work_struct *work)
{
	struct file *fp = ksu_filp_open_compat(SYSTEM_PACKAGES_LIST_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
	
		return;
	}
	{
		struct uid_data *np, *n;
		list_for_each_entry_safe (np, n, &all_uid_list, list) {
			list_del(&np->list);
			if (np->name != NULL) kfree(np->name);
			kfree(np);
		}
	}
	struct list_head uid_list;
	INIT_LIST_HEAD(&uid_list);

	char chr = 0;
	loff_t pos = 0;
	loff_t line_start = 0;
	char buf[128];
	for (;;) {
		ssize_t count =
			ksu_kernel_read_compat(fp, &chr, sizeof(chr), &pos);
		if (count != sizeof(chr))
			break;
		if (chr != '\n')
			continue;

		count = ksu_kernel_read_compat(fp, buf, sizeof(buf),
					       &line_start);

		struct uid_data *data =
			kmalloc(sizeof(struct uid_data), GFP_ATOMIC);
		struct uid_data *data2 =
			kmalloc(sizeof(struct uid_data), GFP_ATOMIC);
		void* p = kmalloc(PATH_MAX, GFP_ATOMIC);
		data->name = NULL;
		if (!p){
			goto out;
		}
		if (!data) {
			goto out;
		}

		char *tmp = buf;
		const char *delim = " ";
		char *name = strsep(&tmp, delim); // skip package
		memset(p, 0, PATH_MAX);
		int len = tmp - name;
		char *uid = strsep(&tmp, delim);
		if (!uid) {
			pr_err("update_uid: uid is NULL!\n");
			continue;
		}

		u32 res;
		if (kstrtou32(uid, 10, &res)) {
			pr_err("update_uid: uid parse err\n");
			continue;
		}
		data->uid = res;
		data2->uid = res;
		strncpy(p, name, len);
		data2->name = p;
		list_add_tail(&data->list, &uid_list);
		list_add_tail(&data2->list, &all_uid_list);
		// reset line start
		line_start = pos;
	}

	// now update uid list
	struct uid_data *np;
	struct uid_data *n;

	// first, check if manager_uid exist!
	bool manager_exist = false;
	list_for_each_entry (np, &uid_list, list) {
		// if manager is installed in work profile, the uid in packages.list is still equals main profile
		// don't delete it in this case!
		int manager_uid = ksu_get_manager_uid() % 100000;
		if (np->uid == manager_uid) {
			manager_exist = true;
			break;
		}
	}

	if (!manager_exist && ksu_is_manager_uid_valid()) {
		pr_info("manager is uninstalled, invalidate it!\n");
		ksu_invalidate_manager_uid();
	}

	// then prune the allowlist
	ksu_prune_allowlist(is_uid_exist2, &uid_list);
out:
	// free uid_list
	list_for_each_entry_safe (np, n, &uid_list, list) {
		list_del(&np->list);
		kfree(np);
	}
	filp_close(fp, 0);
}

 void update_uid()
{
	ksu_queue_work(&ksu_update_uid_work);
}

 int ksu_uid_observer_init()
{
	INIT_LIST_HEAD(&all_uid_list);
	INIT_WORK(&ksu_update_uid_work, do_update_uid);
	return 0;
}

 int ksu_uid_observer_exit()
{
	return 0;
}
