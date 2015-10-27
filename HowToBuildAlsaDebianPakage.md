# Introduction #

우분투 7.10에서  make-kpkg를 사용하여 alsa driver 1.0.16 으로 Backport하는 방법입니다.


# 세부 내용 #


  * ALSA driver 소스 와 fakeroot 설치하기.

```
#sudo su -
# apt-get install alsa-source fakeroot
```

  * ALSA driver 소스 압축 풀기와 debian 폴더 1.0.16으로 이동하기

```
# cd /usr/src
# tar -jxvf alsa-driver.tar.gz
# cd modules
# wget ftp://ftp.alsa-project.org/pub/driver/alsa-driver-1.0.16.tar.bz2
# tar -jxvf alsa-driver-1.0.16.tar.bz2
# mv modules/alsa-driver/debian modules/alsa-driver-1.0.16
# rm -rf modules/alsa-driver
```

  * 커널 헤더에 현재 사용중인 config 파일 복사하기

```
# cd linux-headers-2.6.22-14-generic
# cp /boot/config-2.6.22-14-generic .config
```

  * Makefile 안에  EXTRAVERSION 을 -14-generic으로 변경하기

```
# vi Makefile
//편집
@@ -1,7 +1,7 @@
 VERSION = 2
 PATCHLEVEL = 6
 SUBLEVEL = 22
-EXTRAVERSION = .9
+EXTRAVERSION = -14-generic
 NAME = Holy Dancing Manatees, Batman!
```

  * 커널 유틸리티 제작

```
# make scripts
```

  * make-kpkg을 이용한 커널 alsa 커널 모듈 빌드 및 데비안 패키지 제작

```
# make-kpkg modules_image
```

  * 패키지 설치하기 : 데비안 패키지 결과물은 /usr/src에 존재 합니다.

```
# cd ..
# dpkg -i alsa-modules-2.6.22-14-generic_1.0.14-1ubuntu2+2.6.22.9-10.00.Custom_i386.deb
```

  * alsa-base 파일에 snd-hda-intel 커널 모듈 옵션 추가하기

```
# vi /etc/modprobe.d/alsa.base
//편집 -- 파일 마지막 줄에 아래 내용 추가하고, 저장하기
options snd-hda-intel model=gateway
```