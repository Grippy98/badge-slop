### Simple Zephyr/LVGL Demos for the AM62L Badge

This project is a collection of games and tools demonstrating the capabilities of the AM62L Badge running Zephyr RTOS and LVGL.

### Included Applications

#### Apps
*   **Badge Mode**: Static Badge with pre-created files
*   **DVD Screensaver**: The mesmerizing bouncing logo.

#### Games
*   **Snake**: Classic snake
*   **Space Invaders**: Defend against waves of aliens.
*   **Brick Breaker**: Break all the bricks with the ball and paddle.
*   **Froggr**: Cross the road and river safely.
*   **Beagle Man**: Pac-Man clone. Avoid the ghost.
*   **Beagle Run**: Endless runner. Jump over obstacles!
*   **Beaglegotchi**: A tamagotchi-style virtual pet

#### Tools 
*   **I2C Scanner**: Scan I2C buses for connected devices.
*   **Serial Monitor**: View UART output on the screen.
*   **Button Test**: Diagnose hardware button inputs.


---

### Build Instructions

1. **Setup Python Virtual Environment:**
   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   ```

2. **Install West and Dependencies:**
   ```bash
   pip install west
   west init -l zephyr
   west update
   pip install -r zephyr/scripts/requirements.txt
   ```

3. **Apply DT Patches:**
   This project requires patches to the Zephyr board definitions to fix button input mappings.
   ```bash
   cd zephyr
   git apply ../am62l_badge_buttons.patch
   cd ..
   ```

4. **Build the Badge Launcher:**
   ```bash
   west build -p always -b am62l_badge/am62l3/a53 Badge-Launcher
   ```

### USB DFU For Debug -

Uboot - 
```
env set dfu_alt_info zephyr.bin ram 0x82000000 0x10000000; dfu 0 ram 0
```

PC - 
```
sudo dfu-util -R -D zephyr/zephyr.bin
```
Uboot - 
```
dcache flush; icache flush; dcache off; icache off; go 0x82000000
```

Repeat....

### Flashing Instructions 

A valid u-boot Boot partition is provided in the "Releases" section. This is intended for use to flash "itself" to the On-board OSPI.

To make development easier, this OSPI will grab uENV.txt and Zephyr.bin from the SD Card to make field changes easier.

1. **Flash the Boot partition with your favorite tool**

2. **Write the relevant bits to OSPI from the UBoot Console**
   
   ```
   sf probe
   fatload mmc 1:1 ${loadaddr} tiboot3.bin
   sf update ${loadaddr} 0x0 ${filesize}
   fatload mmc 1:1 ${loadaddr} tispl.bin
   sf update ${loadaddr} 0x80000 ${filesize}
   fatload mmc 1:1 ${loadaddr} u-boot.img
   sf update ${loadaddr} 0x280000 ${filesize} 
   ```
3. **Now prepare a SD card formatted as FAT32 or FAT16**

Include a zephyr.bin (like the one included in the Releases Section) and a uEnv.txt

Sample uEnv.txt:

```
   uenvcmd=fatload mmc 1:1 0x82000000 zephyr.bin; dcache flush; icache flush; dcache off; icache off; go 0x82000000
```

3. **Place the SD card in the badge, reboot and enjoy!**