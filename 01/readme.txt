** ext4 로그 추가 **
usr/src/linux-5.15/fs/ext4/page-io.c
int ext4_bio_write_page()
    // 로그
	struct dentry *dentry = d_find_alias(inode);
	loff_t offset = (loff_t)page->index << PAGE_SHIFT;

    // 로그
		if (dentry) {
			char *buf = (char *)__get_free_page(GFP_KERNEL);
			if (buf) {
				char *path = dentry_path_raw(dentry, buf, PAGE_SIZE);
				if (!IS_ERR(path)) {
					// "test"가 경로에 포함되어 있을 때 && offset이 512KB 단위일 때만 출력
					if ((strstr(path, "test") != NULL) && (offset % (1024 * 512) == 0)) {
						printk(KERN_INFO "[EXT4][BIO_WRITE] file=%s inode=%lu offset=%llu\n",
							   path, inode->i_ino, (unsigned long long)offset);
					}
					// if (strstr(path, "test") != NULL) {
					// 	printk(KERN_INFO "[EXT4][BIO_WRITE] file=%s inode=%lu offset=%llu\n",
					// 		   path, inode->i_ino, (unsigned long long)offset);
					// }
				}
				free_page((unsigned long)buf);
			}
			dput(dentry);
		}

** 코드 및 수정한 부분 설명 **
EXT4 파일 시스템의 실제 디스크 쓰기(bio write)가 발생하는 시점인 ext4_bio_write_page() 함수에 로그 출력을 추가하였다. 
d_find_alias()를 통해 inode에 대응되는 dentry를 찾고, 
dentry_path_raw()를 사용해 파일 경로를 얻은 뒤, 
경로에 "test"라는 문자열이 포함되어 있고 offset이 512KB 단위일 경우에만 로그를 출력하도록 조건을 걸었다. 
offset은 현재 페이지의 index를 기준으로 계산하였으며, 
로그는 [EXT4][BIO_WRITE] file=경로 inode=번호 offset=값 형태로 출력되어 
디스크에 flush되는 시점의 파일 경로, inode 번호, offset 정보를 확인할 수 있다. 



** btrfs 로그 추가 **
usr/src/linux-5.15/fs/btrfs/extent_io.c
static int __extent_writepage()
    // 로그
	loff_t offset;
	struct dentry *dentry;
	char *buf;
	char *path;

	// 로그
	offset = page_offset(page);
	dentry = d_find_alias(inode);
	if (dentry) {
		buf = (char *)__get_free_page(GFP_KERNEL);
		if (buf) {
			path = dentry_path_raw(dentry, buf, PAGE_SIZE);
			if (!IS_ERR(path)){
				// "test"가 경로에 포함되어 있을 때 &&offset이 512KB 단위일 때만 출력
				if ((strstr(path, "test") != NULL) && (offset % (1024 * 512) == 0)){
					printk(KERN_INFO "[BTRFS][WRITEPAGE] file=%s inode=%lu offset=%llu\n",
						path, inode->i_ino, (unsigned long long)offset);
				}
				// if (strstr(path, "test") != NULL){
				// 	printk(KERN_INFO "[BTRFS][WRITEPAGE] file=%s inode=%lu offset=%llu\n",
				// 		path, inode->i_ino, (unsigned long long)offset);
				// }
			}
			free_page((unsigned long)buf);
		}
		dput(dentry);
	}

** 코드 및 수정한 부분 설명 **
Btrfs 파일 시스템에서 CoW 방식으로 디스크에 flush가 발생하는 시점인 __extent_writepage() 함수에 로그 출력을 추가하였다. 
페이지의 파일 내 위치(offset)는 page_offset()을 통해 계산하였고, 
해당 페이지가 어떤 파일에 속하는지를 확인하기 위해 d_find_alias()로 dentry를 구한 후, 
dentry_path_raw()를 이용해 경로 문자열을 얻었다. 
이후 해당 경로에 "test"가 포함되어 있고 offset이 정확히 512KB 단위일 때만 로그를 출력하도록 조건을 설정하였다. 
출력되는 로그는 [BTRFS][WRITEPAGE] file=경로 inode=번호 offset=값 형식이며, 
이를 통해 특정 실험 대상 파일의 디스크 flush 시점, inode 번호, 쓰기 offset을 명확하게 추적할 수 있다. 



