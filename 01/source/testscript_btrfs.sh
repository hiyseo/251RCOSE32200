#!/bin/bash

home_dir="/home/kys/btrfs_dir"

echo "[INFO] Cleaning up old files in ${home_dir}..."
sudo rm -rf ${home_dir}/*

echo "[INFO] Creating initial 2MB test file..."
sudo dd if=/dev/zero of=${home_dir}/test bs=1024 count=2000 status=progress

echo "[INFO] Starting copy operations..."

# 파일 1000번 복사
# for i in $(seq 1 1000); do
#     sudo cp --reflink=auto ${home_dir}/test ${home_dir}/test_$i

#     # 복사 진행 상황 출력
#     if (( $i % 100 == 0 )); then
#         echo "[INFO] Copied $i files to ${home_dir}"
#     fi
# done

# 파일 500번 복사
# for i in $(seq 1 500); do
#     sudo cp --reflink=auto ${home_dir}/test ${home_dir}/test_$i
#     # 복사 진행 상황 출력
#     if (( $i % 100 == 0 )); then
#         echo "[INFO] Copied $i files to ${home_dir}"
#     fi
# done

# 파일 100번 복사
for i in $(seq 1 100); do
    sudo cp --reflink=auto ${home_dir}/test ${home_dir}/test_$i
    # 복사 진행 상황 출력
    if (( $i % 10 == 0 )); then
        echo "[INFO] Copied $i files to ${home_dir}"
    fi
done

echo "[INFO] Copy complete!"

