# Light control

robot_dog broadcasts a JSON text message once per wall-clock second to every
client connected to the configured websocket.server_light route (currently
/robot/light). Connect using ws://192.168.44.102:9001/robot/light.

Messages contain type=light_controller and exactly one value:

| Priority | Condition | value |
| --- | --- | --- |
| 1 | BMS fault flag or non-SOC alarm level > 0 | bms_error |
| 2 | Latest received BMS SOC < configured threshold | low_battery |
| 3 | StartingFollow or Following | follow |
| 4 | StartingNavigation, NavigationReady or Navigating | navigation |
| 5 | Other states, including initial idle | Default |

SOC-low alarm levels are excluded from the fault condition so the configured
threshold controls low_battery. precharge_mos_fault is also treated as a fault.
Connection-success and working-status flags are not faults.

In build/robot.yaml:

```yaml
light:
  low_battery_percent: 20
```

Allowed range: 0..100; omitted value defaults to 20. Restart robot_dog after
editing this setting. At exactly the threshold low_battery is not active.
Before the first BMS update there is no inferred battery fault/low battery.
Subsequent decisions use the latest BMS sample; there is no stale-data timeout.
Follow reflects the existing FSM placeholder, not confirmation of physical
following. Mapping/stopping/failed modes use Default unless a battery condition
takes priority. Clearing an alarm or recovering SOC restores the current
motion-mode light at the next timer tick.

No inbound command is needed. New connections receive the current light at
the next 1 Hz tick. The first tick occurs once the ROS spin loop is running.
The remote BMS connection behavior is unchanged by this feature.

Validation:

```bash
g++ -std=c++17 -Isrc tests/light_policy_test.cc -o /tmp/grebot_light_policy_test
/tmp/grebot_light_policy_test
```
