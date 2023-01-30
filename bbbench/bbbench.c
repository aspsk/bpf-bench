#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jhash.h>
#include <linux/xxhash.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>

/*
 * While the time stamps has zero stddev and variance, the hash function itself
 * can have some. We run the target function HASH_LOOP_SIZE times and get the
 * mean value of results.
 */
#define HASH_LOOP_SIZE 333

/*
 * The APERF MSR register address
 */
#define MSR_IA32_APERF 0x000000e8

/*
 * It takes 117 cycles to execute MSR_IA32_APERF on my machine.
 */
#define TIME_SAMPLE_OFFSET 70

#define SIZE_OF_STAT 50
#define BOUND_OF_LOOP 50

#define time_sample_rdtsc() ({				\
	u32 low, high;					\
							\
	asm volatile ("rdtsc\n\t" 			\
			: "=a" (low), "=d" (high)	\
			:);				\
							\
	low | (u64) high << 32;				\
})

#define time_sample_rdmsr() ({				\
	u32 low, high;					\
							\
	asm volatile ("rdmsr\n\t" 			\
			: "=a" (low), "=d" (high)	\
			: "c"(MSR_IA32_APERF));		\
							\
	low | (u64) high << 32;				\
})

#define time_sample_rdtscp_start() ({                   \
        u32 low, high;                                  \
							\
        asm volatile ("LFENCE\n\t"			\
                "RDTSC\n\t"				\
                "mov %%edx, %0\n\t"			\
                "mov %%eax, %1\n\t"			\
		: "=r" (high), "=r" (low)		\
                :: "%rax", "%rdx");			\
							\
        low | (u64) high << 32;                         \
})

#define time_sample_rdtscp_end() ({                     \
        u32 low, high;                                  \
                                                        \
        asm volatile(					\
		"RDTSCP\n\t"				\
		"LFENCE\n\t"				\
		"mov %%edx, %0\n\t"			\
		"mov %%eax, %1\n\t"			\
		: "=r" (high), "=r" (low)		\
		:: "%rax", "%rdx");			\
							\
        low | (u64) high << 32;                         \
})

//#define time_sample_start time_sample_rdtsc
//#define time_sample_end time_sample_rdtsc
#define time_sample_start time_sample_rdtscp_start
#define time_sample_end time_sample_rdtscp_end
//#define time_sample_start time_sample_rdmsr
//#define time_sample_end time_sample_rdmsr

/*

On arm64 this should be something like:

    asm volatile("mrs %0, cntvct_el0" : "=r" (val));

or

    static inline notrace u64 arch_timer_read_cntpct_el0(void)
    {
            u64 cnt;
    
            asm volatile(ALTERNATIVE("isb\n mrs %0, cntpct_el0",
                                     "nop\n" __mrs_s("%0", SYS_CNTPCTSS_EL0),
                                     ARM64_HAS_ECV)
                         : "=r" (cnt));
    
            return cnt;
    }

*/

enum {
	HASH_NONE,
	HASH_JHASH,
	HASH_JHASH2,
	HASH_XXH3,
	HASH_XXH32,
	HASH_XXH64,
	HASH_BPF,
	HASH_TEST,
};

static size_t hash_func = HASH_NONE;
static size_t hash_size = 1;
static size_t hash_size_4 = 0;

static const char *hash_func_str(void)
{
	switch (hash_func) {
	case HASH_NONE:
		return "none";
	case HASH_JHASH:
		return "jhash";
	case HASH_JHASH2:
		return "jhash2";
	case HASH_XXH3:
		return "xxh3";
	case HASH_XXH32:
		return "xxh32";
	case HASH_XXH64:
		return "xxh64";
	case HASH_BPF:
		return "bpf";
	case HASH_TEST:
		return "test";
	default:
		return "_ignotum_";
	}
}

static inline u32 bpf_hash(const void *x, size_t n, u32 seed)
{
#if 0
	if (likely(((n & 3) == 0) && n <= 8))
		return jhash2(x, n >> 2, seed);
	if (n <= 16) {
		if (n > 8)
			return xxh3_9_to_16(x, n, seed);
		return jhash_1_to_7(x, n, seed);
	}
	if (n <= 240)
		return xxh3_240(x, n, seed);
	return xxhash(x, n, seed);
#endif

	return 0;
}

static inline u32 test_hash(const void *x, size_t n, u32 seed)
{
#if 0
	#include "spooky.h"
	return spooky32(x, n, seed);
#endif
	return 0;
}

#define __warm_up(foo)						\
	for (k = 0; k < 10; k++)				\
		ret = foo((void *) input, input_size, seed)

#define __benchmark(foo)					\
	start = time_sample_start();				\
	for (k = 0; k < HASH_LOOP_SIZE; k++)			\
		ret = foo((void *) input, input_size, seed);	\
	end = time_sample_end()

