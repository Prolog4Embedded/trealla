#pragma once

#include <stdint.h>
extern const unsigned char benchmark_program[];

typedef struct {
  const char *name;
  const char *query;
  uint8_t expected_answers;
} tpl_bench_case_t;


static const uint8_t bench_cases_len = 12;
static const tpl_bench_case_t bench_cases[] = {
    {"component_lookup", "call(component(radio)).", 1},
    {"simple_fault", "call(fault(battery, undervoltage)).", 1},
    {"single_diagnosis", "call(diagnose(battery, Fault, Action)).", 1},
    {"all_diagnoses", "call(call(diagnose(Component, Fault, Action))).", 7},
    {"affected_subsystems", "call(affected_subsystem(S, C, F)).", 7},
    {"critical_faults", "call(critical_fault(C, F)).", 4},
    {"dependency_true", "call(depends_transitive(radio, power)).", 1},
    {"dependency_backtracking", "call(depends_transitive(C, power)).", 7},
    {"list_member", "call(member_of(thermal, [battery, computer, radio, thermal, imu])).", 1},
    {"list_length", "call(list_length([battery, computer, radio, thermal], N)).", 1},
    {"append_list", "call(append_list([battery, computer], [radio, thermal], X)).", 1},
    {"reverse_list", "call(reverse_list([battery, computer, radio, thermal], X)).", 1},
};
