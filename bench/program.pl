component(power).
component(battery).
component(solar_array).
component(computer).
component(radio).
component(imu).
component(camera).
component(wheel_left).
component(wheel_right).
component(thermal).

subsystem(power, battery).
subsystem(power, solar_array).
subsystem(avionics, computer).
subsystem(comms, radio).
subsystem(adcs, imu).
subsystem(payload, camera).
subsystem(mobility, wheel_left).
subsystem(mobility, wheel_right).
subsystem(thermal_control, thermal).

observed(battery, voltage_low).
observed(solar_array, current_low).
observed(computer, watchdog_reset).
observed(radio, link_loss).
observed(imu, noisy_data).
observed(wheel_left, stall).
observed(thermal, temperature_high).

fault(battery, undervoltage) :- observed(battery, voltage_low).
fault(solar_array, degraded_power) :- observed(solar_array, current_low).
fault(computer, software_hang) :- observed(computer, watchdog_reset).
fault(radio, comms_fault) :- observed(radio, link_loss).
fault(imu, sensor_fault) :- observed(imu, noisy_data).
fault(wheel_left, actuator_fault) :- observed(wheel_left, stall).
fault(thermal, overheat) :- observed(thermal, temperature_high).

recovery(undervoltage, enter_safe_mode).
recovery(degraded_power, reduce_load).
recovery(software_hang, reboot_computer).
recovery(comms_fault, reset_radio).
recovery(sensor_fault, switch_to_backup_estimator).
recovery(actuator_fault, stop_mobility).
recovery(overheat, disable_payload).

diagnose(Component, Fault, Action) :-
    fault(Component, Fault),
    recovery(Fault, Action).

affected_subsystem(Subsystem, Component, Fault) :-
    subsystem(Subsystem, Component),
    fault(Component, Fault).

critical(battery).
critical(computer).
critical(radio).
critical(thermal).

critical_fault(Component, Fault) :-
    critical(Component),
    fault(Component, Fault).

depends_on(computer, power).
depends_on(radio, computer).
depends_on(camera, computer).
depends_on(imu, computer).
depends_on(wheel_left, computer).
depends_on(wheel_right, computer).
depends_on(thermal, power).

depends_transitive(A, B) :-
    depends_on(A, B).

depends_transitive(A, C) :-
    depends_on(A, B),
    depends_transitive(B, C).

member_of(X, [X|_]).
member_of(X, [_|Xs]) :-
    member_of(X, Xs).

list_length([], zero).
list_length([_|Xs], s(N)) :-
    list_length(Xs, N).

append_list([], Ys, Ys).
append_list([X|Xs], Ys, [X|Zs]) :-
    append_list(Xs, Ys, Zs).

reverse_list([], []).
reverse_list([X|Xs], R) :-
    reverse_list(Xs, RXs),
    append_list(RXs, [X], R).