static inline void experiment(u64 **times, u8 *input, size_t input_size)
{
	volatile int variable = 0;
	unsigned long flags;
	u64 start, end;
	int i, j, k;
	u32 seed = 0xabde4319;
	volatile u32 ret;

	if (hash_func == HASH_JHASH2)
		input_size /= 4;

	for (j = 0; j < BOUND_OF_LOOP; j++) {
		for (i = 0; i < SIZE_OF_STAT; i++) {

			variable = 0;

			preempt_disable();
			raw_local_irq_save(flags);

			/* warm up the instruction cache */
			time_sample_start();
			time_sample_end();
			time_sample_start();
			time_sample_end();
			time_sample_start();
			time_sample_end();
			time_sample_start();
			time_sample_end();

			switch (hash_func) {
			case HASH_JHASH:
				__warm_up(jhash);
				__benchmark(jhash);
				break;
			case HASH_JHASH2:
				__warm_up(jhash2);
				__benchmark(jhash2);
				break;
			case HASH_XXH3:
				__warm_up(xxh3);
				__benchmark(xxh3);
				break;
			case HASH_XXH32:
				__warm_up(xxh32);
				__benchmark(xxh32);
				break;
			case HASH_XXH64:
				__warm_up(xxh64);
				__benchmark(xxh64);
				break;
			case HASH_BPF:
				__warm_up(bpf_hash);
				__benchmark(bpf_hash);
				break;
			case HASH_TEST:
				__warm_up(test_hash);
				__benchmark(test_hash);
				break;
			case HASH_NONE:
				start = time_sample_start();
				end = time_sample_end();
				break;
			default:
				pr_err("not today");
				return;
			}


			raw_local_irq_restore(flags);
			preempt_enable();

			if (end < start) {
				pr_err("bad counters");
				times[j][i] = 0;
			} else {
				times[j][i] = end - start;
			}
		}
	}

	/* make sure that we use the value */
	pr_info("ret=%x", ret);
}

static u64 var_calc(u64 *inputs, u64 size)
{
	u64 acc = 0, previous = 0, temp_var = 0;
	int i;

	for (i = 0; i < size; i++) {
		if (acc < previous)
			goto overflow;
		previous = acc;
		acc += inputs[i];
	}

	acc = acc * acc;
	if (acc < previous)
		goto overflow;

	previous = 0;
	for (i = 0; i < size; i++) {
		if (temp_var < previous)
			goto overflow;
		previous = temp_var;
		temp_var += inputs[i] * inputs[i];
	}

	temp_var = temp_var * size;
	if (temp_var < previous)
		goto overflow;
	return (temp_var - acc)/(size * size);

overflow:
	pr_err("CRITICAL OVERFLOW ERROR\n");
	return -EINVAL;
}

static int bbbench(u64 *mean_ret,
		   u64 *max_stddev_ret,
		   u64 *mean_variance_ret,
		   u64 *vvariance_ret)
{
	int i = 0, j = 0, spurious = 0;

	void *mem, *memp;

	u64 **times;
	u64 times_size = BOUND_OF_LOOP * sizeof(u64*);
	u64 times_item_size = SIZE_OF_STAT * sizeof(u64);

	u64 *variances;
	u64 variances_size = BOUND_OF_LOOP * sizeof(u64);

	u64 *min_values;
	u64 min_values_size = BOUND_OF_LOOP * sizeof(u64);

	u8 *input;
	u64 input_size = max(hash_size, PAGE_SIZE);

	u64 total_size = times_size + BOUND_OF_LOOP * times_item_size + variances_size + min_values_size + input_size;

	u64 max_dev, min_time, max_time, prev_min, tot_var, max_dev_all, var_of_vars, var_of_mins, mean, orig_mean, tot_mean;

	memp = mem = kmalloc(total_size, GFP_KERNEL);
	if (!mem) {
		pr_err("kmalloc(%llu)\n", total_size);
		return -ENOMEM;
	}

	times = memp;
	memp += times_size;

	for (i = 0; i < BOUND_OF_LOOP; i++, memp += times_item_size)
		times[i] = memp;

	variances = memp;
	memp += variances_size;

	min_values = memp;
	memp += min_values_size;

	/* input is the last so that  */
	input = memp;
	for (j = 0; j < input_size; j++)
		input[i] = j;

	experiment(times, input, hash_size);

	prev_min = tot_var = max_dev_all = tot_mean = 0;
	for (j = 0; j < BOUND_OF_LOOP; j++) {
		max_dev = 0;
		min_time = 0;
		max_time = 0;

		mean = 0;
		for (i = 0; i < SIZE_OF_STAT; i++) {
			if ((min_time == 0)||(min_time > times[j][i]))
				min_time = times[j][i];
			if (max_time < times[j][i])
				max_time = times[j][i];
			mean += times[j][i];
		}
		orig_mean = mean;
		if (hash_func != HASH_NONE)
			mean -= TIME_SAMPLE_OFFSET * SIZE_OF_STAT;
		mean /= SIZE_OF_STAT;
		if (hash_func != HASH_NONE)
			if (HASH_LOOP_SIZE)
				mean /= HASH_LOOP_SIZE;
		tot_mean += mean;

		max_dev = max_time - min_time;
		min_values[j] = min_time;

		if ((prev_min != 0) && (prev_min > min_time))
			spurious++;
		if (max_dev > max_dev_all)
			max_dev_all = max_dev;

		variances[j] = var_calc(times[j], SIZE_OF_STAT);

		tot_var += variances[j];

		pr_debug("loop_size:%d >>>> variance(cycles): %llu; _deviation: %llu ;min time: %llu, mean: %llu (orig = %llu)",
				j, variances[j], max_dev, min_time, mean, orig_mean);

		prev_min = min_time;
	}

	var_of_vars = var_calc(variances, BOUND_OF_LOOP);
	var_of_mins = var_calc(min_values, BOUND_OF_LOOP);
	tot_mean /= BOUND_OF_LOOP;

	pr_debug("total number of spurious min values = %d\n", spurious);
	pr_debug("total variance = %llu\n", (tot_var/BOUND_OF_LOOP));
	pr_debug("absolute max deviation = %llu\n", max_dev_all);
	pr_debug("variance of variances = %llu\n", var_of_vars);
	pr_debug("variance of minimum values = %llu\n", var_of_mins);

	*mean_ret = tot_mean;
	*max_stddev_ret = max_dev_all;
	*mean_variance_ret = var_of_mins;
	*vvariance_ret = var_of_vars;

	kfree(mem);

	return 0;
}