** ext4 테스트 스크립트 **
home/kys/testscript_ext4.sh
#!/bin/bash

home_dir="/home/kys/ext4_dir"

echo "[INFO] Cleaning up old files in ${home_dir}..."
sudo rm -rf ${home_dir}/*

echo "[INFO] Creating initial 2MB test file..."
sudo dd if=/dev/zero of=${home_dir}/test bs=1024 count=2000 status=progress

echo "[INFO] Starting copy operations..."


# 파일 100번 복사
for i in $(seq 1 100); do
    sudo cp ${home_dir}/test ${home_dir}/test_$i
    # 복사 진행 로그 출력
    if (( $i % 10 == 0 )); then
        echo "[INFO] Copied $i files to ${home_dir}"
    fi
done

echo "[INFO] Copy complete!"

** 코드 설명 및 테스트 구동 환경 **
testscript_ext4.sh는 EXT4 파일 시스템에서 디스크 쓰기 동작을 관찰하기 위한 실험용 스크립트다. 
스크립트는 /home/kys/ext4_dir 디렉토리 내에 실험 환경을 구성하며, 먼저 해당 디렉토리의 기존 파일을 모두 삭제한 뒤, dd 명령어를 사용해 2MB 크기의 초기 파일 test를 생성한다. 
이후 이 파일을 test_1부터 test_100까지 100회 복사하며, 10개 단위로 복사 진행 상황을 출력한다. 
복사에는 --reflink 옵션이 사용되지 않아, 매 복사마다 실제 데이터 블록이 디스크에 기록되며 inode도 새로 할당된다. 



** btrfs 테스트 스크립트 **
home/kys/testscript_btrfs..sh
#!/bin/bash

home_dir="/home/kys/btrfs_dir"

echo "[INFO] Cleaning up old files in ${home_dir}..."
sudo rm -rf ${home_dir}/*

echo "[INFO] Creating initial 2MB test file..."
sudo dd if=/dev/zero of=${home_dir}/test bs=1024 count=2000 status=progress

echo "[INFO] Starting copy operations..."

# 파일 100번 복사
for i in $(seq 1 100); do
    sudo cp --reflink=auto ${home_dir}/test ${home_dir}/test_$i
    # 복사 진행 상황 출력
    if (( $i % 10 == 0 )); then
        echo "[INFO] Copied $i files to ${home_dir}"
    fi
done

echo "[INFO] Copy complete!"

** 코드 설명 및 테스트 구동 환경 **
testscript_btrfs.sh는 Btrfs 파일 시스템의 Copy-on-Write(CoW) 동작을 실험적으로 관찰하기 위한 스크립트다. 
실험 대상 디렉토리는 /home/kys/btrfs_dir이며, 스크립트 실행 시 먼저 해당 디렉토리 내 기존 파일들을 모두 삭제하고, dd 명령어를 사용해 2MB 크기의 테스트 파일 test를 생성한다.
이후 이 파일을 test_1부터 test_100까지 100회 복사하는데, cp 명령에 --reflink=auto 옵션을 사용함으로써 Btrfs의 CoW 특성이 활성화된다.
이 방식은 실제 파일 데이터를 복사하지 않고 메타데이터만 복사하며, 파일 내용이 변경되지 않는 한 동일한 데이터 블록을 참조한다. 
복사 과정 중에는 10개 단위로 복사 진행 상황을 출력하여 진행 상태를 확인할 수 있다.