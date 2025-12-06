#include "include/memory.h"
#include "include/string.h"
#include "process/pcb.h"
#include "ut/ut_framework.h"
#include "utils/queue.h"
#include <stdint.h>

/*=============================================================================
 * TEST CASES - PCB Basic Functions
 *===========================================================================*/

int ut_pcb_create_destroy(void) {
  debugf("  Creating PCB with PID 0 (auto-assign)...\n");
  Pcb *pcb = pcb_create(0, "test_process", PROCESS_PRIORITY_NORMAL);

  UT_ASSERT_NOT_NULL(pcb, "PCB creation failed");
  UT_ASSERT(pcb->pid >= 1, "PID should be auto-assigned to >= 1");
  UT_ASSERT_STR_EQUAL("test_process", pcb->name, "Process name mismatch");
  UT_ASSERT_EQUAL(PROCESS_PRIORITY_NORMAL, pcb->priority, "Priority mismatch");
  UT_ASSERT_EQUAL(PROCESS_STATE_NEW, pcb->state, "Initial state should be NEW");

  debugf("  Created PCB: PID=%u, Name=%s, Priority=%s, State=%s\n", pcb->pid,
         pcb->name, pcb_priority_string_get(pcb->priority),
         pcb_state_string_get(pcb->state));

  pcb_destroy(pcb);
  return UT_PASS;
}

int ut_pcb_create_with_pid(void) {
  debugf("  Creating PCB with specific PID...\n");
  Pcb *pcb = pcb_create(42, "specific_pid", PROCESS_PRIORITY_HIGH);

  UT_ASSERT_NOT_NULL(pcb, "PCB creation failed");
  UT_ASSERT_EQUAL(42, pcb->pid, "PID should be 42");
  UT_ASSERT_STR_EQUAL("specific_pid", pcb->name, "Process name mismatch");
  UT_ASSERT_EQUAL(PROCESS_PRIORITY_HIGH, pcb->priority,
                  "Priority should be HIGH");

  pcb_destroy(pcb);
  return UT_PASS;
}

int ut_pcb_create_no_name(void) {
  debugf("  Creating PCB without name (auto-generate)...\n");
  Pcb *pcb = pcb_create(0, NULL, PROCESS_PRIORITY_LOW);

  UT_ASSERT_NOT_NULL(pcb, "PCB creation failed");
  UT_ASSERT(strlen(pcb->name) > 0, "Name should be auto-generated");
  debugf("  Auto-generated name: %s\n", pcb->name);

  pcb_destroy(pcb);
  return UT_PASS;
}

int ut_pcb_state_transitions(void) {
  debugf("  Testing state transitions...\n");
  Pcb *pcb = pcb_create(0, "state_test", PROCESS_PRIORITY_NORMAL);

  UT_ASSERT_NOT_NULL(pcb, "PCB creation failed");
  UT_ASSERT_EQUAL(PROCESS_STATE_NEW, pcb->state, "Initial state should be NEW");

  pcb_state_set(pcb, PROCESS_STATE_READY);
  UT_ASSERT_EQUAL(PROCESS_STATE_READY, pcb->state, "State should be READY");

  pcb_state_set(pcb, PROCESS_STATE_RUNNING);
  UT_ASSERT_EQUAL(PROCESS_STATE_RUNNING, pcb->state, "State should be RUNNING");

  pcb_state_set(pcb, PROCESS_STATE_WAITING);
  UT_ASSERT_EQUAL(PROCESS_STATE_WAITING, pcb->state, "State should be WAITING");

  pcb_state_set(pcb, PROCESS_STATE_TERMINATED);
  UT_ASSERT_EQUAL(PROCESS_STATE_TERMINATED, pcb->state,
                  "State should be TERMINATED");

  pcb_destroy(pcb);
  return UT_PASS;
}

int ut_pcb_context_init(void) {
  debugf("  Testing CPU context initialization...\n");
  Pcb *pcb = pcb_create(0, "context_test", PROCESS_PRIORITY_NORMAL);

  UT_ASSERT_NOT_NULL(pcb, "PCB creation failed");

  uint32_t entry_point = 0x100000;
  uint32_t stack_top = 0x200000;

  pcb_context_init(pcb, entry_point, stack_top);

  UT_ASSERT_EQUAL(entry_point, pcb->cpu_context.eip, "EIP mismatch");
  UT_ASSERT_EQUAL(stack_top, pcb->cpu_context.esp, "ESP mismatch");
  UT_ASSERT_EQUAL(stack_top, pcb->cpu_context.ebp, "EBP mismatch");
  UT_ASSERT_EQUAL(0x202, pcb->cpu_context.eflags,
                  "EFLAGS should enable interrupts");

  pcb_destroy(pcb);
  return UT_PASS;
}

