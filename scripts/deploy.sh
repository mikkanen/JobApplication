#!/bin/bash
rsync -avz --exclude 'build/' --exclude '.git/' ~/Development/jobapplication/ mikkanen@Raspberry5.local:~/Development/jobapplication/
# ssh mikkanen@Raspberry5.local "cd ~/Development/jobapplication/build && make -j$(nproc)"

