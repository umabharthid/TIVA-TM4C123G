 Let me save the context first, then give you the full testing guide.

  Wrote 5 memories (ctrl+o to expand)

● Context saved. Now here's your full testing guide:

  ---
  Stage 1 — Bench test (today/tomorrow, no props)

  Step 1: Flash and check UART

  Connect Tiva C via USB, open serial monitor at 115200 baud. Flash and power on. You should see:
  Calibrating — do not move
  Calibration done
  Arming ESCs...
  ESCs armed
  Set throttle to minimum (1020-1050 us)
  If it hangs at "Calibrating" → I2C wiring issue (check PB2/PB3 to MPU).
  If it hangs at "Set throttle to minimum" → receiver not connected or throttle channel not on PB6.

  Step 2: Verify gyro axes

  Once the loop starts, tilt the drone and watch UART output (prints at 5Hz):
  Thr=1000 Rx=0.00 Ry=0.00 Rz=0.00 M1=1000 M2=1000 M3=1000 M4=1000
  - Roll drone right → Rx should go positive
  - Pitch nose down → Ry should go positive
  - Spin drone clockwise (from above) → Rz should go positive

  If any axis is reversed, you need to negate that axis in MPU_ReadGyro() (e.g. *gx = -(raw_x - gx_off) / 65.5f).

  Step 3: Verify motor corrections (props OFF, throttle ~1150)

  Raise throttle above 1050 so motors arm. Raise to about 1150 so they spin audibly. Now tilt the drone and listen:

  ┌────────────────┬───────────────────────────────────────┐
  │     You do     │           Expected response           │
  ├────────────────┼───────────────────────────────────────┤
  │ Tilt nose down │ Rear motors (M2 RR + M3 RL) speed up  │
  ├────────────────┼───────────────────────────────────────┤
  │ Tilt nose up   │ Front motors (M1 FR + M4 FL) speed up │
  ├────────────────┼───────────────────────────────────────┤
  │ Roll right     │ Left motors (M3 RL + M4 FL) speed up  │
  ├────────────────┼───────────────────────────────────────┤
  │ Roll left      │ Right motors (M1 FR + M2 RR) speed up │
  └────────────────┴───────────────────────────────────────┘

  If corrections are backwards on an axis → that axis is inverted in your IMU orientation. Negate it in MPU_ReadGyro().
  If wrong pair of motors responds → your physical ESC wiring doesn't match the pin assignment. Swap the ESC connectors on the Tiva pins.

  ---
  Stage 2 — PID tuning (props OFF, secured to table)

  With the drone held down or zip-tied, raise throttle to ~1300. Tilt it sharply and watch how fast it corrects:
  - Correction too slow/weak → raise P_RATE_ROLL by 0.1 at a time
  - Correction oscillates (wobbles back and forth) → lower P by 20%, then raise D_RATE_ROLL slightly
  - Drone drifts slowly and doesn't correct → raise I_RATE_ROLL by 0.05 at a time

  Typical end values for this class of drone: P≈0.6–1.0, I≈0.5–1.0, D≈0.02–0.04.

  ---
  Stage 3 — First hover (outdoors, open space)

  Before this:
  - Remove the UART print block (Step 11 in the main loop, clearly marked)
  - Recompile and reflash
  - Double-check all prop directions match motor spin directions (CW prop on CW motor)
  - Do a quick arm test at very low throttle (~1060) to confirm all 4 motors spin

  For the first hover:
  1. Place on flat ground, stand back
  2. Slowly raise throttle — the drone should lift near ~1400–1600 (depends on your motors)
  3. It will drift sideways (no position hold) — that's expected
  4. Bring it down, disarm (throttle to zero)

  If it flips immediately on takeoff → one motor is spinning the wrong direction or motor mixing sign is wrong for that axis. Land, diagnose with
  the UART test again.

  ---
  What to tell me when you're back

  Just say "resuming drone project" and I'll pick up from the memory. Tell me what happened at each stage and I'll help you debug or tune from
  there.
