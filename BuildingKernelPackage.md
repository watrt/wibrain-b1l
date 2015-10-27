# 커널 데비안 패키지 생성방법 #

## 미리 설치되어 있어야 하는 꾸러미 ##
  * linux-source-2.6.22, libncurses5-dev, kernel-package

## 꾸러미 설치 방법 ##
  * 미리 설치되어 있어야 하는 꾸러미들이 없는 경우 debian package 생성시 error가 나타날 수 있습니다.
  * Kernel source를 받기위하여 B1L에 설치되어있는 우분투 타스크바 상단메뉴의 [시스템]->[관리]->[소프트웨어 소스] 메뉴를 선택한다.
  * 소프트웨어 소스창 내부의 탭중에 Ubuntu소프트웨어 탭안에 소스코드를 활성화 시킨후 닫기 버튼으로 종료한다.
  * 터미널에서 아래와 같은 명령을 통하여 꾸러미들을 설치할 수 있습니다.

```
sudo apt-get install linux-source-2.6.22 libncurses5-dev, kernel-package
```

  * linux-source 는 /usr/src에 저장되며 압축을 풀어서 사용합니다. 압축해제는 아래와 같이 합니다.

```
// linux-source 받은 곳으로 이동하여 커널 소스압축을 해제 합니다.
cd /usr/src
sudo tar -jxvf linux-source-2.6.22.tar.bz2
```

## 커널 Build 및 Debian package생성방법 ##
  * 터미널을 통하여 아래와 같은 방법으로 커널 build 및 Debian package를 생성할 수 있습니다.

```
// 먼저 Kernel 소스 디렉토리를 linux라는 이름으로 soft link를 걸어줍니다.
sudo ln -s linux-source-2.6.22 linux

// 리눅스 폴더로 이동하여 리눅스 폴더에 설정되어 있는 컴파일 설정을 다 지우고 초기화 합니다.
cd linux
sudo make mrproper

// 현재 B1L에서 사용하고 있는 컴파일 환경을 가져 옵니다.
sudo cp /boot/config-2.6.22-14-generic .config

// 커널을 설정하기 위하여 menuconfig를 사용하여 원하는 부분을 설정합니다.
sudo make menuconfig

// 설정을 저장한 후 아래와 같은 명령으로 Kernel Debian 패키지를 생성합니다.
sudo make-kpkg --append-to-version=test kernel_image --initrd binary

// 생성되면 /usr/src폴더에 Kernel Debian패키지 생성을 확인 할 수 있습니다.
// Kernel Debian Package를 설치하고 싶은 경우 아래와 같은 명령으로 설정 할 수 있습니다.
sudo dpkg -i [생성된 커널 이미지 데비안 패키지]
sudo dpkg -i [생성된 커널 해더 데비안 패키지]

// 정상적으로 설정되었는지 아래 파일을 열어 업데이트 되어진 커널을 확인합니다.
sudo gedit /boot/grub/menu.list

// 재 시작 후 아래의 명령으로 커널버젼을 확인하여 새로운 커널이 정상적으로 적용되었는지 확인합니다.
uname -r
```

## 커널이 정상적으로 부팅되지 않는 경우 ##
  * 커널의 설정 부분 또는 드라이버의 잘못되어진 설정으로 부팅이 되지 않을 수 있습니다.
  * 시스템 재시작시 ESC ('Fn' + 'back space')을 눌러 설치되어있는 커널 menu list를 불러 올 수 있습니다.
  * 기존에 사용하던 generic kernel을 선택하여 정상적으로 부팅시킬 수 있습니다.