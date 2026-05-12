#!/bin/bash
rsync -avz --exclude 'build/' --exclude '.git/' ~/Development/hue_alarmclock/ mikkanen@Raspberry5.local:~/Development/hue_alarmclock/
# ssh mikkanen@Raspberry5.local "cd ~/Development/hue_alarmclock/build && make -j$(nproc)"
