#include "heepatia_ctrl_reg.h"
#include "heepatia_ctrl.h"
#include "heepatia.h"

#define LOCKED 1
#define UNLOCKED 0

// int *test_and_set_ptr0 = (int *)(HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_TEST_SET_0_REG_OFFSET);
// int *test_and_set_ptr1 = (int *)(HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_TEST_SET_1_REG_OFFSET);
// int *test_and_set_ptr2 = (int *)(HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_TEST_SET_2_REG_OFFSET);
// int *test_and_set_ptr3 = (int *)(HEEPATIA_CTRL_START_ADDRESS + HEEPATIA_CTRL_TEST_SET_3_REG_OFFSET);

// volatile int * test_and_set_ptr[4];

// void heepatia_ctrl_init() {
//     test_and_set_ptr[0] = test_and_set_ptr0;
//     test_and_set_ptr[1] = test_and_set_ptr1;
//     test_and_set_ptr[2] = test_and_set_ptr2;
//     test_and_set_ptr[3] = test_and_set_ptr3;
// }

// int heepatia_test_and_set(uint8_t id) {

//     int oldValue;

//     // if reading 0, the cycle after is 1 atomically
//     oldValue = *(test_and_set_ptr[id]);

//     return oldValue;
// }

// void heepatia_wait_test_and_set(uint8_t id) {
//     while(heepatia_test_and_set(id) == LOCKED);
// }

// void heepatia_release_test_and_set(uint8_t id) {
//     *test_and_set_ptr[id] = UNLOCKED;
// }