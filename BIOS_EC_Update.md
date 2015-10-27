# Ubuntu 상에서 USB 메모리를 부팅 가능하게 만들어 BIOS/EC를 업데이트 하는 방법 #
**USB 메모리는 별매이며, 시중에서 쉽게 구매하실 수 있습니다.**

B1 Ubuntu 사용자 중 LiveUSB가 아닌 LiveCD를 가지고 있는 사용자(혹은 LiveUSB가 깨진 경우)는 별도로 가지고 있는 부팅 가능한 USB 메모리를 통해 BIOS/EC 업데이트를 받을 수 있습니다.

1) 준비물 :
```
☞ 깨끗하게 포맷한 USB 메모리(FAT/FAT32 중 택1)
☞ Downloads 자료실의 Biosup.img
☞ Ubuntu 7.10 or 8.04
```

2) 부팅 USB 만들기 :
```
① B1의 전원을 켠 후, Ubuntu Linux로 부팅합니다.
② 빈 USB 메모리를 B1 USB 단자에 꽂습니다.
③ 응용 프로그램 - 터미널을 실행합니다.
④ 아래 명령으로 USB 메모리의 장치 정보를 확인합니다.

sudo fdisk -l

⑤ Biosup.img를 HOME 폴더에 복사한 후, 아래 명령으로 USB 메모리에 복사합니다.
여기서 USB 메모리의 장치 정보는 /dev/sda1이라 가정하였습니다.

sudo dd if=biosup.img of=/dev/sda1

⑥ 복사가 끝나면 바탕 화면에 있는 USB 메모리를 선택한 후, 마우스 우측 버튼 메뉴의 Unmount를 선택합니다.
⑦ 이제 B1을 재시작합니다.

※ 512MB 이상의 고용량 USB 메모리(1, 2, 4GB 혹은 그 이상)를 사용할 경우에는 3-1, 아닐 경우에는 3-2의 내용을 참고하여 진행해 주시기 바랍니다.
```

3-1) BIOS 설정 변경 - 512MB 이상의 고용량 USB 메모리 사용
```
① Wibrain 로고 화면에서 DEL 키를 눌러 BIOS로 진입합니다.
② 우측 화살표키를 눌러 Advanced 탭으로 이동합니다.
③ 아래 화살표키를 눌러 USB Configuration 항목으로 이동한 후, Enter 키를 누릅니다.
④ 아래 화살표키를 눌러 USB Mass Storage Device Configuration 항목으로 이동한 후, Enter 키를 누릅니다.
⑤ Emulation Type을 Force FDD로 변경합니다.
⑥ F10 키를 눌러 저장후 빠져 나옵니다. 

※ 주의! BIOS/EC 업데이트 후에는 원래값인 Auto로 변경하시기 바랍니다.
```

3-2) 부팅 장치 선택 - 512MB 이하의 저용량 USB 메모리 사용
```
① Wibrain 로고 화면에서 F11키를 누릅니다.
② Boot Device 중에 USB 메모리 항목을 선택한 후, Enter 키를 누릅니다.
```

4) USB 부팅 메뉴
```
정상적으로 부팅되면 아래와 같이 선택 메뉴가 나타납니다.
```
```
*************************************
* BIOS & EC Update for Ubuntu users *
*************************************

WARNING! This will update BIOS and EC for all B1 products!
Please connect D.C Power and battery when updating batch job.

BIOS changes to 1105
EC   changes to 4048

1. Update BIOS and EC now!
2. Read the revision history about BIOS and EC
3. Exit to DOS
Choose the number[1.2.3]?_
```
각 번호를 선택시 실행되는 내용은 아래와 같습니다.
```
1번 - BIOS/EC 업데이트를 실시합니다. 정상적으로 업데이트 된 후에는 B1의 전원이 자동으로 꺼집니다. 이후 BIOS에서 BIOS/EC의 버전이 바뀌었는지 확인하시기 바랍니다.
2번 - BIOS/EC의 버전별 수정 사항과 그 역사를 기록한 문서를 보여줍니다. 본 후에 아무 키나 누르면 다시 선택 메뉴로 돌아옵니다.
3번 - DOS로 빠져나갑니다.
```

5) USB 메모리 재포맷
```
BIOS/EC 업데이트용으로 만든 USB 메모리는 재포맷해야 원래 크기로 돌아옵니다.
```