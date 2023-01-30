#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>

#define FOO(N1, N2)					\
static noinline int deep_stack_foo ## N1(int i)		\
{							\
	return deep_stack_foo ## N2(i+1);		\
}

static noinline int deep_stack_foo(int i)
{
	return i;
}

FOO(_31, )
FOO(_30, _31)
FOO(_29, _30)
FOO(_28, _29)
FOO(_27, _28)
FOO(_26, _27)
FOO(_25, _26)
FOO(_24, _25)
FOO(_23, _24)
FOO(_22, _23)
FOO(_21, _22)
FOO(_20, _21)
FOO(_19, _20)
FOO(_18, _19)
FOO(_17, _18)
FOO(_16, _17)
FOO(_15, _16)
FOO(_14, _15)
FOO(_13, _14)
FOO(_12, _13)
FOO(_11, _12)
FOO(_10, _11)
FOO(_09, _10)
FOO(_08, _09)
FOO(_07, _08)
FOO(_06, _07)
FOO(_05, _06)
FOO(_04, _05)
FOO(_03, _04)
FOO(_02, _03)
FOO(_01, _02)
FOO(_00, _01)

static ssize_t deep_stack_run_store(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *bufp,
				    size_t n)
{
	int i, N = 60;

	for (i = 0; i < N; i++) {
		if (!deep_stack_foo_00(i))
			break;
		mdelay(50);
	}

	return n;
}

static struct kobj_attribute run_attr = __ATTR_WO(deep_stack_run);
static struct attribute *deep_stack_attributes[] = {
	&run_attr.attr,
	NULL
};
static struct attribute_group deep_stack_attr_group = {
	.attrs = deep_stack_attributes,
};
static struct kobject *deep_stack_kobj;

static int __init deep_stack_init(void)
{
	int ret;

	deep_stack_kobj = kobject_create_and_add("deep-stack", kernel_kobj);
	if (!deep_stack_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(deep_stack_kobj, &deep_stack_attr_group);
	if (ret) {
		kobject_put(deep_stack_kobj);
		return ret;
	}

	return 0;
}
module_init(deep_stack_init);

static void __exit deep_stack_exit(void)
{
	kobject_put(deep_stack_kobj);
}
module_exit(deep_stack_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anton Protopopov <aspsk@isovalent.com>");
