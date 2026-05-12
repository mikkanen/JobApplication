#!/bin/bash
rsync -avz --exclude 'build/' --exclude '.git/' ~/Development/hue_jobapplication/ mikkanen@Raspberry5.local:~/Development/hue_jobapplication/
# ssh mikkanen@Raspberry5.local "cd ~/Development/hue_jobapplication/build && make -j$(nproc)"

