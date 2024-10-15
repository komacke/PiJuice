#!/bin/bash

CONFIG_FOLDER="/var/lib/pijuice"
I2CBUS_CONFIG_FILE="$CONFIG_FOLDER/pijuice_i2cbus"
PIJUICE_CONFIG_FILE="$CONFIG_FOLDER/pijuice_config.JSON"

main() {
    case $1 in
        --find-bus)
            find_bus
            echo $I2C_BUS
            ;;
        --save-bus)
            find_bus
            save_bus
            ;;
        --wake-rtc)
            wake_rtc
            ;;
        ""|-h|--help)
            usage 
            ;;
        *)
            echo "Invalid option: "$1""
            usage
            ;;
    esac
}

usage() {
cat <<EOF
$(basename $0):
    --find-bus
    --save-bus
    --wake-rtc
    --help|-h
EOF
}

wake_rtc() {
    . $I2CBUS_CONFIG_FILE
    if [ -z "$I2C_BUS" ]; then
        I2C_BUS=1
        echo "WARNING: bus number not configured. Consider using --save-bus option."
    fi

    if [ ! -d /sys/module/rtc_ds1307 ]; then
        echo ds1307 0x68 >/sys/class/i2c-adapter/i2c-$I2C_BUS/new_device 2>&1
        RETVAL=$?
        if [ $RETVAL -ne 0 ]; then
            echo "WARNING: device not available on bus $I2C_BUS. Consider using --save-bus option to discover it."
        fi
    fi
}

find_bus() {
    for f in $(ls -d /sys/class/i2c-dev/*); do
        TEMP_I2C_BUS=$(sed -n 's/^MINOR=\([[:digit:]]\+\)/\1/p' $f/uevent)
        # bang it 3 times fast to wake it up
        for i in {1..3}; do
            /usr/sbin/i2cget -y $TEMP_I2C_BUS 0x14 >/dev/null 2>&1
            RETVAL=$?
            if [ $RETVAL -eq 0 ]; then
                I2C_BUS=$TEMP_I2C_BUS
                break 2
            fi
        done
    done
}

save_bus() {
    echo "I2C_BUS=$I2C_BUS" > "$I2CBUS_CONFIG_FILE"
    sed -i "s/\(.*\"i2c_bus\":\s*\)[[:digit:]]\+\(.*\)/\1$I2C_BUS\2/" $PIJUICE_CONFIG_FILE
}

main "$@"
exit 0
