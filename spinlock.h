#ifndef __SPINLOCK_H
#define __SPINLOCK_H

typedef volatile unsigned int atomic_t;

void spin_init();
void spin_lock(atomic_t *lock, int id);
void spin_unlock(atomic_t *lock, int id);

#endif
