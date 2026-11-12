#include <sched.h>
void main() {
    while(1) {
        sched_yield();
    }
}