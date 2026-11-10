#ifndef PQAX_PMUCTL_H
#define PQAX_PMUCTL_H

#include <sys/ioctl.h>

#define PMUCTL_IOC_MAGIC 0xF1

/* List of PMU controls enabled by the driver. */
enum pmu_ctls {
    PM_CTL_PMCCNTR,
    PM_CTL_CNTKCTL,
    PM_CTL_CNT,
};

struct pmuctl_pmccntr_data {
    int enable;
};

#define PMU_IOC_PMCCNTR                                                    \
    _IOW(PMUCTL_IOC_MAGIC, PM_CTL_PMCCNTR, struct pmuctl_pmccntr_data)
#define PMU_IOC_CNTKCTL                                                    \
    _IOW(PMUCTL_IOC_MAGIC, PM_CTL_CNTKCTL, struct pmuctl_pmccntr_data)

#endif /* PQAX_PMUCTL_H */