int ut_pcb_multiple_create(void) {
  debugf("  Creating multiple PCBs with auto-incrementing PIDs...\n");
  Pcb *pcb1 = pcb_create(0, "proc1", PROCESS_PRIORITY_NORMAL);
  Pcb *pcb2 = pcb_create(0, "proc2", PROCESS_PRIORITY_NORMAL);
  Pcb *pcb3 = pcb_create(0, "proc3", PROCESS_PRIORITY_NORMAL);

  UT_ASSERT_NOT_NULL(pcb1, "PCB1 creation failed");
  UT_ASSERT_NOT_NULL(pcb2, "PCB2 creation failed");
  UT_ASSERT_NOT_NULL(pcb3, "PCB3 creation failed");

  UT_ASSERT(pcb2->pid > pcb1->pid, "PID should auto-increment");
  UT_ASSERT(pcb3->pid > pcb2->pid, "PID should auto-increment");

  debugf("  Created PIDs: %u, %u, %u\n", pcb1->pid, pcb2->pid, pcb3->pid);

  pcb_destroy(pcb1);
  pcb_destroy(pcb2);
  pcb_destroy(pcb3);
  return UT_PASS;
}

int ut_pcb_string_conversions(void) {
  debugf("  Testing string conversion functions...\n");

  const char *state_str = pcb_state_string_get(PROCESS_STATE_RUNNING);
  UT_ASSERT_STR_EQUAL("RUNNING", state_str, "State string mismatch");

  state_str = pcb_state_string_get(PROCESS_STATE_READY);
  UT_ASSERT_STR_EQUAL("READY", state_str, "State string mismatch");

  const char *priority_str = pcb_priority_string_get(PROCESS_PRIORITY_HIGH);
  UT_ASSERT_STR_EQUAL("HIGH", priority_str, "Priority string mismatch");

  priority_str = pcb_priority_string_get(PROCESS_PRIORITY_LOW);
  UT_ASSERT_STR_EQUAL("LOW", priority_str, "Priority string mismatch");

  return UT_PASS;
}

/*=============================================================================
 * TEST CASES - PCB with Queue
 *===========================================================================*/

int ut_pcb_with_queue(void) {
  debugf("  Testing PCB with existing queue implementation...\n");
  Queue queue;
  queue_init(&queue);

  UT_ASSERT(queue_is_empty(&queue), "Queue should be empty");

  // Create some PCBs
  Pcb *pcb1 = pcb_create(1, "proc1", PROCESS_PRIORITY_NORMAL);
  Pcb *pcb2 = pcb_create(2, "proc2", PROCESS_PRIORITY_NORMAL);
  Pcb *pcb3 = pcb_create(3, "proc3", PROCESS_PRIORITY_NORMAL);

  // Enqueue them
  enqueue(&queue, pcb1);
  enqueue(&queue, pcb2);
  enqueue(&queue, pcb3);

  UT_ASSERT(!queue_is_empty(&queue), "Queue should not be empty");

  // Dequeue and verify FIFO order
  Pcb *dequeued1 = (Pcb *)dequeue(&queue);
  UT_ASSERT_EQUAL(pcb1, dequeued1, "First dequeued should be pcb1");

  Pcb *dequeued2 = (Pcb *)dequeue(&queue);
  UT_ASSERT_EQUAL(pcb2, dequeued2, "Second dequeued should be pcb2");

  Pcb *dequeued3 = (Pcb *)dequeue(&queue);
  UT_ASSERT_EQUAL(pcb3, dequeued3, "Third dequeued should be pcb3");

  UT_ASSERT(queue_is_empty(&queue),
            "Queue should be empty after dequeuing all");

  pcb_destroy(pcb1);
  pcb_destroy(pcb2);
  pcb_destroy(pcb3);
  return UT_PASS;
}

/*=============================================================================
 * DEFINE THE TEST SUITE
 *===========================================================================*/

static ut_test_case_t pcb_tests[] = {
    UT_TEST(ut_pcb_create_destroy),     UT_TEST(ut_pcb_create_with_pid),
    UT_TEST(ut_pcb_create_no_name),     UT_TEST(ut_pcb_state_transitions),
    UT_TEST(ut_pcb_context_init),       UT_TEST(ut_pcb_multiple_create),
    UT_TEST(ut_pcb_string_conversions), UT_TEST(ut_pcb_with_queue),
};

// Export the test suite
ut_test_suite_t pcb_suite = {
    .suite_name = "PCB Operations",
    .setup = NULL,
    .teardown = NULL,
    .suite_setup = NULL,
    .suite_teardown = NULL,
    .tests = pcb_tests,
    .num_tests = sizeof(pcb_tests) / sizeof(pcb_tests[0]),
};
