# How to make a bootable USB stick for updating BIOS/EC via Ubuntu/Linux #
**There's no USB stick for booting in the gift box. If you'd like to use it pls purchase one and follow the indication as below.**

Most of Wibrain B1 series users can update BIOS/EC by a bootable USB stick. Please read this document as follows.

1. Requirements :
```
- USB memory stick that has been already formatted to FAT or FAT32 filesystem.
- Biosup.img from Downloads
- Ubuntu 7.10 or 8.04
```

2. How to make a bootable USB memory stick :
```
1) Turn the power on.
2) Connect the USB stick to your product.
3) Go to 'Applications - Accessories - Terminal'
4) Check the device name of the USB memory stick. (ex) /dev/sda1

sudo fdisk -l

5) Copy the Biosup.img in 'HOME folder' and run this command.

sudo dd if=biosup.img of=/dev/sda1

6) If you finished above installation, restart your product.
```
```
* If the storage of your USB memory stick is more than 512MB, go to "3-1". If not, go to "3-2".
```

3-1. Change the BIOS setup for the USB memory stick over 512MB
```
1) Press DEL key when the Wibrain start-up image shows up.
2) Go to 'Advanced' menu.
3) Go to 'USB Configuration' section, and press [Enter].
4) Go to 'USB Mass Storage Device Configuration', and press [Enter] key.
5) Change 'Emulation Type' to "Force FDD".
6) If you finished above setting, save settings and exit(F10). 
```
```
* After the BIOS/EC has been updated, pls change 'Emulation Type' to "Auto".
```

3-2. Choose the bootable device for the USB memory stick less than 512MB
```
1) Press "F11" when the Wibrain start-up logo shows up.
2) Choose the USB memory stick model from the device list, and press [Enter].
```

4. USB booting menu
```
If you have installed properly, you will see as below text on the display.
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
As per your choice among No. 1~3, your product will be working as below.
```
If you select No. 1 :
Update the BIOS/EC immediately. After updated, B1 will be turned off automatically. After done, turn on the B1 and go on a BIOS. You may find the BIOS and EC version.
```
```
If you select No. 2 :
Read the revision history about B1. After read, press any key. You may select the Boot menu once again. 
```
```
If you select No. 3 :
Exit to DOS. 
```

5. Re-formatting the USB memory stick
```
After you've done all indication, pls re-format the USB stick. 
```