/*
 * Input: hash_size
 * Output: mean value, max stddev, mean variance, variance of variances
 */
static int do_it(char *buf)
{
	u64 mean, max_stddev, mean_variance, vvariance;
	int ret;

	ret = bbbench(&mean, &max_stddev, &mean_variance, &vvariance);
	if (ret)
		return ret;

	return scnprintf(buf, PAGE_SIZE,
			 "hash_func=%s hash_size=%lu mean=%llu max_stddev=%llu mean_variance=%llu vvariance=%llu\n",
			 hash_func_str(), hash_size, mean, max_stddev, mean_variance, vvariance);
}

static ssize_t run_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return do_it(buf);
}

static ssize_t run_store(struct kobject *kobj,
			 struct kobj_attribute *attr,
			 const char *bufp,
			 size_t n)
{
	char buf[16];
	long res;
	int ret;

	strncpy(buf, bufp, sizeof(buf));
	if (buf[strlen(buf)-1] == '\n')
		buf[strlen(buf)-1] = '\0';

	ret = kstrtol(buf, 10, &res);
	if (ret) {
		if (!strcmp(buf, "none")) {
			hash_func = HASH_NONE;
			return n;
		}
		if (!strcmp(buf, "jhash")) {
			hash_func = HASH_JHASH;
			return n;
		}
		if (!strcmp(buf, "jhash2")) {
			hash_func = HASH_JHASH2;
			return n;
		}
		if (!strcmp(buf, "xxh3")) {
			hash_func = HASH_XXH3;
			return n;
		}
		if (!strcmp(buf, "xxh32")) {
			hash_func = HASH_XXH32;
			return n;
		}
		if (!strcmp(buf, "xxh64")) {
			hash_func = HASH_XXH64;
			return n;
		}
		if (!strcmp(buf, "bpf")) {
			hash_func = HASH_BPF;
			return n;
		}
		if (!strcmp(buf, "test")) {
			hash_func = HASH_TEST;
			return n;
		}
		return ret;
	}

	if (res > 0) {
		hash_size = res;
		if (hash_size % 4 == 0 && hash_size <= 8)
			hash_size_4 = hash_size / 4;
		else
			hash_size_4 = 0;
	}
	else
		return -EINVAL;

	return n;
}

static struct kobj_attribute run_attr = __ATTR_RW(run);
static struct attribute *bbbench_attributes[] = {
	&run_attr.attr,
	NULL
};
static struct attribute_group bbbench_attr_group = {
	.attrs = bbbench_attributes,
};
static struct kobject *bbbench_kobj;

static int __init bbbench_init(void)
{
	int rc;

	bbbench_kobj = kobject_create_and_add("bbbench", kernel_kobj);
	if (!bbbench_kobj)
		return -ENOMEM;

	rc = sysfs_create_group(bbbench_kobj, &bbbench_attr_group);
	if (rc) {
		kobject_put(bbbench_kobj);
		return rc;
	}

	return 0;
}
module_init(bbbench_init);

static void __exit bbbench_exit(void)
{
	kobject_put(bbbench_kobj);
}
module_exit(bbbench_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anton Protopopov <aspsk@isovalent.com>");
MODULE_DESCRIPTION("Benchmark some hash functions");
