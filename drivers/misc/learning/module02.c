#include <linux/module.h>
#include <linux/init.h>

int __init mod_init(void)
{
	pr_info("mod_init called\n");
	return 0;
}

void __exit mod_cleanup(void)
{
	pr_info("mod_cleanup called\n");
}

module_init(mod_init);
module_exit(mod_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Homuth");
MODULE_DESCRIPTION("Example module");
