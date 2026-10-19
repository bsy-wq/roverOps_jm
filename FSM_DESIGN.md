# JSON behavior coordination

`CommandExecutor::execute` validates the envelope, then consults `RobotFsm`
before navigation and mapping execution. MotionHandle and its control commands
are unchanged. Payload and parameter commands keep their existing routes.

## Commands

- `start_follow`: preempt navigation initialization or an active navigation
  environment. Invalidate pending callbacks, stop navigation, verify relevant
  processes have exited, then log the follow placeholder. The response is
  `fsm_state` with `status: placeholder` and `follow_placeholder: true`.
- `stop_follow`: leave the placeholder and return to Idle. No previous navigation
  task is resumed.
- `get_fsm_state`: return a `fsm_state` snapshot without changing behavior.
- `stop_navigation`: bypass the existing command busy gate, invalidate pending
  navigation requests, and return Idle only after cleanup verification. On
  failure enter Fault; a subsequent stop_navigation may retry cleanup.

Navigation starts only in Idle. Follow blocks navigation commands. Mapping
blocks follow and navigation; the existing upload/map-processing workflow stays
in Mapping until `nav-upload-successful` finishes successfully.

## State and cancellation

Navigation environment readiness, localization readiness, and task status are
separate. Generation numbers invalidate older startup and service workers.
Attempt IDs invalidate localization, environment, waypoint and clear-point
timeouts. Behavior operations and callbacks share a recursive execution mutex;
slow map downloads occur outside that mutex, and check their generation before
launching any processes. A stop request invalidates the generation before
waiting for an operation already in progress to leave the mutex.
Service invocation uses a ten-second timeout and checks its generation before
publishing completion. A successful invocation does not mean the robot task has
finished; Running persists until an explicit pause/cancel event.

Health stays Unknown: no trustworthy system-wide health predicate has been
specified yet. No simulated health success is reported. Following is explicitly
a placeholder; no follow hardware command is sent. Motion control still bypasses
behavior coordination by request, so this layer does not provide exclusive
physical motion ownership.

## Verification

Build `tests/robot_fsm_test.cc` with C++17 and pthread. It checks startup
preemption, rejected late completion, failed stop, duplicate follow, mapping
conflicts, stop/restart and concurrent follow admission without moving hardware.
Board compilation checks adapter integration. Live navigation/follow tests
require a separate controlled hardware run.
